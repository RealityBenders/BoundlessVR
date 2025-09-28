#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_BNO08x.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

// Internal library imports
#include "DriverHeaders.h"
#include "DriverPacketLengths.hpp"
#include "MinBiTCore.h"
#include "MinBiTTcpClient.h"

using namespace DriverHeaders;
using Request = std::shared_ptr<MinBiTCore::Request>;

// RTOS Task Handles
TaskHandle_t wifiTaskHandle = NULL;
TaskHandle_t imuTaskHandle = NULL;
TaskHandle_t pingTaskHandle = NULL;
TaskHandle_t connectionTaskHandle = NULL;

// RTOS Queues and Semaphores
QueueHandle_t sensorDataQueue = NULL;
SemaphoreHandle_t protocolMutex = NULL;

// Sensor data structure for queue
struct SensorData {
  uint8_t sensorId;
  union {
    struct {
      float real, i, j, k;
    } quaternion;
    struct {
      uint64_t deltaTime;
      int stepCount;
    } step;
  } data;
};

const char* ssid = "ssid";         // Your Wi-Fi network SSID
const char* password = "password"; // Your Wi-Fi network password
const char* serverIp = "serverip"; // IP address of your TCP server
const uint16_t serverPort = 8080;       // Port of your TCP 

#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET -1

// Global objects (shared between tasks)
MinBiTTcpClient client("TcpClient");
std::shared_ptr<MinBiTCore> protocol;
Adafruit_BNO08x bno08x(BNO08X_RESET);

// Global state variables (protected by mutex when needed)
volatile int lastStepCount = 0;
volatile uint64_t timeSinceLastStep = 0;
volatile bool systemInitialized = false;

// Function declarations
void driverReadHandler(std::shared_ptr<MinBiTCore> protocol, Request request);
void setReports(void);

// RTOS Task Functions
void wifiTask(void *parameter);
void imuTask(void *parameter);
void networkTask(void *parameter);
void pingTask(void *parameter);
void connectionWatchdogTask(void *parameter);

void setup() {
  Serial.begin(115200);
  delay(2000); // Give time for serial connection to establish
  
  Serial.println();
  Serial.println("=== BoundlessVR RTOS Firmware Starting ===");

  // Initialize RTOS primitives
  sensorDataQueue = xQueueCreate(10, sizeof(SensorData));
  protocolMutex = xSemaphoreCreateMutex();

  if (sensorDataQueue == NULL || protocolMutex == NULL) {
    Serial.println("FATAL: Failed to create RTOS primitives!");
    while(1) delay(1000);
  }

  // Initialize IMU early (doesn't depend on network)
  Serial.println("Initializing BNO08x...");
  if (!bno08x.begin_I2C()) {
    Serial.println("FATAL: Failed to find BNO08x chip!");
    while (1) delay(1000);
  }

  Serial.println("BNO08x Found!");
  for (int n = 0; n < bno08x.prodIds.numEntries; n++) {
    Serial.print("Part ");
    Serial.print(bno08x.prodIds.entry[n].swPartNumber);
    Serial.print(": Version :");
    Serial.print(bno08x.prodIds.entry[n].swVersionMajor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionMinor);
    Serial.print(".");
    Serial.print(bno08x.prodIds.entry[n].swVersionPatch);
    Serial.print(" Build ");
    Serial.println(bno08x.prodIds.entry[n].swBuildNumber);
  }

  setReports();
  timeSinceLastStep = micros();

  Serial.println("Creating RTOS tasks...");

  // Create RTOS tasks with appropriate priorities
  xTaskCreatePinnedToCore(
    wifiTask,           // Task function
    "WiFi_Task",        // Task name
    8192,               // Increased stack size
    NULL,               // Parameters
    2,                  // Priority (higher = more important)
    &wifiTaskHandle,    // Task handle
    0                   // Core (0 or 1)
  );

  xTaskCreatePinnedToCore(
    imuTask,
    "IMU_Task",
    8192,               // Increased stack size
    NULL,
    3,                  // High priority for real-time sensor data
    &imuTaskHandle,
    1                   // Run on core 1
  );

  xTaskCreatePinnedToCore(
    networkTask,
    "Network_Task",
    16384,              // Much larger stack for network operations
    NULL,
    2,                  // Lower priority than IMU to prevent starvation
    NULL,
    0
  );

  xTaskCreatePinnedToCore(
    pingTask,
    "Ping_Task",
    4096,               // Increased stack size
    NULL,
    1,                  // Lower priority
    &pingTaskHandle,
    0
  );

  xTaskCreatePinnedToCore(
    connectionWatchdogTask,
    "Connection_Watchdog",
    4096,               // Increased stack size
    NULL,
    1,
    &connectionTaskHandle,
    0
  );

  Serial.println("RTOS tasks created successfully!");
  Serial.println("=== System Ready ===");
}

void setReports(void) {
  Serial.println("Setting desired reports");
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR)) {
    Serial.println("Could not enable rotation vector");
  }
  // if (!bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
  //   Serial.println("Could not enable game rotation vector");
  // }
  if (!bno08x.enableReport(SH2_STEP_COUNTER)) {
    Serial.println("Could not enable step counter");
  }
}

void loop() {
   // Keep loop task alive but idle to prevent watchdog issues
  vTaskDelay(pdMS_TO_TICKS(1000)); // Sleep for 1 second
}

// ============================================================================
// RTOS TASK IMPLEMENTATIONS
// ============================================================================

void wifiTask(void *parameter) {
  Serial.println("[WiFi] Task started");
  
  WiFi.mode(WIFI_STA);
  
  while (true) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.print("[WiFi] Connecting to: ");
      Serial.println(ssid);
      WiFi.begin(ssid, password);
      
      int attempts = 0;
      while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        vTaskDelay(pdMS_TO_TICKS(500));
        Serial.print(".");
        attempts++;
      }
      
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[WiFi] Connected!");
        Serial.print("[WiFi] IP Address: ");
        Serial.println(WiFi.localIP());
        systemInitialized = true;
      } else {
        Serial.println("\n[WiFi] Connection failed, retrying...");
        vTaskDelay(pdMS_TO_TICKS(5000));
      }
    } else {
      // WiFi is connected, check periodically
      vTaskDelay(pdMS_TO_TICKS(10000)); // Check every 10 seconds
    }
  }
}

void imuTask(void *parameter) {
  /*Serial.println("[IMU] Task started");
  sh2_SensorValue_t sensorValue;
  
  // Wait for system initialization
  while (!systemInitialized) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  
  while (true) {
    if (bno08x.wasReset()) {
      Serial.println("[IMU] Sensor was reset, reconfiguring...");
      setReports();
    }

    if (bno08x.getSensorEvent(&sensorValue)) {
      SensorData data;
      bool sendData = false;

      switch (sensorValue.sensorId) {
        case SH2_ROTATION_VECTOR: {
          data.sensorId = SH2_ROTATION_VECTOR;
          data.data.quaternion.real = sensorValue.un.rotationVector.real;
          data.data.quaternion.i = sensorValue.un.rotationVector.i;
          data.data.quaternion.j = sensorValue.un.rotationVector.j;
          data.data.quaternion.k = sensorValue.un.rotationVector.k;
          sendData = true;
          break;
        }
        case SH2_GAME_ROTATION_VECTOR: {
          data.sensorId = SH2_GAME_ROTATION_VECTOR;
          data.data.quaternion.real = sensorValue.un.gameRotationVector.real;
          data.data.quaternion.i = sensorValue.un.gameRotationVector.i;
          data.data.quaternion.j = sensorValue.un.gameRotationVector.j;
          data.data.quaternion.k = sensorValue.un.gameRotationVector.k;
          sendData = true;
          break;
        }
        case SH2_STEP_COUNTER: {
          if (sensorValue.un.stepCounter.steps != lastStepCount) {
            lastStepCount = sensorValue.un.stepCounter.steps;
            uint64_t currTime = micros();
            uint64_t deltaTime = currTime - timeSinceLastStep;
            timeSinceLastStep = currTime;
            
            data.sensorId = SH2_STEP_COUNTER;
            data.data.step.deltaTime = deltaTime;
            data.data.step.stepCount = lastStepCount;
            sendData = true;
          }
          break;
        }
      }

      if (sendData) {
        // Send to queue (non-blocking)
        if (xQueueSend(sensorDataQueue, &data, 0) != pdTRUE) {
          Serial.println("[IMU] Warning: Queue full, dropping sensor data");
        }
      }
    }
    
    vTaskDelay(pdMS_TO_TICKS(10)); // 100Hz sensor polling
  }*/
}

void networkTask(void *parameter) {
  Serial.println("[Network] Task started");
  SensorData receivedData;
  
  // Wait for WiFi to be ready
  while (!systemInitialized) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  
  // Initialize protocol
  protocol = client.getProtocol();
  protocol->loadOutgoingByRequest(&outgoingByRequest);
  client.setReadHandler(&driverReadHandler);

  // Connect to TCP server
  Serial.print("[Network] Connecting to TCP server at ");
  Serial.print(serverIp);
  Serial.print(":");
  Serial.println(serverPort);

  if (!client.begin(serverIp, serverPort)) {
    Serial.println("[Network] Initial connection failed!");
  } else {
    Serial.println("[Network] Connected to server!");
    
    // Send initial ping
    if (xSemaphoreTake(protocolMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      protocol->writeRequest(PING);
      protocol->sendAll();
      xSemaphoreGive(protocolMutex);
    }
  }

  while (true) {
    // Process sensor data from queue
    if (xQueueReceive(sensorDataQueue, &receivedData, pdMS_TO_TICKS(50)) == pdTRUE) {
      if (client.isOpen() && xSemaphoreTake(protocolMutex, pdMS_TO_TICKS(500)) == pdTRUE) {
        switch (receivedData.sensorId) {
          case SH2_ROTATION_VECTOR:
          case SH2_GAME_ROTATION_VECTOR: {
            protocol->writeRequest(IMU_QUAT);
            protocol->writeQuaternionf(Eigen::Quaternionf(
              receivedData.data.quaternion.real,
              receivedData.data.quaternion.i,
              receivedData.data.quaternion.j,
              receivedData.data.quaternion.k
            ));
            protocol->sendAll();
            break;
          }
          case SH2_STEP_COUNTER: {
            protocol->writeRequest(IMU_STEP);
            protocol->writeBytes(reinterpret_cast<uint8_t*>(&receivedData.data.step.deltaTime), 
                                sizeof(receivedData.data.step.deltaTime));
            protocol->sendAll();
            break;
          }
        }
        xSemaphoreGive(protocolMutex);
      }
    }
    protocol->updateData();  
    // Small delay to prevent task starvation
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void pingTask(void *parameter) {
  Serial.println("[Ping] Task started");
  
  // Wait for network to be ready
  while (!systemInitialized) {
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
  
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds
    
    if (client.isOpen() && xSemaphoreTake(protocolMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
      Serial.println("[Ping] Sending periodic ping...");
      protocol->writeRequest(PING);
      protocol->sendAll();
      xSemaphoreGive(protocolMutex);
    }
  }
}

void connectionWatchdogTask(void *parameter) {
  Serial.println("[Watchdog] Task started");
  
  while (true) {
    vTaskDelay(pdMS_TO_TICKS(2000)); // Check every 2 seconds
    
    if (systemInitialized && !client.isOpen()) {
      Serial.println("[Watchdog] Server disconnected. Attempting to reconnect...");
      
      if (xSemaphoreTake(protocolMutex, pdMS_TO_TICKS(1000)) == pdTRUE) {
        if (client.begin(serverIp, serverPort)) {
          Serial.println("[Watchdog] Reconnected to server!");
          protocol->writeRequest(PING);
          protocol->sendAll();
        } else {
          Serial.println("[Watchdog] Reconnection failed.");
        }
        xSemaphoreGive(protocolMutex);
      }
    }
  }
}

// ============================================================================
// CALLBACK FUNCTIONS
// ============================================================================

void driverReadHandler(std::shared_ptr<MinBiTCore> protocol, Request request) {
  if (request->IsTimedOut()) {
    if (request->GetHeader() == PING) {
      Serial.println("[Handler] Connection request timed out. Trying again");
      if (xSemaphoreTake(protocolMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        protocol->writeRequest(PING);
        protocol->sendAll();
        xSemaphoreGive(protocolMutex);
      }
    }
  }
  
  // Process response headers
  switch (request->GetHeader()) {
    case PING: {
      if (request->GetResponseHeader() == ACK) {
        Serial.println("[Handler] Ping acknowledged by server.");
      } else {
        Serial.println("[Handler] Connection denied.");
        if (xSemaphoreTake(protocolMutex, pdMS_TO_TICKS(100)) == pdTRUE) {
          protocol->writeRequest(PING);
          protocol->sendAll();
          xSemaphoreGive(protocolMutex);
        }
      }
      digitalWrite(LED_BUILTIN, HIGH);
      break;
    }
    case IMU_QUAT: {
      if (request->GetResponseHeader() != ACK) {
        Serial.println("[Handler] Unexpected response to IMU quaternion data.");
      } else {
        Serial.println("[Handler] Quaternion data acknowledged by server.");
      }
      break;
    }
    case IMU_STEP: {
      if (request->GetResponseHeader() != ACK) {
        Serial.println("[Handler] Unexpected response to IMU step data.");
      } else {
        Serial.println("[Handler] Step data acknowledged by server.");
      }
      break;
    }
  }
}
#include <WiFi.h>
#include <WiFiClient.h>
#include <Adafruit_BNO08x.h>

// Internal library imports
#include "DriverHeaders.h"
#include "DriverPacketLengths.hpp"
#include "MinBiTCore.h"
#include "MinBiTTcpClient.h"

using namespace DriverHeaders;
using Request = std::shared_ptr<MinBiTCore::Request>;

const char* ssid = "ssid";         // Your Wi-Fi network SSID
const char* password = "password"; // Your Wi-Fi network password
const char* serverIp = "serverip"; // IP address of your TCP server
const uint16_t serverPort = 8080;       // Port of your TCP 

#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET -1

MinBiTTcpClient client("TcpClient");
std::shared_ptr<MinBiTCore> protocol;
Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;
int lastStepCount = 0;
int timeSinceLastStep = 0;


void driverReadHandler(Request request);
void setReports(void);

void setup() {
  WiFi.mode(WIFI_STA);
  Serial.begin(115200);
  Serial.println();

  // Connect to Wi-Fi
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // Initialize protocol
  protocol = client.getProtocol();
  protocol->loadOutgoingByRequest(&outgoingByRequest);
  protocol->setReadHandler(&driverReadHandler);

  // Connect to TCP server
  Serial.print("Connecting to TCP server at ");
  Serial.print(serverIp);
  Serial.print(":");
  Serial.println(serverPort);

  if (!client.begin(serverIp, serverPort)) {
    Serial.println("Connection failed!");
    return;
  }
  Serial.println("Connected to server!");

  // Send data to the server
  String message = "Hello from ESP32!";
  protocol->writeRequest(PING);
  protocol->sendAll();
  Serial.print("Pinged");

  if (!bno08x.begin_I2C()) {
    // if (!bno08x.begin_UART(&Serial1)) {  // Requires a device with > 300 byte
    // UART buffer! if (!bno08x.begin_SPI(BNO08X_CS, BNO08X_INT)) {
    Serial.println("Failed to find BNO08x chip");
    while (1) {
      delay(10);
    }
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

  Serial.println("Reading events");
  delay(100);

  timeSinceLastStep = micros();
}

void setReports(void) {
  Serial.println("Setting desired reports");
  if (!bno08x.enableReport(SH2_ROTATION_VECTOR)) {
    Serial.println("Could not enable rotation vector");
  }
  // if (!bno08x.enableReport(SH2_GAME_ROTATION_VECTOR)) {
  //   Serial.println("Could not enable game rotation vector");
  // }
  // if (!bno08x.enableReport(SH2_STEP_COUNTER)) {
  //   Serial.println("Could not enable step counter");
  // }
}

void loop() {

  delay(10);

  if (bno08x.wasReset()) {
    Serial.print("sensor was reset ");
    setReports();
  }

  if (!bno08x.getSensorEvent(&sensorValue)) {
    return;
  }

  switch (sensorValue.sensorId) {
    case SH2_ROTATION_VECTOR: {
      Serial.println("Got rotation vector");
      protocol->writeRequest(IMU_QUAT);
      Serial.println("Writing quaternion");
      protocol->writeQuaternionf(Eigen::Quaternionf(sensorValue.un.rotationVector.real, sensorValue.un.rotationVector.i, sensorValue.un.rotationVector.j, sensorValue.un.rotationVector.k));
      Serial.println("Sending all");
      protocol->sendAll();
      Serial.print("Rotation Vector: ");
      Serial.print(sensorValue.un.rotationVector.real);
      Serial.print(", ");
      Serial.print(sensorValue.un.rotationVector.i);
      Serial.print(", ");
      Serial.print(sensorValue.un.rotationVector.j);
      Serial.print(", ");
      Serial.print(sensorValue.un.rotationVector.k);
      Serial.println();
      break;
    }
    case SH2_GAME_ROTATION_VECTOR: {
      Serial.println("Got game rotation vector");
      protocol->writeRequest(IMU_QUAT);
      Serial.println("Writing quaternion");
      protocol->writeQuaternionf(Eigen::Quaternionf(sensorValue.un.gameRotationVector.real, sensorValue.un.gameRotationVector.i, sensorValue.un.gameRotationVector.j, sensorValue.un.gameRotationVector.k));
      Serial.println("Sending all");
      protocol->sendAll();
      Serial.print("Game Rotation Vector: ");
      Serial.print(sensorValue.un.gameRotationVector.real);
      Serial.print(", ");
      Serial.print(sensorValue.un.gameRotationVector.i);
      Serial.print(", ");
      Serial.print(sensorValue.un.gameRotationVector.j);
      Serial.print(", ");
      Serial.print(sensorValue.un.gameRotationVector.k);
      Serial.println();
      break;
    }

    case SH2_STEP_COUNTER: {
      Serial.println("Got step counter");
      if (sensorValue.un.stepCounter.steps == lastStepCount) {
        break; // No new steps
      }
      lastStepCount = sensorValue.un.stepCounter.steps;
      Serial.println("New step detected");
      uint64_t currTime = micros();
      uint64_t deltaTime = currTime - timeSinceLastStep;
      timeSinceLastStep = currTime;
      Serial.println("Writing step data");
      protocol->writeRequest(IMU_STEP);
      Serial.println("Writing delta time");
      protocol->writeBytes(reinterpret_cast<uint8_t*>(&deltaTime), sizeof(deltaTime));
      Serial.println("Sending all");
      protocol->sendAll();
      Serial.print("Step Count: ");
      Serial.print(sensorValue.un.stepCounter.steps);
      Serial.print(", Delta Time: ");
      Serial.print(deltaTime);
      Serial.println(" microseconds");
      break;
    }
  }

  // Optional: Close and reconnect or handle disconnects
  if (!client.isOpen()) {
    Serial.println("Server disconnected. Attempting to reconnect...");
    if (client.begin(serverIp, serverPort)) {
      Serial.println("Reconnected to server!");
    } else {
      Serial.println("Reconnection failed.");
      delay(5000); // Wait before trying again
    }
  }
}

void driverReadHandler(Request request) {
  if (request->IsTimedOut()) {
    if (request->GetHeader() == PING) {
      Serial.println("Connection request timed out. Trying again"); // Use Serial instead
      // Sends another ping
      protocol->writeRequest(PING);
      protocol->sendAll();
    }
  }
	//Reads initial packet header
	switch (request->GetHeader()) {
		case PING: {
			// Checks response
      if (request->GetResponseHeader() == ACK) {
        Serial.println("Ping acknowledged by server.");
      } else {
        Serial.println("Connection denied.");
        // Sends another ping
        protocol->writeRequest(PING);
        protocol->sendAll();
      }
			digitalWrite(LED_BUILTIN, HIGH);
			break;
    }
		case IMU_QUAT: {
			// Checks response
      if (request->GetResponseHeader() != ACK) {
        Serial.println("Unexpected response to IMU quaternion data.");
        break;
      }
			break;
		}
    case IMU_STEP: {
      // Checks response
      if (request->GetResponseHeader() != ACK) {
        Serial.println("Unexpected response to IMU step data.");
        break;
      }
      break;
    }
  }
}
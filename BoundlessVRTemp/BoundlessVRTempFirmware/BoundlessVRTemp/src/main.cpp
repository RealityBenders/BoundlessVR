#include <Arduino.h>
#include <WiFi.h>
#include <Adafruit_BNO08x.h>
#include <WiFiClient.h>

const char* ssid = "ssid";         // Your Wi-Fi network SSID
const char* password = "password"; // Your Wi-Fi network password
const char* serverIp = "serverip"; // IP address of your TCP server
const uint16_t serverPort = 8080;       // Port of your TCP server

WiFiClient client; // TCP client
 // Your Wi-Fi network password

#define BNO08X_CS 10
#define BNO08X_INT 9
#define BNO08X_RESET -1

Adafruit_BNO08x bno08x(BNO08X_RESET);
sh2_SensorValue_t sensorValue;
int lastStepCount = 0;

// put function declarations here:
int myFunction(int, int);

void setReports(void);

void connectToServer() {
  while (!client.connected()) {
    Serial.println("Connecting to server...");
    if (client.connect(serverIp, serverPort)) {
      Serial.println("Connected to server!");
    } else {
      Serial.println("Connection failed! Retrying in 5 seconds...");
      delay(5000);
    }
  }
}

void sendToServer(const char* data) {
  if (!client.connected()) {
    connectToServer();
  }
  client.println(data);
}

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
  
  // Connect to TCP server
  connectToServer();

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
      char data[128];
      snprintf(data, sizeof(data), "ROT,%f,%f,%f,%f", 
        sensorValue.un.rotationVector.real,
        sensorValue.un.rotationVector.i,
        sensorValue.un.rotationVector.j,
        sensorValue.un.rotationVector.k);
      sendToServer(data);
      
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
      char data[128];
      snprintf(data, sizeof(data), "ROT,%f,%f,%f,%f",
        sensorValue.un.rotationVector.real,
        sensorValue.un.rotationVector.i,
        sensorValue.un.rotationVector.j,
        sensorValue.un.rotationVector.k);
      sendToServer(data);

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
      if (sensorValue.un.stepCounter.steps == lastStepCount) {
        break; // No new steps
      }
      lastStepCount = sensorValue.un.stepCounter.steps;
      uint64_t currTime = micros();
      
      char data[128];
      snprintf(data, sizeof(data), "STP,%d,%llu", 
        lastStepCount,
        currTime);
      sendToServer(data);
      
      Serial.println("New step detected");
      Serial.print("Step Time: ");
      Serial.print(currTime);
      Serial.println(" microseconds");
      break;
    }
  }
}

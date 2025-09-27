#include <WiFi.h>
#include <WiFiClient.h>

// Internal library imports
#include "DriverHeaders.h"
#include "DriverPacketLengths.hpp"
#include "MinBiTCore.h"
#include "MinBiTTcpClient.h"

using namespace DriverHeaders;
using Request = std::shared_ptr<MinBiTCore::Request>;

const char* ssid = "network";         // Your Wi-Fi network SSID
const char* password = "password"; // Your Wi-Fi network password
const char* serverIp = "SERVER_IP_ADDRESS"; // IP address of your TCP server
const uint16_t serverPort = 8080;       // Port of your TCP server

MinBiTTcpClient client("TcpClient");
std::shared_ptr<MinBiTCore> protocol;

void driverReadHandler(Request request);

void setup() {
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
}

void loop() {
  // Send data to the server
  String message = "Hello from ESP32!";
  protocol->writeRequest(PING);
  protocol->sendAll();
  Serial.print("Pinged");

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

  delay(5000); // Send/receive every 5 seconds
}

void driverReadHandler(Request request) {
	// Ensures request did not time out
	if (request->IsTimedOut())
	{
		return;
	}
	//Reads initial packet header
	switch (request->GetHeader()) {
		case PING:
			// Checks response
      if (request->GetResponseHeader() == ACK) {
        Serial.println("Ping acknowledged by server.");
      } else {
        Serial.println("Unexpected response to ping.");
      }
			digitalWrite(LED_BUILTIN, HIGH);
			break;
		case IMU_DATA: {
			// Checks response
      if (request->GetResponseHeader() != ACK) {
        Serial.println("Unexpected response to IMU data.");
        break;
      }
			break;
		}
  }
}
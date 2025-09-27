#include <WiFi.h>
#include <WiFiClient.h>

const char* ssid = "PLACEHOLDER_SSID";         // Your Wi-Fi network SSID
const char* password = "PLACEHOLDER_PASSWORD"; // Your Wi-Fi network password
const char* serverIp = "PLACEHOLDER_SERVER_IP"; // IP address of your TCP server
const uint16_t serverPort = 8080;       // Port of your TCP server

WiFiClient client;

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

  // Connect to TCP server
  Serial.print("Connecting to TCP server at ");
  Serial.print(serverIp);
  Serial.print(":");
  Serial.println(serverPort);

  if (!client.connect(serverIp, serverPort)) {
    Serial.println("Connection failed!");
    return;
  }
  Serial.println("Connected to server!");
}

void loop() {
  // Send data to the server
  String message = "Hello from ESP32!";
  client.println(message);
  Serial.print("Sent: ");
  Serial.println(message);

  // Receive data from the server
  if (client.available()) {
    String response = client.readStringUntil('\n');
    Serial.print("Received: ");
    Serial.println(response);
  }

  // Optional: Close and reconnect or handle disconnects
  if (!client.connected()) {
    Serial.println("Server disconnected. Attempting to reconnect...");
    if (client.connect(serverIp, serverPort)) {
      Serial.println("Reconnected to server!");
    } else {
      Serial.println("Reconnection failed.");
      delay(5000); // Wait before trying again
    }
  }

  delay(5000); // Send/receive every 5 seconds
}
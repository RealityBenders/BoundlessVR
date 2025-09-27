#ifndef MINBIT_TCP_CLIENT_H
#define MINBIT_TCP_CLIENT_H

#include <memory>
#include <string>
#include <Wifi.h>
#include <WiFiClient.h>
#include "TcpStream.h"
#include "MinBiTCore.h"

typedef WiFiClient WifiClient;

class MinBiTTcpClient {
public:
    using ReadHandler = std::function<void(std::shared_ptr<MinBiTCore>, std::shared_ptr<MinBiTCore::Request>)>;

    MinBiTTcpClient(std::string name);
    ~MinBiTTcpClient();

    // Initialize the TCP client and connect to the server
    bool begin(const char* serverIp, uint16_t serverPort);

    // Sets read handler
    void setReadHandler(ReadHandler readHandler);

    // Close the serial port
    void end();

    // Get the MinBiTCore protocol object
    std::shared_ptr<MinBiTCore> getProtocol();

    // Check if the serial port is open
    bool isOpen() const;

private:
    std::shared_ptr<WifiClient> tcpClient;     
    std::shared_ptr<TcpStream> tcpStream;
    std::shared_ptr<MinBiTCore> protocol;
    std::thread ioThread;
    bool running = false;

    ReadHandler readHandler;

    // Attaches protocol
    void attachProtocol();
};

#endif // MINBIT_TCP_CLIENT_H
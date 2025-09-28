#ifndef TCP_STREAM_H
#define TCP_STREAM_H

#include <Arduino.h>
#include <memory>
#include <WiFi.h>
#include <WiFiClient.h>

#include "IStream.h"

typedef WiFiClient WifiClient;

class TcpStream : public IStream {
    public:
        explicit TcpStream(std::shared_ptr<WifiClient> client);

        void write(const uint8_t* buffer, std::size_t length) override;
        void read(uint8_t* buffer, std::size_t length) override;
        uint8_t available() override;
        bool isOpen() override;
        void close() override;
    private:
        std::shared_ptr<WifiClient> client;
};

#endif
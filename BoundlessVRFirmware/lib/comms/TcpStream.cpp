#include "TcpStream.h"

TcpStream::TcpStream(std::shared_ptr<WifiClient> client) {
    this->client = client;
}

void TcpStream::write(const uint8_t* buffer, std::size_t length) {
    if (client) {
        client->write(buffer, length);
    }
}

void TcpStream::read(uint8_t* buffer, std::size_t length) {
    if (client) {
        client->readBytes(buffer, length);
    }
}

uint8_t TcpStream::available() {
    if (client) {
        return client->available();
    }
    return 0;
}

bool TcpStream::isOpen() {
    if (client) {
        return client->connected();
    }
    return false;
}

void TcpStream::close() {
    if (client && client->connected()) {
        client->stop();
    }
}

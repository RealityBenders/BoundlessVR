#include "MinBiTTcpClient.h"

MinBiTTcpClient::MinBiTTcpClient(std::string name)
    : tcpClient(std::make_shared<WifiClient>()),
    tcpStream(std::make_shared<TcpStream>(tcpClient)),
    protocol(std::make_shared<MinBiTCore>(name, tcpStream))
{
    protocol->setWriteMode(MinBiTCore::WriteMode::BULK);
    protocol->setRequestTimeout(1000);
}

MinBiTTcpClient::~MinBiTTcpClient() {
    end();
}

bool MinBiTTcpClient::begin(const char* serverIp, uint16_t serverPort) {
    if (!tcpClient->connect(serverIp, serverPort)) {
        return false;
    }
    attachProtocol();
    
    return true;
}

void MinBiTTcpClient::setReadHandler(ReadHandler readHandler) {
    this->readHandler = readHandler;
}

void MinBiTTcpClient::attachProtocol() {
    protocol->setReadHandler([this](std::shared_ptr<MinBiTCore::Request> request) {
        if (readHandler) {
            readHandler(protocol, request);
        }
    });
}

void MinBiTTcpClient::end() {
    // Stop background thread
    running.store(false);
    if (ioThread.joinable()) {
        ioThread.join();
    }

    if (tcpStream) tcpStream->close();
}

std::shared_ptr<MinBiTCore> MinBiTTcpClient::getProtocol() {
    return protocol;
}

bool MinBiTTcpClient::isOpen() const {
    return tcpStream && tcpStream->isOpen();
}
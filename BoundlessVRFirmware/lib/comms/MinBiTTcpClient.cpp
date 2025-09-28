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
    
    // Start background IO thread
    if (!running.load()) {
        running.store(true);
        ioThread = std::thread([this]() {
            // Poll loop: call fetchData periodically while running
            while (running.load()) {
                try {
                    if (protocol) protocol->fetchData();
                }
                catch (...) {
                    // Swallow exceptions to keep the thread alive
                }
                // Small sleep to avoid busy-waiting; adjust as needed
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        });
    }
    return true;
}

void MinBiTTcpClient::setReadHandler(ReadHandler readHander) {
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
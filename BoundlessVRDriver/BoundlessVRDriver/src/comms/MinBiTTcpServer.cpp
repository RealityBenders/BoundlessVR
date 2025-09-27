#include "MinBiTTcpServer.h"

MinBiTTcpServer::MinBiTTcpServer(std::string name, unsigned short port)
    : name(name), listenPort(port)
{
}

MinBiTTcpServer::~MinBiTTcpServer() {
    end();
}

bool MinBiTTcpServer::begin() {
    try {
        acceptor = std::make_unique<boost::asio::ip::tcp::acceptor>(
            ioContext,
            boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), listenPort)
        );
        auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);
        acceptor->accept(*socket);
        tcpStream = std::make_shared<TcpStream>(socket);
        protocol = std::make_shared<MinBiTCore>(name, tcpStream);
        protocol->setEndianness(MinBiTCore::Endianness::BigEndian);
        protocol->setWriteMode(MinBiTCore::WriteMode::IMMEDIATE);
        protocol->setRequestTimeout(500);
        connected = true;
        ioThread = std::thread([this]() { ioContext.run(); });
        attachProtocol();
        return true;
    }
    catch (const std::exception& e) {
        connected = false;
        return false;
    }
}

void MinBiTTcpServer::setReadHandler(ReadHandler handler) {
    this->readHandler = handler;
}

void MinBiTTcpServer::attachProtocol() {
    protocol->setReadHandler([this](std::shared_ptr<MinBiTCore::Request> request) {
        if (readHandler) {
            readHandler(protocol, request);
        }
        if (connected) {
            protocol->asyncFetchByte();
        }
        });
    protocol->asyncFetchByte();
}

void MinBiTTcpServer::end() {
    if (connected) {
        connected = false;
        if (tcpStream && tcpStream->isOpen()) {
            tcpStream->close();
        }
        ioContext.stop();
        if (ioThread.joinable()) {
            ioThread.join();
        }
    }
}

std::shared_ptr<MinBiTCore> MinBiTTcpServer::getProtocol() {
    return protocol;
}

bool MinBiTTcpServer::isConnected() const {
    return tcpStream && tcpStream->isOpen();
}
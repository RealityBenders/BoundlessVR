#include "MinBiTTcpServer.h"
#include <openvr_driver.h>

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
        running = true;
        // Launch worker threads for ioContext.run()
        unsigned int threadCount = std::thread::hardware_concurrency();
        if (threadCount == 0) threadCount = 4;
        for (unsigned int i = 0; i < threadCount; ++i) {
            workerThreads.emplace_back([this]() { ioContext.run(); });
        }
        // Start accepting clients asynchronously
        acceptClients();
        return true;
    }
    catch (const std::exception& e) {
        running = false;
        return false;
    }
}

void MinBiTTcpServer::acceptClients() {
    if (!running) return;
    auto socket = std::make_shared<boost::asio::ip::tcp::socket>(ioContext);
    acceptor->async_accept(*socket, [this, socket](const boost::system::error_code& ec) {
        if (!ec && running) {
            std::string clientIp;
            try {
                clientIp = socket->remote_endpoint().address().to_string();
            }
            catch (const std::exception&) {
                clientIp = "Unknown";
            }
            // Log client connection with IP using OpenVR logging
            std::string logMsg = "Client connected: " + clientIp;
            VRDriverLog()->Log(logMsg.c_str());
            int clientId = nextClientId++;
            auto tcpStream = std::make_shared<TcpStream>(socket);
            auto protocol = std::make_shared<MinBiTCore>(name, tcpStream);
            protocol->setEndianness(MinBiTCore::Endianness::BigEndian);
            protocol->setWriteMode(MinBiTCore::WriteMode::IMMEDIATE);
            protocol->setRequestTimeout(500);

            // Call the initialization handler if set
            if (initHandler) {
                initHandler(protocol);
            }

            {
                std::lock_guard<std::mutex> lock(clientMutex);
                clientStreams[clientId] = tcpStream;
                clientProtocols[clientId] = protocol;
            }

            // Set read handler if available
            if (readHandler) {
                protocol->setReadHandler([this, protocol](std::shared_ptr<MinBiTCore::Request> request) {
                    readHandler(protocol, request);
                    });
            }

            // Start the async read chain - this is critical
            protocol->asyncFetchByte();
        }
        else if (ec) {
            // Log the error for debugging
            VRDriverLog()->Log(("Accept error: " + ec.message()).c_str());
        }
        // Accept next client
        acceptClients();
        });
}

void MinBiTTcpServer::setReadHandler(ReadHandler handler) {
    this->readHandler = handler;
}

void MinBiTTcpServer::setInitHandler(InitHandler handler) {
    this->initHandler = handler;
}

std::vector<std::shared_ptr<MinBiTCore>> MinBiTTcpServer::getProtocols() {
    std::lock_guard<std::mutex> lock(clientMutex);
    std::vector<std::shared_ptr<MinBiTCore>> protocols;
    for (auto& kv : clientProtocols) {
        protocols.push_back(kv.second);
    }
    return protocols;
}

bool MinBiTTcpServer::isConnected() {
    std::lock_guard<std::mutex> lock(clientMutex);
    for (const auto& kv : clientStreams) {
        if (kv.second && kv.second->isOpen()) return true;
    }
    return false;
}

void MinBiTTcpServer::end() {
    running = false;
    if (acceptor) acceptor->close();
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (auto& kv : clientStreams) {
            if (kv.second && kv.second->isOpen()) kv.second->close();
        }
    }
    ioContext.stop();
    for (auto& t : workerThreads) {
        if (t.joinable()) t.join();
    }
    workerThreads.clear();
    clientStreams.clear();
    clientProtocols.clear();
}
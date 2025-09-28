#include "MinBiTTcpServer.h"


using namespace vr;

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
            } catch (const std::exception&) {
                clientIp = "Unknown";
            }
            // Log client connection with IP using OpenVR logging
            std::string logMsg = "Client connected: " + clientIp;
            VRDriverLog()->Log(logMsg.c_str());
            int clientId = nextClientId++;
			clientSockets.push_back(socket);
            // Starts asychronous read chain for this client
            asyncReadLoop(socket);
        }
        // Accept next client
        acceptClients();
    });
}

void MinBiTTcpServer::asyncReadLoop(std::shared_ptr<boost::asio::ip::tcp::socket> socket) {
    if (!socket || !socket->is_open()) return;

    auto tempBuffer = std::make_shared<std::vector<uint8_t>>(1);
    socket->async_read_some(boost::asio::buffer(tempBuffer->data(), 1),
        [this, tempBuffer, socket](const boost::system::error_code& error, std::size_t bytesTransferred) {
            if (!error) {
                if (readHandler) {
                    readHandler(tempBuffer->data());
				}
            }
            else {
                VRDriverLog()->Log(("(" + name + ") Error reading from stream: " + error.message()).c_str());
            }
            asyncReadLoop(socket);
        });
}

void MinBiTTcpServer::setReadHandler(ReadHandler handler) {
    this->readHandler = handler;
}

void MinBiTTcpServer::end() {
    running = false;
    if (acceptor) acceptor->close();
    {
        std::lock_guard<std::mutex> lock(clientMutex);
        for (auto& kv : clientSockets) {
            if (kv && kv->is_open()) kv->close();
        }
    }
    ioContext.stop();
    for (auto& t : workerThreads) {
        if (t.joinable()) t.join();
    }
    workerThreads.clear();
    clientSockets.clear();
}
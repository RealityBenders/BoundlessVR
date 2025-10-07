#ifndef MINBIT_TCP_SERVER_H
#define MINBIT_TCP_SERVER_H

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <openvr_driver.h>
#include <boost/asio.hpp>

using namespace vr;

using StreamHandler = std::function<void(const boost::system::error_code&, std::size_t)>;

class MinBiTTcpServer {
public:
    using ReadHandler = std::function<void(std::shared_ptr<MinBiTCore>, std::shared_ptr<MinBiTCore::Request>)>;
    using InitHandler = std::function<void(std::shared_ptr<MinBiTCore>)>;

    MinBiTTcpServer(std::string name, unsigned short port);
    ~MinBiTTcpServer();

    // Start listening for client connections
    bool begin();

    // Stop the server and close all connections
    void end();

    // Set the user read handler
    void setReadHandler(ReadHandler handler);

    // Set the initialization handler for new clients
    void setInitHandler(InitHandler handler);

    // Get all active protocols
    std::vector<std::shared_ptr<MinBiTCore>> getProtocols();

    // Check if any client is connected
    bool isConnected();

private:
    std::string name;
    boost::asio::io_context ioContext;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
	std::vector<std::shared_ptr<boost::asio::ip::tcp::socket>> clientSockets;
    unsigned short listenPort;
    ReadHandler readHandler;
    InitHandler initHandler;
    std::vector<std::thread> workerThreads;
    std::mutex clientMutex;
    std::atomic<int> nextClientId{ 0 };
    std::thread ioThread;
    bool running = false;

    // Accept clients in a loop
    void acceptClients();
    void asyncReadLoop(std::shared_ptr<boost::asio::ip::tcp::socket> socket);
};

#endif // MINBIT_TCP_SERVER_H
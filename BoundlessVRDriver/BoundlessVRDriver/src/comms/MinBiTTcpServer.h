#ifndef MINBIT_TCP_SERVER_H
#define MINBIT_TCP_SERVER_H

#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <map>
#include <mutex>
#include <boost/asio.hpp>
#include "TcpStream.h"
#include "MinBiTCore.h"

class MinBiTTcpServer {
public:
    using ReadHandler = std::function<void(std::shared_ptr<MinBiTCore>, std::shared_ptr<MinBiTCore::Request>)>;

    MinBiTTcpServer(std::string name, unsigned short port);
    ~MinBiTTcpServer();

    // Start listening for client connections
    bool begin();

    // Stop the server and close all connections
    void end();

    // Set the user read handler
    void setReadHandler(ReadHandler handler);

    // Get all active protocols
    std::vector<std::shared_ptr<MinBiTCore>> getProtocols();

    // Check if any client is connected
    bool isConnected() const;

private:
    std::string name;
    boost::asio::io_context ioContext;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    unsigned short listenPort;
    ReadHandler readHandler;
    std::vector<std::thread> workerThreads;
    std::map<int, std::shared_ptr<TcpStream>> clientStreams;
    std::map<int, std::shared_ptr<MinBiTCore>> clientProtocols;
    std::mutex clientMutex;
    std::atomic<int> nextClientId{0};
    std::thread ioThread;
    bool running = false;

    // Accept clients in a loop
    void acceptClients();
    // Worker thread for each client
    void clientWorker(int clientId);
};

#endif // MINBIT_TCP_SERVER_H
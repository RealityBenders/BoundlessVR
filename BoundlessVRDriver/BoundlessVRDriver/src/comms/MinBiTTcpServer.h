#ifndef MINBIT_TCP_SERVER_H
#define MINBIT_TCP_SERVER_H

#include <memory>
#include <string>
#include <thread>
#include <boost/asio.hpp>
#include "TcpStream.h"
#include "MinBiTCore.h"

class MinBiTTcpServer {
public:
    using ReadHandler = std::function<void(std::shared_ptr<MinBiTCore>, std::shared_ptr<MinBiTCore::Request>)>;

    MinBiTTcpServer(std::string name, unsigned short port);
    ~MinBiTTcpServer();

    // Start listening for a client connection
    bool begin();

    // Stop the server and close the connection
    void end();

    // Get the MinBiTCore protocol object
    std::shared_ptr<MinBiTCore> getProtocol();

    // Check if a client is connected
    bool isConnected() const;

    // Set the user read handler
    void setReadHandler(ReadHandler handler);

private:
    std::string name;
    boost::asio::io_context ioContext;
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor;
    std::shared_ptr<TcpStream> tcpStream;
    std::shared_ptr<MinBiTCore> protocol;
    std::thread ioThread;
    bool connected = false;
    unsigned short listenPort;
    ReadHandler readHandler;

    // Attach protocol and start async read loop
    void attachProtocol();
};

#endif // MINBIT_TCP_SERVER_H
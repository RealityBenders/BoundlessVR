#ifndef TCPSTREAM_H
#define TCPSTREAM_H

#include "IStream.h"

class TcpStream : public IStream {
public:
    explicit TcpStream(std::shared_ptr<boost::asio::ip::tcp::socket> socket);

    std::shared_ptr<boost::asio::ip::tcp::socket> getSocket();
    void asyncWrite(const uint8_t* buffer, std::size_t length, StreamHandler handler) override;
    void asyncRead(uint8_t* buffer, std::size_t length, StreamHandler handler) override;
    bool isOpen() const override;
    void close() override;

private:
    std::shared_ptr<boost::asio::ip::tcp::socket> socket;
};

#endif // TCPSTREAM_H

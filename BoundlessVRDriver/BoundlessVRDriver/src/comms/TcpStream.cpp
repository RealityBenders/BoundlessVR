#include "TcpStream.h"

TcpStream::TcpStream(std::shared_ptr<boost::asio::ip::tcp::socket> socket)
    : socket(std::move(socket)) {}

std::shared_ptr<boost::asio::ip::tcp::socket> TcpStream::getSocket() {
	return socket;
}

void TcpStream::asyncWrite(const uint8_t* buffer, std::size_t length, StreamHandler handler) {
    boost::asio::async_write(*socket, boost::asio::buffer(buffer, length), handler);
}

void TcpStream::asyncRead(uint8_t* buffer, std::size_t length, StreamHandler handler) {
    socket->async_read_some(boost::asio::buffer(buffer, length), handler);
}

bool TcpStream::isOpen() const {
    return socket->is_open();
}

void TcpStream::close() {
    if (socket->is_open()) {
        socket->close();
    }
}

#include "tcp_client_session.h"

TCPClientSession::TCPClientSession() : socket_(io_context_) {
}

TCPClientSession::~TCPClientSession() {
    close();
}

void TCPClientSession::connect(const std::string& host, const std::string& port) {
    boost::system::error_code ec;
    tcp::resolver resolver(io_context_);
    auto endpoints = resolver.resolve(host, port, ec);
    if (ec) {
        std::cerr << "TCP resolve error: " << ec.message() << "\n";
        throw boost::system::system_error(ec);
    }
    boost::asio::connect(socket_, endpoints, ec);
    if (ec) {
        std::cerr << "TCP connect error: " << ec.message() << "\n";
        throw boost::system::system_error(ec);
    }
    std::cout << "Successful TCP connection...\n";
    io_thread_ = std::thread(&TCPClientSession::run_io, this);
    start_reading();
}

void TCPClientSession::run_io() {
    io_context_.run();
}

void TCPClientSession::start_reading() {
    do_read();
}

void TCPClientSession::do_read() {
    boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(input_buffer_), "\n",
        [this](const boost::system::error_code& ec, size_t length) {
            if (!ec) {
                std::string line(input_buffer_.substr(0, length - 1));
                input_buffer_.erase(0, length);
                handle_read(line);
            } else {
                std::cerr << "Read error: " << ec.message() << "\n";
            }
        }
    );
}

void TCPClientSession::write_line(const std::string& line) {
    io_context_.post([this, line]() { do_write(line); });
}

void TCPClientSession::do_write(const std::string& line) {
    std::string data = line + "\r\n";
    boost::asio::async_write(socket_, boost::asio::buffer(data),
        [](const boost::system::error_code& ec, size_t) {
            if (ec) {
                std::cerr << "Write error: " << ec.message() << "\n";
            }
        }
    );
}

void TCPClientSession::handle_read(const std::string& message) {
    std::cout << "Received: " << message << "\n";
    do_read();
}

void TCPClientSession::close() {
    if (socket_.is_open()) {
        socket_.close();
    }
    io_context_.stop(); 
    if (io_thread_.joinable()) {
        io_thread_.join();
    }
}
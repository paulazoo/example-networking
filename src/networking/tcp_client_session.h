#ifndef TCP_CLIENT_SESSION_H
#define TCP_CLIENT_SESSION_H

#include "boost_header.h"
#include <string>
#include <iostream>
#include <cstdlib>
#include <thread>

class TCPClientSession {
public:
    TCPClientSession();
    virtual ~TCPClientSession();

    void connect(const std::string& host, const std::string& port);
    void write_line(const std::string& line);
    
    void start_reading();
    void close();

protected:
    virtual void handle_read(const std::string& message);

    void do_read();
    void do_write(const std::string& line);
    void run_io();

    net::io_context io_context_;
    boost::asio::ip::tcp::socket socket_;
    std::string input_buffer_;
    std::thread io_thread_;
};

#endif // TCP_CLIENT_SESSION_H

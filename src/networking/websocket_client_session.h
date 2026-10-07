#ifndef WEBSOCKET_CLIENT_SESSION_H
#define WEBSOCKET_CLIENT_SESSION_H

#include "boost_header.h"
#include "image_buffer.h"
#include <iostream>
#include <memory>
#include <string>
#include <thread>

constexpr size_t MAX_TEXT_MESSAGE_SIZE = 256;

class WebsocketClientSession : public std::enable_shared_from_this<WebsocketClientSession> { // Because the beast examples use beast::bind_front_handler
public:
    WebsocketClientSession();
    virtual ~WebsocketClientSession();

    void connect_ws(const std::string& host, const std::string& port);
    virtual void send_text_message(const std::string& message);
    void send_image_data(ImageBuffer& image_buffer);
    void close();

protected:
    void run_io_context();
    void do_read();
    void on_resolve(beast::error_code ec, tcp::resolver::results_type results);
    void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep);
    void on_handshake(beast::error_code ec);
    virtual void on_read(beast::error_code ec, size_t bytes_transferred);
    virtual void handle_read(const std::string& line);
    void fail(beast::error_code ec, char const* what);
    void process_write_queue();
    void send_binary_message(const std::vector<uint8_t>& data);

    std::deque<std::vector<uint8_t>> write_queue_;
    bool write_in_progress_;
    net::io_context io_context_;
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
    std::thread io_context_thread_;
    tcp::resolver resolver_;
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    std::string host_;
};

#endif // WEBSOCKET_CLIENT_SESSION_H

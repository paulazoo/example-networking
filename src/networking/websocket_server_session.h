#ifndef WEBSOCKET_SERVER_SESSION_H
#define WEBSOCKET_SERVER_SESSION_H

#include "timing.h"

#include "boost_header.h"

#include "config.h"

#include <iostream>
#include <deque>
#include <vector>
#include <mutex>
#include <atomic>
#include <memory>
#include <functional>

namespace net = boost::asio;
namespace beast = boost::beast;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;

constexpr size_t  MAX_TEXT_MESSAGE_SIZE = 256;

class WebsocketServerSession : public std::enable_shared_from_this<WebsocketServerSession> {
public:
    explicit WebsocketServerSession(
        tcp::socket socket, 
        std::function<void(const std::string& message)> broadcast_message_callback,
        net::thread_pool& thread_pool,  // Thread pool from server class
        int32_t client_id,
        std::function<void(int32_t, const std::string&)> send_to_engine_callback
    );
    virtual ~WebsocketServerSession();

    virtual void configure(Config& config);

    void start();
    void send_message(std::string message); // take value to move into queue later

protected:
    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    std::function<void(const std::string& message)> broadcast_message_callback_;
    std::deque<std::vector<uint8_t>> process_message_queue_;
    std::deque<int64_t> message_timestamps_queue_;
    std::mutex queue_mutex_;
    net::thread_pool& thread_pool_;  // Store reference

    // for engine
    int32_t client_id_{0};
    std::function<void(int32_t, const std::string&)> send_to_engine_callback_;

    // writing
    std::deque<std::shared_ptr<std::string>> outbox_;
    void do_write();

    void read_message();
    void process_message_thread();
    void call_broadcast_message_function(const std::string& message);
    virtual void process_text(const std::string& text_message, const int64_t& message_timestamp);
    virtual void process_image(std::vector<uint16_t>& pre_image_vector, const int64_t& message_timestamp);
};


#endif // WEBSOCKET_SERVER_SESSION_H

#ifndef WEBSOCKET_SERVER_H
#define WEBSOCKET_SERVER_H

#include "boost_header.h"
#include "websocket_server_session.h"
#include "fortyninety_server_session.h"
#include "trial_engine.h"

#include "config.h"

#include <iostream>
#include <set>
#include <mutex>
#include <memory>

class TrialEngine;

class WebsocketServer {
public:
    explicit WebsocketServer(net::io_context& io_context, const std::string& host, uint16_t port);
    ~WebsocketServer();

    void broadcast_message(const std::string& message);
    void attach_engine(std::shared_ptr<TrialEngine> engine);
    void send_to_engine(int32_t client_id, const std::string& msg);

    void set_server_config(Config& config);

private:
    tcp::acceptor acceptor_;
    std::set<std::shared_ptr<WebsocketServerSession>> sessions_;
    std::mutex session_mutex_;
    net::thread_pool thread_pool_{8};  // Shared thread pool across all sessions
    std::atomic<int32_t> next_client_id_{1};
    std::shared_ptr<TrialEngine> engine_;

    Config server_config_;

    void register_session(std::shared_ptr<WebsocketServerSession> session);
    void accept_connection();
};

#endif // WEBSOCKET_SERVER_H
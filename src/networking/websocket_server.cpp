#include "websocket_server.h"

WebsocketServer::WebsocketServer(net::io_context& io_context, const std::string& host, uint16_t port)
    : acceptor_(io_context, tcp::endpoint(net::ip::make_address(host), port)), 
      thread_pool_(8) {  // Initialize thread pool
    std::cout << "WebSocket Server listening on " << host << ":" << port << "\n";
    accept_connection();
}

WebsocketServer::~WebsocketServer() {
    std::lock_guard<std::mutex> lock(session_mutex_);
    sessions_.clear();
    std::cout << "WebSocket server shutting down. All sessions_ cleared.\n";
}

void WebsocketServer::set_server_config(Config& config) {
    server_config_ = config;
}

void WebsocketServer::broadcast_message(const std::string& message) {
    std::vector<std::shared_ptr<WebsocketServerSession>> copy;
    { std::lock_guard<std::mutex> lock(session_mutex_);
      copy.assign(sessions_.begin(), sessions_.end());
    }
    for (auto& s : copy) s->send_message(message);  // safe: each session serializes its own writes
}

void WebsocketServer::attach_engine(std::shared_ptr<TrialEngine> engine) {
    engine_ = std::move(engine);
}

void WebsocketServer::send_to_engine(int32_t client_id, const std::string& message) {
    auto e = engine_;
    if (e) {
        e->on_client_message(client_id, message);
    }
}

void WebsocketServer::register_session(std::shared_ptr<WebsocketServerSession> session) { // Polymorphism this is fine
    std::lock_guard<std::mutex> lock(session_mutex_);
    sessions_.insert(session);
}

void WebsocketServer::accept_connection() {
    // Create a socket bound to a strand so all ops for this session are serialized
    auto& ctx = static_cast<net::io_context&>(acceptor_.get_executor().context());
    auto socket = std::make_shared<tcp::socket>(net::make_strand(ctx));

    acceptor_.async_accept(*socket,
        [this, socket](beast::error_code ec) {
            if (!ec) {
                const int32_t client_id = next_client_id_.fetch_add(1);
                auto session = std::make_shared<FortyninetyServerSession>( // NOTE: HI FORTYNINETYSERVERSESSION USED HERE
                    std::move(*socket),
                    [this](const std::string& m) { this->broadcast_message(m); },
                    thread_pool_,
                    client_id,
                    [this](int32_t id, const std::string& m) { this->send_to_engine(id, m); }
                );
                register_session(session);
                session->configure(server_config_);
                session->start();
            } else {
                std::cerr << "Accept: " << ec.message() << "\n";
            }
            accept_connection();
        });
}

#include "websocket_client_session.h"

WebsocketClientSession::WebsocketClientSession()
    : resolver_(net::make_strand(io_context_)), ws_(net::make_strand(io_context_)), work_guard_(net::make_work_guard(io_context_)), write_in_progress_(false) { 
}

WebsocketClientSession::~WebsocketClientSession() {
    close();
}

void WebsocketClientSession::run_io_context() {
    try {
        std::cout << "Running I/O context..." << std::endl;
        io_context_.run();
        std::cout << "I/O context exited" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "I/O context error: " << e.what() << std::endl;
    }
}

void WebsocketClientSession::connect_ws(const std::string& host, const std::string& port) {
    host_ = host;
    std::cout << "Starting WS connection...\n";
    resolver_.async_resolve(host, port, beast::bind_front_handler(&WebsocketClientSession::on_resolve, shared_from_this()));
    io_context_thread_ = std::thread([this] { run_io_context(); });
}

void WebsocketClientSession::on_resolve(beast::error_code ec, tcp::resolver::results_type results) {
    if (ec) return fail(ec, "resolve");
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));
    beast::get_lowest_layer(ws_).async_connect(results,
                                               beast::bind_front_handler(&WebsocketClientSession::on_connect, shared_from_this()));
}

void WebsocketClientSession::on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type ep) {
    if (ec) return fail(ec, "connect");
    
    beast::get_lowest_layer(ws_).expires_never();
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::client));
    
    ws_.set_option(websocket::stream_base::decorator(
        [](websocket::request_type& req) {
            req.set(http::field::user_agent, std::string(BOOST_BEAST_VERSION_STRING) + " websocket-client-async");
        }));

    host_ += ':' + std::to_string(ep.port());
    ws_.async_handshake(host_, "/",
                        beast::bind_front_handler(&WebsocketClientSession::on_handshake, shared_from_this()));
}

void WebsocketClientSession::on_handshake(beast::error_code ec) {
    if (ec) return fail(ec, "handshake");
    std::cout << "Successful WS handshake!\n";
    do_read();
}

void WebsocketClientSession::on_read(beast::error_code ec, size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    if (ec) return fail(ec, "read");

    std::string line(static_cast<const char*>(buffer_.data().data()), buffer_.size());
    buffer_.consume(buffer_.size());
    handle_read(line);
}

void WebsocketClientSession::handle_read(const std::string& line) {
    std::cout << "Received: " << line << "\n";
    do_read();
}

void WebsocketClientSession::do_read() {
    ws_.async_read(buffer_,
                   beast::bind_front_handler(&WebsocketClientSession::on_read, shared_from_this()));
}

void WebsocketClientSession::close() {
    work_guard_.reset(); // Allow io_context to stop only when needed
    io_context_.stop();
    if (io_context_thread_.joinable()) {
        io_context_thread_.join();
    }
}

void WebsocketClientSession::send_text_message(const std::string& message) {
    if (!ws_.is_open()) {
        std::cerr << "WS is not open, cant send message\n";
        return;
    }

    if (message.size() > MAX_TEXT_MESSAGE_SIZE) {
        std::cerr << "Message too long, truncating to " << MAX_TEXT_MESSAGE_SIZE << " characters.\n";
    }

    std::vector<uint8_t> message_to_send(std::min(message.size(), MAX_TEXT_MESSAGE_SIZE));
    std::memcpy(message_to_send.data(), message.c_str(), std::min(message.size(), MAX_TEXT_MESSAGE_SIZE));

    ws_.binary(true);  // Mark binary to avoid utf8 issues
    send_binary_message(message_to_send);
}

void WebsocketClientSession::send_image_data(ImageBuffer& image_buffer) {
    if (!ws_.is_open()) {
        std::cerr << "WS is not open, cant send message\n";
        return;
    }
    uint16_t* image_start = image_buffer.get_start_pointer();
    unsigned int image_size = image_buffer.total_samples * sizeof(uint16_t);
    if (!image_start || image_size == 0) {
        std::cerr << "No image buffer available\n";
        return;
    }
    std::vector<uint8_t> message_to_send(image_size);
    std::memcpy(message_to_send.data(), reinterpret_cast<uint8_t*>(image_start), image_size);

    ws_.binary(true);  // Mark binary to avoid utf8 issues
    send_binary_message(message_to_send);
}

void WebsocketClientSession::fail(beast::error_code ec, char const* what) {
    std::cerr << "ws " << what << ": " << ec.message() << "\n";
}

void WebsocketClientSession::process_write_queue() {
    if (write_in_progress_ || write_queue_.empty()) return;  // Don't start new write if busy

    write_in_progress_ = true;
    auto& message = write_queue_.front();
    
    ws_.async_write(net::buffer(message),
        [self = shared_from_this()](beast::error_code ec, size_t bytes_transferred) {
            boost::ignore_unused(bytes_transferred);

            self->write_in_progress_ = false;
            if (ec) {
                std::cerr << "WS write error: " << ec.message() << "\n";
            } else {
                self->write_queue_.pop_front();
                self->process_write_queue(); // Go and double check for anything else to do, might be faster
            }
        });
}

void WebsocketClientSession::send_binary_message(const std::vector<uint8_t>& data) {
    net::post(io_context_, [self = shared_from_this(), data]() mutable {
        self->write_queue_.push_back(std::move(data));
        self->process_write_queue();
    });
}
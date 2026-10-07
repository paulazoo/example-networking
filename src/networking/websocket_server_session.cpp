#include "websocket_server_session.h"

WebsocketServerSession::WebsocketServerSession(
    tcp::socket socket, 
    std::function<void(const std::string& message)> broadcast_message_callback,
    net::thread_pool& thread_pool,
    int32_t client_id,
    std::function<void(int32_t, const std::string&)> send_to_engine_callback)
    : ws_(std::move(socket)), 
    broadcast_message_callback_(std::move(broadcast_message_callback)),
    thread_pool_(thread_pool),
    client_id_(client_id),
    send_to_engine_callback_(std::move(send_to_engine_callback)) {}

WebsocketServerSession::~WebsocketServerSession(){
}

void WebsocketServerSession::configure(Config& config) {
    // Default implementation does nothing. Derived classes can override.
}

void WebsocketServerSession::start() {
    ws_.async_accept(
        [self = shared_from_this()](beast::error_code ec) {
            if (!ec) {
                std::cout << "Client " << self->client_id_ << " connected\n";
                self->read_message();
            } else {
                std::cerr << "WebSocket client " << self->client_id_ << " accept error: " << ec.message() << "\n";
            }
        });
}

void WebsocketServerSession::send_message(std::string message) {
    // Ensure this runs on the websocket's executor (which will be a strand; see acceptor change below)
    auto self = shared_from_this();
    net::post(self->ws_.get_executor(),
              [self, msg = std::make_shared<std::string>(std::move(message))]() mutable {
                  const bool idle = self->outbox_.empty();
                  self->outbox_.push_back(std::move(msg));
                  if (idle) self->do_write(); // if not alr writing then start writing
              });
}

void WebsocketServerSession::do_write() { // beast’s default is text(true), so frames are sent as text
    auto self = shared_from_this();
    self->ws_.async_write(
        net::buffer(*self->outbox_.front()),
        [self](beast::error_code ec, size_t) {
            if (ec) {
                std::cerr << "WebSocket client " << self->client_id_ << " send error: " << ec.message() << "\n";
                self->outbox_.clear();
                return;
            }
            self->outbox_.pop_front();
            if (!self->outbox_.empty()) self->do_write(); // keep writing if more after pop_front
        });
}

void WebsocketServerSession::call_broadcast_message_function(const std::string& message) {
    if (broadcast_message_callback_) {
        broadcast_message_callback_(message);
    }
}

void WebsocketServerSession::read_message() { // Beast allows one read and one write concurrently
    auto self = shared_from_this();
    ws_.async_read(buffer_,
        [self](beast::error_code ec, size_t bytes_transferred) {
            if (!ec) {
                int64_t message_timestamp = Timing::get_epoch_time_now();
                auto b = self->buffer_.data();                        // const_buffers_type
                auto* data = static_cast<const uint8_t*>(b.data());   // const
                std::vector<uint8_t> message(data, data + bytes_transferred);
                self->buffer_.consume(bytes_transferred);
                {
                    std::lock_guard<std::mutex> lock(self->queue_mutex_);
                    self->process_message_queue_.push_back(std::move(message)); // bc message could be big
                    self->message_timestamps_queue_.push_back(std::move(message_timestamp));
                }
                
                net::post(self->thread_pool_, [self] {
                    self->process_message_thread();
                });

                self->read_message();
            } else {
                std::cerr << "WebSocket client " << self->client_id_ << " read error: " << ec.message() << "\n";
            }
        }
    );
}

void WebsocketServerSession::process_message_thread() {
    std::vector<uint8_t> message;
    int64_t message_timestamp;
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        if (process_message_queue_.empty() || message_timestamps_queue_.empty()) return; // Dont process if either queue is empty/not finished filling
        message = std::move(process_message_queue_.front());
        process_message_queue_.pop_front();
        message_timestamp = std::move(message_timestamps_queue_.front());
        message_timestamps_queue_.pop_front();
    }

    // message processing
    if (message.size() >= MAX_TEXT_MESSAGE_SIZE) { // treat as image if big (larger than 256 characters) regardless of op code
        // convert to uint16 vector
        std::vector<uint8_t> image_data(message.begin(), message.end());
        size_t num_pixels = image_data.size() / sizeof(uint16_t);
        const uint16_t* image_pixels = reinterpret_cast<const uint16_t*>(image_data.data());
        std::vector<uint16_t> pre_image_vector(image_pixels, image_pixels + num_pixels);
        process_image(pre_image_vector, message_timestamp);

    } else { // all other text messages
        std::string text_message(message.begin() + 0, message.end());
        process_text(text_message, message_timestamp);
    }
}

void WebsocketServerSession::process_text(const std::string& text_message, const int64_t& message_timestamp) {
    std::cout << "Client " << client_id_ << " text: " << text_message << " time: " << message_timestamp << "\n";
    if (send_to_engine_callback_) {
        send_to_engine_callback_(client_id_, text_message);
    } else {
        std::cout << "No send_to_engine_callback_ function\n";
    }
}


void WebsocketServerSession::process_image(std::vector<uint16_t>& pre_image_vector, const int64_t& message_timestamp) {
    std::cout << "Image from client " << client_id_ << "time: " << message_timestamp << "\n";
    // example image processing
    int32_t broadcast_signal = 60;
    call_broadcast_message_function(std::to_string(broadcast_signal));
    std::string text_message_from_image = "12";
    if (send_to_engine_callback_) send_to_engine_callback_(client_id_, text_message_from_image);
}
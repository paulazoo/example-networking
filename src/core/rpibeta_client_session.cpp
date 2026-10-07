#include "rpibeta_client_session.h"

RpibetaClientSession::RpibetaClientSession(int water_delivery_duration)
    : _water_delivery_duration(water_delivery_duration)
    , WebsocketClientSession() {

    if (gpioInitialise() < 0) {
        std::cerr << "Pigpio initialization failed!" << std::endl;
    } else {
        gpioWrite(2, 0);
    }
}

RpibetaClientSession::~RpibetaClientSession() {
    WebsocketClientSession::close();
}

void RpibetaClientSession::fortyninety_connect() {
    std::cout << "Connecting to fortyninety...\n";
    const std::string websocket_host = "10.10.100.100";
    const std::string websocket_port = "3141";
    WebsocketClientSession::connect_ws(websocket_host, websocket_port);
}

void RpibetaClientSession::handle_read(const std::string& line) {
    if (line[0] == 'r') {
        int reward_amount = line[1] - '0';  // '1' -> 1, '2' -> 2, etc.

        if (reward_amount == 0) {
            std::system("aplay ./tones/white_noise.wav");
        } else {
            for (int i = 0; i < reward_amount; ++i) {
                gpioWrite(2, 1);
                std::this_thread::sleep_for(std::chrono::milliseconds(_water_delivery_duration));
                gpioWrite(2, 0);

                // Wait between rewards, but not after the last one.
                if (i < reward_amount - 1) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(250));
                }
            }
        }

    } else if (line[0] == 't') {
        std::system("aplay ./tones/white_noise.wav");
    } else if (line[0] == 's') {
        std::system("aplay ./tones/tone0.wav");
    } else {
        // std::cout << "Received other signal: " << line << "\n";
    }
    WebsocketClientSession::do_read();
}

void RpibetaClientSession::send_text_message(const std::string& message) {
    if (!ws_.is_open()) {
        std::cerr << "WS is not open, cant send message\n";
        return;
    }

    std::vector<uint8_t> message_to_send(message.size(), 0);
    std::memcpy(message_to_send.data(), message.c_str(), message.size());

    ws_.binary(true);  // Mark binary to avoid utf8 issues
    send_binary_message(message_to_send);
}
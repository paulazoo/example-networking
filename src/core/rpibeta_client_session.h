#ifndef RPIBETA_CLIENT_SESSION_H
#define RPIBETA_CLIENT_SESSION_H

#include "websocket_client_session.h"

#include <pigpio.h>
#include <string>
#include <iostream>
#include <chrono>
#include <thread>
#include <cstdlib>
#include <random>
#include <mutex>
#include <memory>

class RpibetaClientSession : public WebsocketClientSession {
    public:
        RpibetaClientSession(int water_delivery_duration = 200);
        ~RpibetaClientSession() override;

        void send_text_message(const std::string& message) override;
        void set_random_run(bool setting);
        void fortyninety_connect();
    private:
        void handle_read(const std::string& line) override;
        int _water_delivery_duration;
};

#endif // RPIBETA_CLIENT_SESSION_H
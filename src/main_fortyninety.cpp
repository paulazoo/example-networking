#include "cpu_fft.h"
#include "json.hpp"

#include "websocket_server.h"
#include "trial_engine.h"

#include "config.h"

#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <algorithm>
#include <condition_variable>
#include <thread>
#include <filesystem>
#include <fstream>


int main() {
    Config stimtrain_config;
    const std::string config_path = "config.json";
    stimtrain_config = load_config_json(config_path);

    try {
        std::cout << stimtrain_config.task_name << "\n";
        std::string input;
        std::cout << "Press Enter to start server: \n";
        std::getline(std::cin, input);

        net::io_context io_context;
        WebsocketServer server(io_context, "10.10.100.100", 3141); // example server
        server.set_server_config(stimtrain_config);
        

        auto engine = std::make_shared<TrialEngine>(
            io_context,
            [&](const std::string& msg) { server.broadcast_message(msg); }
        );

        engine->configure(
            stimtrain_config.num_trials,
            stimtrain_config.p0_min_ms,
            stimtrain_config.p0_max_ms,
            stimtrain_config.p1_min_ms,
            stimtrain_config.p1_max_ms,
            stimtrain_config.p2_min_ms,
            stimtrain_config.p2_max_ms,
            stimtrain_config.p3_min_ms,
            stimtrain_config.p3_max_ms,
            stimtrain_config.initial_skip_p1,
            stimtrain_config.initial_skip_p2,
            stimtrain_config.initial_skip_p3,
            stimtrain_config.initial_reward_amount,
            stimtrain_config.require_lick_connection,
            stimtrain_config.require_lever_connection,
            stimtrain_config.require_led_connection,
            stimtrain_config.require_twophoton_connection,
            stimtrain_config.require_rpibeta_connection
        );
        server.attach_engine(engine);
        engine->start();

        io_context.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
#include "rpibeta_client_session.h"

#include <pigpio.h>
#include <iostream>
#include <string>

// Remember to run sudo killall pigpiod before running this program

void do_random_run(int water_delivery_duration = 200) {
    std::random_device rd;
    std::mt19937 gen = std::mt19937(rd());  // Seed generator anyway in constructor
    std::uniform_int_distribution<> random_distribution = std::uniform_int_distribution<>(2, 20);

    int random_duration = random_distribution(gen);
    std::chrono::steady_clock::time_point last_trial_end_time = std::chrono::steady_clock::now();

    if (gpioInitialise() < 0) {
        std::cerr << "Pigpio initialization failed!" << std::endl;
    } else {
        gpioWrite(2, 0);
    }

    for (size_t i = 0; i < 360000; i=i+1)  {
        auto now = std::chrono::steady_clock::now();
        if (now - last_trial_end_time > std::chrono::seconds(random_duration)) {
            gpioWrite(2, 1);
            std::this_thread::sleep_for(std::chrono::milliseconds(water_delivery_duration));
            gpioWrite(2, 0);
            random_duration = random_distribution(gen);
            last_trial_end_time = std::chrono::steady_clock::now();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    gpioTerminate(); 
}


void do_water_calibration(int water_delivery_duration = 200) {

    if (gpioInitialise() < 0) {
        std::cerr << "Pigpio initialization failed!" << std::endl;
    } else {
        gpioWrite(2, 0);
    }

    std::cout << "Running water calibration for " << water_delivery_duration << " ms...\n";

    for (size_t i = 0; i < 100; i=i+1)  {
        std::system("aplay ./tones/tone0.wav");
        gpioWrite(2, 1);
        std::this_thread::sleep_for(std::chrono::milliseconds(water_delivery_duration));
        gpioWrite(2, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    gpioTerminate(); 
}

int main() {
    std::string input;
    int water_delivery_duration;
    std::cout << "Enter (int) water delivery duration in ms (default 200): \n";
    std::getline(std::cin, input);
    if (!input.empty()) {
        try {
            water_delivery_duration = std::stoi(input);
        } catch (const std::invalid_argument&) {
            std::cerr << "Invalid input, using default value of 200ms\n";
            water_delivery_duration = 200;
        }
    } else {
        water_delivery_duration = 200; // default value
    }

    std::cout << "Water calibration? [y/n]: \n";
    std::getline(std::cin, input);
    if (input == "y") {
        do_water_calibration(water_delivery_duration);
        return 0;
    }
    std::cout << "Random run? [y/n]: \n";
    std::getline(std::cin, input);
    if (input == "y") {
        do_random_run(water_delivery_duration);
        return 0;
    }

    auto ws_session = std::make_shared<RpibetaClientSession>(water_delivery_duration);

    std::cout << "Press Enter to connect..\n";
    std::getline(std::cin, input);
    ws_session->fortyninety_connect();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    std::string b_message = "b"; // identify as rpibeta
    ws_session->send_text_message(b_message);

    for (size_t i = 0; i < 360000; i=i+1)  {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    gpioTerminate();
    std::cout << "Exiting main, waiting for io_context to stop...\n";
    ws_session->close();
    return 0;
}

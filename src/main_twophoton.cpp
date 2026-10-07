#include "websocket_client_session.h"
#include "prairieview_client_session.h"
#include "image_buffer.h"
// #include "image_saver.h" // windows.h must be included AFTER boost_headers.h d/t the boost library containing WinSock2.h

#include <functional>
#include <thread>
#include <chrono>


void run_request_raw_data(ImageBuffer& my_image_buffer, PrairieviewClientSession& prairieview_client, std::shared_ptr<WebsocketClientSession>& ws_session) {
    my_image_buffer.reset_if_filled();
    unsigned int number_newly_written_samples = prairieview_client.request_raw_data(my_image_buffer.next_pointer_uint, my_image_buffer.remaining_blank_samples);
    my_image_buffer.advance_buffer_offset(number_newly_written_samples);
    if (my_image_buffer.remaining_blank_samples == 0) {
        ws_session->send_image_data(my_image_buffer);
        std::cout << "Image sent\n";
    }
    return;
}

void prepare_prairieview(PrairieviewClientSession& prairieview_client) {
    prairieview_client.dont_wait_during_scan();
    prairieview_client.stop_streaming_raw_data();
    prairieview_client.start_limit_buffer_size(0);
    prairieview_client.start_streaming_raw_data(20);
    return;
}


int main() {
    std::string input;
    unsigned int samples_available, number_samples_written;
    uintptr_t next_pointer;
    ImageBuffer my_image_buffer;
    unsigned int buffer_size_to_set {65536}; // 250312 256x256 1 channel = 65536
    my_image_buffer.set_total_buffer_size(buffer_size_to_set); // 524288 samples = 1 512x512 frame with 2 channels; 262144 = 1 channel

    const std::string websocket_host = "10.10.100.100";
    const std::string  websocket_port = "3141";
    auto ws_session = std::make_shared<WebsocketClientSession>();
    ws_session->connect_ws(websocket_host, websocket_port);

    PrairieviewClientSession prairieview_client;
    prairieview_client.prairieview_connect();

    std::cout << "Press Enter if both WS and PrairieView connected successfully...\n";
    std::getline(std::cin, input);
    prepare_prairieview(prairieview_client);

    std::cout << "Press Enter to send opening message to fortyninety and start recording...\n";
    std::getline(std::cin, input);
    std::string ws_check_message = "w"; // identify as twophoton
    ws_session->send_text_message(ws_check_message);

    // Imaging
    for (size_t i = 0; i < 3600000; i=i+1)  {
        run_request_raw_data(my_image_buffer, prairieview_client, ws_session);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    std::cout << "Press Enter to close...\n";
    std::getline(std::cin, input);
    ws_session->close();
    prairieview_client.close();

    std::cout << "Press Enter to exit...\n";
    std::getline(std::cin, input);

    return 0;
}

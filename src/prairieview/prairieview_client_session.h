#ifndef PRAIRIEVIEW_CLIENT_SESSION_H
#define PRAIRIEVIEW_CLIENT_SESSION_H

#include "tcp_client_session.h"

#include <mutex>
#include <windows.h>

class PrairieviewClientSession : public TCPClientSession {
public:
    PrairieviewClientSession();
    ~PrairieviewClientSession();

    void prairieview_connect();

    void start_streaming_raw_data(unsigned short number_buffer_frames);
    void stop_streaming_raw_data();
    void start_limit_buffer_size(unsigned short buffer_size);
    void stop_limit_buffer_size();
    unsigned int get_samples_per_pixel();
    unsigned int get_pixels_per_line();
    unsigned int get_lines_per_frame();
    void stop_scan();
    void live_scan();
    void start_t_series();
    void dont_wait_during_scan();
    void no_wait();

    void move_relative_x_axis(double relative_position);
    void move_relative_y_axis(double relative_position);
    void move_relative_z_axis(double relative_position);
    void move_absolute_x_axis(double absolute_position);
    void move_absolute_y_axis(double absolute_position);
    void move_absolute_z_axis(double absolute_position);
    void set_channel(unsigned int channel, bool channel_bool);
    void set_laser_power(const std::string& laser_name, unsigned int laser_power);
    void set_utility_button(unsigned int button, bool button_bool);
    void single_scan_until_done();
    
    unsigned int request_raw_data(uintptr_t& next_pointer, unsigned int& samples_available);

private:
    void handle_read(const std::string& message) override;
    unsigned int string_to_uint(const std::string& str);
    std::string command_delimiter_;
    
    std::string last_reply_message_;
    std::mutex reply_message_mutex_;
    std::condition_variable reply_message_cv_;
    bool reply_message_received_;

    std::mutex reply_done_mutex_;
    std::condition_variable reply_done_cv_;
    bool reply_done_received_;
};

#endif // PRAIRIEVIEW_CLIENT_SESSION_H
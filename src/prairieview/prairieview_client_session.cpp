#include "prairieview_client_session.h"

PrairieviewClientSession::PrairieviewClientSession() : TCPClientSession() {
    command_delimiter_ = std::string("\1", 1);
}

PrairieviewClientSession::~PrairieviewClientSession() {
}

void PrairieviewClientSession::prairieview_connect() {
    std::cout << "Connecting to PrairieView...\n";
    const std::string host = "localhost";
    const std::string port = "1236";
    TCPClientSession::connect(host, port);
    std::cout << "Connected to PrairieView!\n";
}

void PrairieviewClientSession::handle_read(const std::string& message) {
    std::string cleaned_message = message;
    // Remove whitespace (spaces, \r, \n, etc)
    cleaned_message.erase(
        std::remove_if(cleaned_message.begin(), cleaned_message.end(), [](unsigned char c) {
            return std::isspace(c);
        }),
        cleaned_message.end()
    );
    if (message.compare(0, 3, "ACK") == 0) {
        // std::cout << "ack\n";
    } else if (message.compare(0, 4, "DONE") == 0) {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = true;
        reply_done_cv_.notify_one();  // Notify waiting thread thru reply_done_cv_; leaving this in the {} seems to be ok
        // notify_one() on a std::condition_variable when no thread is waiting does nothing and is ok
    } else {
        std::lock_guard<std::mutex> lock(reply_message_mutex_);
        last_reply_message_ = cleaned_message;
        reply_message_received_ = true;
        reply_message_cv_.notify_one();  // Notify waiting thread thru reply_message_cv_; leaving this in the {} seems to be ok
    }
    TCPClientSession::do_read();
}

void PrairieviewClientSession::start_streaming_raw_data(unsigned short number_buffer_frames) {
    std::ostringstream oss;
    oss << "-srd" << command_delimiter_ << "True" << command_delimiter_ << number_buffer_frames;
    std::string command = oss.str();
    TCPClientSession::write_line(command);
}

void PrairieviewClientSession::stop_streaming_raw_data() {
    std::ostringstream oss;
    oss << "-srd" << command_delimiter_ << "False";
    std::string command = oss.str();
    TCPClientSession::write_line(command);
}

void PrairieviewClientSession::start_limit_buffer_size(unsigned short buffer_size) {
    std::ostringstream oss;
    oss << "-lbs" << command_delimiter_ << "True" << command_delimiter_ << buffer_size;
    std::string command = oss.str();
    TCPClientSession::write_line(command);
}

void PrairieviewClientSession::stop_limit_buffer_size() {
    std::ostringstream oss;
    oss << "-lbs" << command_delimiter_ << "False";
    std::string command = oss.str();
    TCPClientSession::write_line(command);
}

unsigned int PrairieviewClientSession::get_samples_per_pixel() {
    std::ostringstream oss;
    oss << "-spp";
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_message_mutex_);
        reply_message_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_message_mutex_);
        reply_message_cv_.wait(lock, [this] { return reply_message_received_; }); // wait until reply_message_cv_ notification; note that mutex will be unlocked while waiting
    }
    return string_to_uint(last_reply_message_);
}

unsigned int PrairieviewClientSession::get_pixels_per_line() {
    std::ostringstream oss;
    oss << "-gts" << command_delimiter_ << "pixelsPerLine";
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_message_mutex_);
        reply_message_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_message_mutex_);
        reply_message_cv_.wait(lock, [this] { return reply_message_received_; });  // wait until reply_message_cv_ notification; note that mutex will be unlocked while waiting
    }
    return string_to_uint(last_reply_message_);
}

unsigned int PrairieviewClientSession::get_lines_per_frame() {
    std::ostringstream oss;
    oss << "-gts" << command_delimiter_ << "linesPerFrame";
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_message_mutex_);
        reply_message_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_message_mutex_);
        reply_message_cv_.wait(lock, [this] { return reply_message_received_; }); // wait until reply_message_cv_ notification; note that mutex will be unlocked while waiting
    }
    return string_to_uint(last_reply_message_);
}

void PrairieviewClientSession::stop_scan() {
    std::ostringstream oss;
    oss << "-stop";
    std::string command = oss.str();
    TCPClientSession::write_line(command); 
}

void PrairieviewClientSession::live_scan() {
    std::ostringstream oss;
    oss << "-lv";
    std::string command = oss.str();
    TCPClientSession::write_line(command); 
}

void PrairieviewClientSession::start_t_series() {
    std::ostringstream oss;
    oss << "-ts";
    std::string command = oss.str();
    TCPClientSession::write_line(command); 
}

void PrairieviewClientSession::dont_wait_during_scan() {
    std::ostringstream oss;
    oss << "-dw";
    std::string command = oss.str();
    TCPClientSession::write_line(command); 
}

void PrairieviewClientSession::no_wait() {
    std::ostringstream oss;
    oss << "-nw";
    std::string command = oss.str();
    TCPClientSession::write_line(command); 
}

unsigned int PrairieviewClientSession::request_raw_data(uintptr_t& next_pointer, unsigned int& samples_available) {
    DWORD pid = GetCurrentProcessId(); // From windows API
    std::ostringstream oss;
    oss << "-rrd" << command_delimiter_ << pid << command_delimiter_ << next_pointer << command_delimiter_ << samples_available;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_message_mutex_);
        reply_message_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_message_mutex_);
        reply_message_cv_.wait(lock, [this] { return reply_message_received_; }); // wait until reply_message_cv_ notification; note that mutex will be unlocked while waiting
    }
    return string_to_uint(last_reply_message_);
}

void PrairieviewClientSession::move_relative_x_axis(double relative_position) {
    std::ostringstream oss;
    oss << "-mr" << command_delimiter_ << "X" << command_delimiter_ << relative_position;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::move_relative_y_axis(double relative_position) {
    std::ostringstream oss;
    oss << "-mr" << command_delimiter_ << "Y" << command_delimiter_ << relative_position;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::move_relative_z_axis(double relative_position) {
    std::ostringstream oss;
    oss << "-mr" << command_delimiter_ << "Z" << command_delimiter_ << relative_position;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::move_absolute_x_axis(double absolute_position) {
    std::ostringstream oss;
    oss << "-ma" << command_delimiter_ << "X" << command_delimiter_ << absolute_position;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::move_absolute_y_axis(double absolute_position) {
    std::ostringstream oss;
    oss << "-ma" << command_delimiter_ << "Y" << command_delimiter_ << absolute_position;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::move_absolute_z_axis(double absolute_position) {
    std::ostringstream oss;
    oss << "-ma" << command_delimiter_ << "Z" << command_delimiter_ << absolute_position;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::set_channel(unsigned int channel, bool channel_bool) {
    std::ostringstream oss;
    std::string channel_setting {"Off"};
    if (channel_bool) channel_setting = "On";
    oss << "-c" << command_delimiter_ << channel << command_delimiter_ << channel_setting;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::set_laser_power(const std::string& laser_name, unsigned int laser_power) {
    std::ostringstream oss;
    oss << "-lp" << command_delimiter_ << laser_name << command_delimiter_ << laser_power;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::set_utility_button(unsigned int button, bool button_bool) {
    std::ostringstream oss;
    std::string button_setting {"false"};
    if (button_bool) button_setting = "true";
    oss << "-ub" << command_delimiter_ << button << command_delimiter_ << button_setting;
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}

void PrairieviewClientSession::single_scan_until_done() {
    std::ostringstream oss;
    oss << "-ss";
    std::string command = oss.str();
    {
        std::lock_guard<std::mutex> lock(reply_done_mutex_);
        reply_done_received_ = false;
    }
    TCPClientSession::write_line(command);
    {
        std::unique_lock<std::mutex> lock(reply_done_mutex_);
        reply_done_cv_.wait(lock, [this] { return reply_done_received_; }); // wait until reply_done_cv_ notification; note that mutex will be unlocked while waiting
    }
}
    
unsigned int PrairieviewClientSession::string_to_uint(const std::string& str) {
    unsigned int result = 0;
    for (char c : str) {
        result = result * 10 + (c - '0');
    }
    return result;
}
#ifndef FORTYNINETY_SERVER_SESSION_H
#define FORTYNINETY_SERVER_SESSION_H

#include <random>

#include "websocket_server_session.h"
#include "value_saver.h"
#include "image_saver.h"
#include "timing.h"
#include "cpu_fft.h"
#include "cpu_median_filter_256.h"
#include "calcium_response_processor.h"

#include "config.h"

using LetterValueTimestamp = std::tuple<int32_t, int32_t, int64_t>;

class FortyninetyServerSession : public WebsocketServerSession {
public:
    FortyninetyServerSession(
        tcp::socket socket,
        std::function<void(const std::string& message)> broadcast_message_callback,
        net::thread_pool& thread_pool,
        int32_t client_id,
        std::function<void(int32_t, const std::string&)> send_to_engine_callback
    );
    ~FortyninetyServerSession() override;

    void configure(Config& config) override;

    void record_values_batch(std::deque<LetterValueTimestamp>& values_batch);

private:
    bool process_calcium_response_{false};
    bool do_median_filter_{false};
    size_t image_height_{1};
    size_t image_width_{1};

    // recording
    std::mutex record_queue_mutex_;
    std::condition_variable record_value_queue_cv_;
    std::deque<LetterValueTimestamp> record_value_queue_;
    std::thread consumer_thread_;
    std::atomic<bool> actively_consuming_;
    std::string letters_filename_;
    std::string values_filename_;
    std::string timestamps_filename_;

    std::unique_ptr<CalciumResponseProcessor> fortyninety_calcium_response_processor_;  // for processing calcium response
    std::unique_ptr<CpuFFT> fortyninety_cpu_fft_;  // for fft image alignment
    std::unique_ptr<CpuMedianFilter256> fortyninety_cpu_median_filter_256_; // for image median filtering

    // image message processing
    std::atomic<int32_t> number_processed_images_;
    void process_image(std::vector<uint16_t>& data_vector, const int64_t& message_timestamp) override;
    
    // text message processing
    void process_text(const std::string& text_message, const int64_t& message_timestamp) override;
};

#endif // FORTYNINETY_SERVER_SESSION_H

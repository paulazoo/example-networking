#include "fortyninety_server_session.h"

FortyninetyServerSession::FortyninetyServerSession(
    tcp::socket socket, 
    std::function<void(const std::string& message)> broadcast_message_callback,
    net::thread_pool& thread_pool,
    int32_t client_id,
    std::function<void(int32_t, const std::string&)> send_to_engine_callback
    ) 
    : WebsocketServerSession(std::move(socket), std::move(broadcast_message_callback), thread_pool, client_id, std::move(send_to_engine_callback)),  // Call base constructor
    number_processed_images_(0), // But number_processed_images_ is specific to FortyninetyServerSession
    letters_filename_("../data/stimtrain_data/" + std::to_string(client_id) + "_letters.bin"),
    values_filename_("../data/stimtrain_data/" + std::to_string(client_id) + "_values.bin"),
    timestamps_filename_("../data/stimtrain_data/" + std::to_string(client_id) + "_timestamps.bin"),
    actively_consuming_(true) {

    consumer_thread_ = std::thread([this]() {
        std::deque<LetterValueTimestamp> local_batch_to_record_;
        while (actively_consuming_) {
            {
                std::unique_lock<std::mutex> lock(record_queue_mutex_);
                record_value_queue_cv_.wait(lock, [this] {
                    return (record_value_queue_.size() >= 10) || !actively_consuming_;
                });  // only do recording if stored equal or more than 10 values
                if (!actively_consuming_ && record_value_queue_.empty()) break; // if actively_consuming_ stopped before reaching 100 values

                // std::cout << "Got data to record of batch size: " << record_value_queue_.size() << "\n";
                record_value_queue_.swap(local_batch_to_record_); // Move all pending queue values to local batch
            }

            record_values_batch(local_batch_to_record_);
            local_batch_to_record_.clear(); // reset
        }
    });
}

FortyninetyServerSession::~FortyninetyServerSession() {
    actively_consuming_ = false;
    record_value_queue_cv_.notify_all();
    if (consumer_thread_.joinable()) {
        consumer_thread_.join();
    }
}

void FortyninetyServerSession::configure(Config& config) {
    process_calcium_response_ = config.process_calcium_response;
    do_median_filter_ = config.do_median_filter;
    image_height_ = config.image_height;
    image_width_ = config.image_width;

    if (process_calcium_response_) {
        fortyninety_calcium_response_processor_ = std::make_unique<CalciumResponseProcessor>(
            config.lower_calcium_threshold,
            config.upper_calcium_threshold,
            "../data/stimtrain_data/masks/target_indices_0.bin",
            "../data/stimtrain_data/masks/target_indices_1.bin"
        );
        fortyninety_cpu_fft_ = std::make_unique<CpuFFT>(
            config.image_height,
            config.image_width,
            "../data/stimtrain_data/reference_image.bin"
        );
    } else {
        fortyninety_calcium_response_processor_ = nullptr;
        fortyninety_cpu_fft_ = nullptr;
    }
    fortyninety_cpu_median_filter_256_ = std::make_unique<CpuMedianFilter256>();
    std::cout << "a Fortyninety server session was configured\n";
}

void FortyninetyServerSession::process_text(const std::string& text_message, const int64_t& message_timestamp) {
    // specific message filtering and processing
    if (text_message[0] == 'b' || text_message[0] == 'c' || text_message[0] == 'd' || text_message[0] == 'e' || text_message[0] == 'w') {
        if (send_to_engine_callback_) {
            send_to_engine_callback_(client_id_, text_message); // client_id_ is from base class
        }
    }

    // recording, can take your time now
    // std::cout << "Client: " << client_id_ << " text: " << text_message << " time: " << message_timestamp << "\n"; // client_id_ is from base class
    if (text_message[0] == 'f' || text_message[0] == 'g' || text_message[0] == 'h' || text_message[0] == 'i') {
        int32_t message_value = std::stoi(text_message.substr(1));
        int32_t letter = static_cast<unsigned char>(text_message[0]);
        {
            std::lock_guard<std::mutex> lock(record_queue_mutex_);
            record_value_queue_.emplace_back(letter, message_value, message_timestamp);
        }
        record_value_queue_cv_.notify_one();
    }
}

void FortyninetyServerSession::process_image(std::vector<uint16_t>& data_vector, const int64_t& message_timestamp) {
    int32_t image_number = number_processed_images_.fetch_add(1, std::memory_order_relaxed); // Claude says this is better
    int64_t timing_value = Timing::get_epoch_time_now();
    bool overwrite_binary = (image_number == 0);
    
    // super low value noise
    for (uint16_t& value : data_vector) {
        if (value > 20000) {
            value = 0;
        }
    }

    // median filter
    std::vector<uint16_t> image_vector(image_height_ * image_width_, 0);
    if (do_median_filter_) {
        fortyninety_cpu_median_filter_256_->median_filter(data_vector, image_vector);
    } else {
        image_vector = data_vector;
    }

    // processing image data
    if (process_calcium_response_) {
        fortyninety_cpu_fft_->fft_align_images(image_vector);
        auto [target0_df_f, target1_df_f] = fortyninety_calcium_response_processor_->calculate_calcium_response_signal(image_vector, image_number);
        int32_t target0_mapped = fortyninety_calcium_response_processor_->map_to_feedback(target0_df_f);
        int32_t target1_mapped = fortyninety_calcium_response_processor_->map_to_feedback(target1_df_f);

        // sending output mapped_threshold_value to trial_engine
        std::string to_trial_engine_message = "v0" + std::to_string(target0_mapped);
        send_to_engine_callback_(client_id_, to_trial_engine_message); // client_id_ is from base calss
        to_trial_engine_message = "v1" + std::to_string(target1_mapped);
        send_to_engine_callback_(client_id_, to_trial_engine_message); // client_id_ is from base calss

        std::cout << image_number << ": " << target0_mapped << " " << target1_mapped << "\n";

        // recording but for internally acquired data (hence client_id = 0)

        std::string task_filename = "../data/stimtrain_data/0_target0_values.bin";
        ValueSaver::save_int32_value_to_binary(task_filename, target0_df_f, overwrite_binary);
        task_filename = "../data/stimtrain_data/0_target1_values.bin";
        ValueSaver::save_int32_value_to_binary(task_filename, target1_df_f, overwrite_binary);
    }
    
    // recording but for images
    std::string filename = "../data/stimtrain_data/0_image_timestamps.bin";
    ValueSaver::save_int64_value_to_binary(filename, timing_value, overwrite_binary);
    filename = "../data/stimtrain_data/images/image_" + std::to_string(image_number) + ".bin";
    ImageSaver::save_image_to_binary(filename, image_vector, image_height_, image_width_);
}


void FortyninetyServerSession::record_values_batch(std::deque<LetterValueTimestamp>& values_batch) {
    // recording
    std::vector<int32_t> letters;
    std::vector<int32_t> values;
    std::vector<int64_t> timestamps;
    letters.reserve(values_batch.size());
    values.reserve(values_batch.size()); // performance ig
    timestamps.reserve(values_batch.size());
    for (const auto& [letter, value, timestamp] : values_batch) {
        letters.push_back(letter);
        values.push_back(value);
        timestamps.push_back(timestamp);
    }
    ValueSaver::save_multiple_int32_values_to_binary(letters_filename_, letters, false);
    ValueSaver::save_multiple_int32_values_to_binary(values_filename_, values, false);
    ValueSaver::save_multiple_int64_values_to_binary(timestamps_filename_, timestamps, false);
}

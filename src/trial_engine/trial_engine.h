#pragma once

#include "timing.h"
#include "value_saver.h"

#include "boost_header.h"

#include <atomic>
#include <cstdint>
#include <iostream>
#include <functional>
#include <string>
#include <chrono>
#include <vector>

class TrialEngine : public std::enable_shared_from_this<TrialEngine> {
public:
    using OutboundFcn = std::function<void(const std::string&)>;

    enum class Phase { not_started, p0, p1, p2, p3, done };

    TrialEngine(net::io_context& io, OutboundFcn outbound_callback);

    void configure(
        size_t settings_num_trials,
        int64_t settings_p0_min_ms,
        int64_t settings_p0_max_ms,
        int64_t settings_p1_min_ms,
        int64_t settings_p1_max_ms,
        int64_t settings_p2_min_ms,
        int64_t settings_p2_max_ms,
        int64_t settings_p3_min_ms,
        int64_t settings_p3_max_ms,
        bool settings_initial_skip_p1,
        bool settings_initial_skip_p2,
        bool settings_initial_skip_p3,
        size_t settings_initial_reward_amount,
        bool settings_require_lick_connection,
        bool settings_require_lever_connection,
        bool settings_require_led_connection,
        bool settings_require_twophoton_connection,
        bool settings_require_rpibeta_connection
    ); // sets ranges, skips, trial count
    void start();
    void stop();
    void stop_current_phase();
    void on_client_message(int32_t client_id, const std::string& msg);

private:
 // io
    net::io_context& io_;
    net::strand<net::io_context::executor_type> strand_;
    net::steady_timer phase_timer_;
    OutboundFcn outbound_callback_;

    // config
    size_t num_trials_{0};
    int64_t p0_min_{0}, p0_max_{0};
    int64_t p1_min_{0}, p1_max_{0};
    int64_t p2_min_{0}, p2_max_{0};
    int64_t p3_min_{0}, p3_max_{0};
    bool initial_skip_p1_{false};
    bool initial_skip_p2_{false};
    bool initial_skip_p3_{false};
    size_t initial_reward_amount_{0};
    bool require_lick_connection_{true};
    bool require_lever_connection_{true};
    bool require_led_connection_{true};
    bool require_twophoton_connection_{true};
    bool require_rpibeta_connection_{true};

    // other initial values
    bool connections_satisfied_{false};
    std::vector<bool> initial_trial_logic_;

    // connections
    bool b_connected_{false};
    bool c_connected_{false};
    bool d_connected_{false};
    bool e_connected_{false};
    bool w_connected_{false};
    
    // per-trial specific
    int64_t planned_phase0_ms_{0};
    int64_t planned_phase1_ms_{0};
    int64_t planned_phase2_ms_{0};
    int64_t planned_phase3_ms_{0};
    bool skip_phase1_{false};
    bool skip_phase2_{false};
    bool skip_phase3_{false};
    size_t reward_amount_{0};
    std::vector<bool> trial_logic_;

    // state
    size_t trial_idx_{0};
    Phase phase_{Phase::not_started};
    std::atomic<bool> stopped_{true};

    // recording trial_letters, trial_values, and trial_timestamps
    const std::string trial_letters_filename_{"../data/stimtrain_data/0_trial_letters.bin"};
    const std::string trial_values_filename_{"../data/stimtrain_data/0_trial_values.bin"};
    const std::string trial_timestamps_filename_{"../data/stimtrain_data/0_trial_timestamps.bin"};
    bool overwrite_binaries_{false};


    void broadcast_and_record(std::string message_broadcast);

    void reset_for_next_trial();

    void begin_phase0();
    void begin_phase1();
    void begin_phase2();
    void begin_phase3();
    void finish_trial();

    void schedule_phase_end_in(int64_t ms_delay,
                               Phase expected_phase,
                               std::function<void()> next_phase_fn);

    static int64_t uniform_ms(int64_t min, int64_t max);
   
};

#include "trial_engine.h"

TrialEngine::TrialEngine(net::io_context& io, OutboundFcn outbound_callback)
    : io_(io),
    strand_(net::make_strand(io_)),
    phase_timer_(strand_),
    outbound_callback_(std::move(outbound_callback)) {}

void TrialEngine::configure(
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
) {
    num_trials_ = settings_num_trials;
    p0_min_ = settings_p0_min_ms;
    p0_max_ = settings_p0_max_ms;
    p1_min_ = settings_p1_min_ms;
    p1_max_ = settings_p1_max_ms;
    p2_min_ = settings_p2_min_ms;
    p2_max_ = settings_p2_max_ms;
    p3_min_ = settings_p3_min_ms;
    p3_max_ = settings_p3_max_ms;
    initial_skip_p1_ = settings_initial_skip_p1;
    initial_skip_p2_ = settings_initial_skip_p2;
    initial_skip_p3_ = settings_initial_skip_p3;
    initial_reward_amount_ = settings_initial_reward_amount;
    require_lick_connection_ = settings_require_lick_connection;
    require_lever_connection_ = settings_require_lever_connection;
    require_led_connection_ = settings_require_led_connection;
    require_twophoton_connection_ = settings_require_twophoton_connection;
    require_rpibeta_connection_ = settings_require_rpibeta_connection;

    initial_trial_logic_.assign(10, false);
}

void TrialEngine::start() {
    stopped_.store(false);
    phase_ = Phase::not_started;
}

void TrialEngine::stop() {
    stopped_.store(true);
    std::string message_broadcast = "k";
    outbound_callback_(message_broadcast);
    net::post(strand_, [self = shared_from_this()] {
        self->phase_timer_.cancel();
        self->phase_ = Phase::done;
    });
}

void TrialEngine::reset_for_next_trial() {
    if (trial_idx_ >= num_trials_) { phase_ = Phase::done; return; }
    planned_phase0_ms_ = uniform_ms(p0_min_, p0_max_);
    planned_phase1_ms_ = uniform_ms(p1_min_, p1_max_);
    planned_phase2_ms_ = uniform_ms(p2_min_, p2_max_);
    planned_phase3_ms_ = uniform_ms(p3_min_, p3_max_);
    skip_phase1_ = initial_skip_p1_;
    skip_phase2_ = initial_skip_p2_;
    skip_phase3_ = initial_skip_p3_;
    reward_amount_ = initial_reward_amount_;
    trial_logic_ = initial_trial_logic_;
}

void TrialEngine::on_client_message(int32_t client_id, const std::string& msg) {
    net::post(strand_, [self = shared_from_this(), client_id, msg] {
        if (self->stopped_.load()) return;
        if (self->phase_ == Phase::p2) {
            // ==== REWARD LOGIC HERE =============
            if (msg[0] == 'v') {
                int msg_value = msg[2] - '0'; // integer ASCII code to integer
                if (msg[1] == '0') {
                    if (msg_value >= 4) {
                        self->trial_logic_[0] = true;
                    }
                    if (msg_value >= 9) {
                        self->trial_logic_[1] = true;
                    }
                }
                if (msg[1] == '1') {
                    if (msg_value >= 4) {
                        self->trial_logic_[2] = true;
                    }
                    if (msg_value >= 9) {
                        self->trial_logic_[3] = true;
                    }
                }
            }

            self->reward_amount_ = 0;
            for (size_t i = 0; i < 4; ++i) {
                self->reward_amount_ += self->trial_logic_[i];
            }
            // =====================================

        // Connecting to devices and starting session
        } else if (self->phase_ == Phase::not_started) {
            if (msg[0] == 'c') {
                std::cout << "LICK CONNECTED\n";
                self->c_connected_ = true;
            } else if (msg[0] == 'd') {
                std::cout << "LEVER CONNECTED\n";
                self->d_connected_ = true;
            } else if (msg[0] == 'e') {
                std::cout << "LED CONNECTED\n";
                self->e_connected_ = true;
            } else if (msg[0] == 'w') {
                std::cout << "TWOPHOTON CONNECTED\n";
                self->w_connected_ = true;
            } else if (msg[0] == 'b') {
                std::cout << "RPIBETA CONNECTED\n";
                self->b_connected_ = true;
            }

            self->connections_satisfied_ = true;
            if (self->require_lick_connection_ && !self->c_connected_) {
                self->connections_satisfied_ = false;
            } else if (self->require_lever_connection_ && !self->d_connected_) {
                self->connections_satisfied_ = false;
            } else if (self->require_led_connection_ && !self->e_connected_) {
                self->connections_satisfied_ = false;
            } else if (self->require_twophoton_connection_ && !self->w_connected_) {
                self->connections_satisfied_ = false;
            } else if (self->require_rpibeta_connection_ && !self->b_connected_) {
                self->connections_satisfied_ = false;
            }

            if (self->connections_satisfied_) {
                std::cout << "EVERYONE CONNECTED, STARTING SESSION\n";
                self->trial_idx_ = 0;
                self->reset_for_next_trial();   // ensure planned_phase*_ms_ are set for trial 0
                std::string message_broadcast = "a";
                self->broadcast_and_record(message_broadcast);
                self->begin_phase0();
            }
        }
        
    });
}


void TrialEngine::stop_current_phase() {
    if (stopped_.load()) return;
    switch (phase_) {
        case Phase::p0:
            phase_timer_.cancel();
            begin_phase1();
            break;
        case Phase::p1:
            phase_timer_.cancel();
            begin_phase2();
            break;
        case Phase::p2:
            phase_timer_.cancel();
            begin_phase3();
            break;
        case Phase::p3:
            phase_timer_.cancel();
            finish_trial();
            break;
        case Phase::done:
            break;
        case Phase::not_started:
            break;
    }
}

void TrialEngine::begin_phase0() { // ITI phase, never skip
    phase_ = Phase::p0;
    std::string message_broadcast = "l";
    broadcast_and_record(message_broadcast);
    
    schedule_phase_end_in(planned_phase0_ms_, Phase::p0, [self = shared_from_this()] {
        self->begin_phase1();
    });
}

void TrialEngine::begin_phase1() {
    if (skip_phase1_) {begin_phase2(); return;}
    phase_ = Phase::p1;
    std::string message_broadcast = "m";
    broadcast_and_record(message_broadcast);
    message_broadcast = "q"; // optogenetic stimulation
    broadcast_and_record(message_broadcast);
    // message_broadcast = "s"; // temp sound just to test rpibeta connection
    // broadcast_and_record(message_broadcast);

    schedule_phase_end_in(planned_phase1_ms_, Phase::p1, [self = shared_from_this()] {
        self->begin_phase2();
    });
}


void TrialEngine::begin_phase2() {
    if (skip_phase2_) {begin_phase3(); return;}
    phase_ = Phase::p2;
    std::string message_broadcast = "n";
    broadcast_and_record(message_broadcast);

    schedule_phase_end_in(planned_phase2_ms_, Phase::p2, [self = shared_from_this()] {
        self->begin_phase3();
    });
}

void TrialEngine::begin_phase3() {
    if (skip_phase3_) {finish_trial(); return;}
    phase_ = Phase::p3;
    std::string message_broadcast = "o";
    broadcast_and_record(message_broadcast);
    message_broadcast = "r" + std::to_string(reward_amount_); // reward or white noise
    broadcast_and_record(message_broadcast);

    schedule_phase_end_in(planned_phase3_ms_, Phase::p3, [self = shared_from_this()] {
        self->finish_trial();
    });
}

void TrialEngine::finish_trial() {
    std::string message_broadcast = "p";
    broadcast_and_record(message_broadcast);

    ++trial_idx_;
    std::cout << "TrialEngine finished trial: " << trial_idx_ << "\n";
    reset_for_next_trial();
    if (phase_ != Phase::done) begin_phase0();
}

void TrialEngine::schedule_phase_end_in(int64_t ms_delay, Phase expected_phase, std::function<void()> next_phase_fn) {
    phase_timer_.expires_after(std::chrono::milliseconds(ms_delay));
    phase_timer_.async_wait(net::bind_executor(
        strand_,
        [self = shared_from_this(), expected_phase, fn = std::move(next_phase_fn)]
        (const boost::system::error_code& ec) {
            if (ec) return;
            if (self->stopped_.load()) return;
            if (self->phase_ == Phase::done) return;
            if (self->phase_ != expected_phase) return;
            fn();
        }));
}

int64_t TrialEngine::uniform_ms(int64_t min, int64_t max) {
    if (min > max) std::swap(min, max);
    static thread_local std::mt19937 rng{std::random_device{}()};
    std::uniform_int_distribution<int64_t> dist(min, max);
    return dist(rng);
}


void TrialEngine::broadcast_and_record(std::string message_broadcast) {
    int64_t timestamp = Timing::get_epoch_time_now();
    int32_t message_value = (message_broadcast.size() > 1) ? std::stoi(message_broadcast.substr(1)) : 0;
    int32_t letter = static_cast<unsigned char>(message_broadcast[0]);
    ValueSaver::save_int64_value_to_binary(trial_timestamps_filename_, timestamp, overwrite_binaries_);
    ValueSaver::save_int32_value_to_binary(trial_letters_filename_, letter, overwrite_binaries_);
    ValueSaver::save_int32_value_to_binary(trial_values_filename_, message_value, overwrite_binaries_);
    std::cout << "TrialEngine saving broadcast: " << message_broadcast << " to: " << trial_values_filename_ << "\n";
    outbound_callback_(message_broadcast);
}
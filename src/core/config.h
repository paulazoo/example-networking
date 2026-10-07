#ifndef CONFIG_H
#define CONFIG_H

#include <cstdint>
#include <string>
#include <vector>

struct Config {
    std::string task_name{"helium"};

    size_t image_height{256};
    size_t image_width{256};
    bool process_calcium_response{false};
    bool do_median_filter{false};
    int32_t lower_calcium_threshold{0};
    int32_t upper_calcium_threshold{2000};

    size_t num_trials{50};
    int64_t p0_min_ms{10000};
    int64_t p0_max_ms{30000};
    int64_t p1_min_ms{750};
    int64_t p1_max_ms{750};
    int64_t p2_min_ms{2000};
    int64_t p2_max_ms{2000};
    int64_t p3_min_ms{3000};
    int64_t p3_max_ms{3000};
    bool initial_skip_p1{false};
    bool initial_skip_p2{false};
    bool initial_skip_p3{true};
    size_t initial_reward_amount{0};
    bool require_lick_connection{true};
    bool require_lever_connection{true};
    bool require_led_connection{true};
    bool require_twophoton_connection{true};
    bool require_rpibeta_connection{true};
};

Config load_config_json(const std::string& path);

#endif  // CONFIG_H
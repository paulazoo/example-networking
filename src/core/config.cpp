#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "config.h"
#include "json.hpp"

template <typename T> // T can be any type
void read_if_present(const nlohmann::json& json, const char* key, T& destination) {
    const auto it = json.find(key);
    if (it == json.end()) return;
    try {
        destination = it->get<T>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error("Invalid config value for '" + std::string(key) + "': " + e.what());
    }
}

Config load_config_json(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Failed to open config file: " + path);

    nlohmann::json j;
    try {
        in >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error(std::string("JSON parse error: ") + e.what());
    }

    if (!j.is_object()) throw std::runtime_error("Config JSON must be an object at the top level");

    Config c;

    // Missing keys keep defaults
    read_if_present(j, "task_name", c.task_name);
    read_if_present(j, "image_height", c.image_height);
    read_if_present(j, "image_width", c.image_width);
    read_if_present(j, "process_calcium_response", c.process_calcium_response);
    read_if_present(j, "do_median_filter", c.do_median_filter);
    read_if_present(j, "lower_calcium_threshold", c.lower_calcium_threshold);
    read_if_present(j, "upper_calcium_threshold", c.upper_calcium_threshold);
    read_if_present(j, "num_trials", c.num_trials);
    read_if_present(j, "p0_min_ms", c.p0_min_ms);
    read_if_present(j, "p0_max_ms", c.p0_max_ms);
    read_if_present(j, "p1_min_ms", c.p1_min_ms);
    read_if_present(j, "p1_max_ms", c.p1_max_ms);
    read_if_present(j, "p2_min_ms", c.p2_min_ms);
    read_if_present(j, "p2_max_ms", c.p2_max_ms);
    read_if_present(j, "p3_min_ms", c.p3_min_ms);
    read_if_present(j, "p3_max_ms", c.p3_max_ms);
    read_if_present(j, "initial_skip_p1", c.initial_skip_p1);
    read_if_present(j, "initial_skip_p2", c.initial_skip_p2);
    read_if_present(j, "initial_skip_p3", c.initial_skip_p3);
    read_if_present(j, "initial_reward_amount", c.initial_reward_amount);
    read_if_present(j, "require_lick_connection", c.require_lick_connection);
    read_if_present(j, "require_lever_connection", c.require_lever_connection);
    read_if_present(j, "require_led_connection", c.require_led_connection);
    read_if_present(j, "require_twophoton_connection", c.require_twophoton_connection);
    read_if_present(j, "require_rpibeta_connection", c.require_rpibeta_connection);

    if (c.task_name.empty()) throw std::runtime_error("task_name must not be empty");
    if (c.image_height == 0 || c.image_width == 0) {
        throw std::runtime_error("image_height and image_width must be greater than zero");
    }
    if (c.process_calcium_response && (c.image_height != 256 || c.image_width != 256)) {
        throw std::runtime_error("calcium response processing currently requires 256x256 images");
    }
    if (c.lower_calcium_threshold < 0 ||
        c.upper_calcium_threshold <= c.lower_calcium_threshold + 10) {
        throw std::runtime_error(
            "calcium thresholds must satisfy 0 <= lower_calcium_threshold and "
            "lower_calcium_threshold + 10 < upper_calcium_threshold");
    }

    return c;
}
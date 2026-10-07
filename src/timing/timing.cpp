#include "timing.h"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include "json.hpp"

using json = nlohmann::json;

namespace Timing {
    int64_t get_epoch_time_now() {
        auto now = std::chrono::system_clock::now();
        auto duration = now.time_since_epoch();
        return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
    }

    void write_timestamp_json(const std::string& data_filename, const std::string& json_filename) {
        double timestamp = get_epoch_time_now();

        json j;
        j["data_name"] = data_filename;
        j["time"] = timestamp;

        std::ofstream file(json_filename);
        file << std::setw(4) << j << std::endl;  // pretty print 4 spaces
    }
}
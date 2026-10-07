#ifndef TIMING_H
#define TIMING_H

#include <string>

namespace Timing {
    int64_t get_epoch_time_now();
    void write_timestamp_json(const std::string& data_filename, const std::string& json_filename);
};

#endif  // TIMING_H

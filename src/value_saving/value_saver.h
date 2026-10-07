#ifndef VALUE_SAVER_H
#define VALUE_SAVER_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>

namespace ValueSaver {
    void save_int32_value_to_binary(const std::string& filename, const int32_t& value, bool overwrite_binary);
    void save_int64_value_to_binary(const std::string& filename, const int64_t& value, bool overwrite_binary);
    void save_multiple_int32_values_to_binary(const std::string& filename, const std::vector<int32_t>& values_vector, bool overwrite_binary);
    void save_multiple_int64_values_to_binary(const std::string& filename, const std::vector<int64_t>& values_vector, bool overwrite_binary);
}
#endif // VALUE_SAVER_H

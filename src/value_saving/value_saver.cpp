#include "value_saver.h"

namespace ValueSaver {
    void save_int32_value_to_binary(const std::string& filename, const int32_t& value, bool overwrite_binary) {
        std::ofstream out_file;
        if (overwrite_binary) {
            out_file.open(filename, std::ios::binary);
        } else {
            out_file.open(filename, std::ios::binary | std::ios::app);
        }
        if (!out_file) {
            std::cerr << "Failed to open file for writing"<< "\n";
            return;
        }
        int32_t fourbyte_value = static_cast<int32_t>(value);
        out_file.write(reinterpret_cast<char*>(&fourbyte_value), sizeof(fourbyte_value));
        out_file.close();
    }

    void save_int64_value_to_binary(const std::string& filename, const int64_t& value, bool overwrite_binary) {
        std::ofstream out_file;
        if (overwrite_binary) {
            out_file.open(filename, std::ios::binary);
        } else {
            out_file.open(filename, std::ios::binary | std::ios::app);
        }
        if (!out_file) {
            std::cerr << "Failed to open file for writing"<< "\n";
            return;
        }
        int64_t eightbyte_value = static_cast<int64_t>(value);
        out_file.write(reinterpret_cast<char*>(&eightbyte_value), sizeof(eightbyte_value));
        out_file.close();
    }
    

    void save_multiple_int32_values_to_binary(const std::string& filename, const std::vector<int32_t>& values_vector, bool overwrite_binary) {
        // input vector needs to be already int32_t
        std::ofstream out_file;
        if (overwrite_binary) {
            out_file.open(filename, std::ios::binary);
        } else {
            out_file.open(filename, std::ios::binary | std::ios::app);
        }
        if (!out_file) {
            std::cerr << "Failed to open file for writing"<< "\n";
            return;
        }
        out_file.write(reinterpret_cast<const char*>(values_vector.data()), values_vector.size() * sizeof(int32_t));
        out_file.close();
    }
    
    void save_multiple_int64_values_to_binary(const std::string& filename, const std::vector<int64_t>& values_vector, bool overwrite_binary) {
        // input vector needs to be already int64_t
        std::ofstream out_file;
        if (overwrite_binary) {
            out_file.open(filename, std::ios::binary);
        } else {
            out_file.open(filename, std::ios::binary | std::ios::app);
        }
        if (!out_file) {
            std::cerr << "Failed to open file for writing"<< "\n";
            return;
        }
        out_file.write(reinterpret_cast<const char*>(values_vector.data()), values_vector.size() * sizeof(int64_t));
        out_file.close();
    }

}
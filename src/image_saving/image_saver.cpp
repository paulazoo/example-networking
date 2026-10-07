#include "image_saver.h"

namespace ImageSaver {
    void save_image_vector_to_png_16bit(const std::string& filename, const std::vector<uint16_t>& image_vector, size_t height, size_t width) {
        if (image_vector.empty()) {
            std::cerr << "Image vector empty\n";
            return;
        }
        if (stbi_write_png(filename.c_str(), width, height, 1, image_vector.data(), width * sizeof(uint16_t)) == 0) {
            std::cerr << "Failed to write image to " << filename << "\n";
        } else {
            std::cout << "Uint16 png saved to " << filename << "\n";
        }
    }

    void save_image_to_binary(const std::string& filename, const std::vector<uint16_t>& image_vector, size_t height, size_t width) {
        std::ofstream out_file(filename, std::ios::binary);
        if (!out_file) {
            std::cerr << "Failed to open file for writing"<< "\n";
            return;
        }
        int32_t height_int32 = static_cast<int32_t>(height);
        int32_t width_int32 = static_cast<int32_t>(width);
        out_file.write(reinterpret_cast<char*>(&height_int32), sizeof(height_int32));
        out_file.write(reinterpret_cast<char*>(&width_int32), sizeof(width_int32));
        out_file.write(reinterpret_cast<const char*>(image_vector.data()), image_vector.size() * sizeof(uint16_t));
        out_file.close();
    }

    std::tuple<size_t, size_t, std::vector<uint16_t>> read_image_from_binary(const std::string& filename) {
        std::ifstream in_file(filename, std::ios::binary);
        if (!in_file) {
            std::cerr << "Failed to open file for reading"<< "\n";
            return {0, 0, {}};
        }
        int32_t height_int32, width_int32;
        in_file.read(reinterpret_cast<char*>(&height_int32), sizeof(height_int32));
        in_file.read(reinterpret_cast<char*>(&width_int32), sizeof(width_int32));
        size_t height = static_cast<size_t>(height_int32);
        size_t width = static_cast<size_t>(width_int32);
        size_t num_pixels = height * width;
        std::vector<uint16_t> image_vector(num_pixels);
        in_file.read(reinterpret_cast<char*>(image_vector.data()), num_pixels * sizeof(uint16_t));
        in_file.close();
        return {height, width, std::move(image_vector)};
    }
    
    std::vector<size_t> read_indices_from_binary(const std::string& filename) {
        std::ifstream in_file(filename, std::ios::binary | std::ios::ate);
        if (!in_file) {
            std::cerr << "Failed to open file for reading\n";
            return {};
        }

        std::streamsize file_size = in_file.tellg();
        in_file.seekg(0, std::ios::beg);

        if (file_size % sizeof(uint64_t) != 0) {
            std::cerr << "File size is not a multiple of uint64_t\n";
            return {};
        }

        size_t num_elements = file_size / sizeof(uint64_t);
        std::vector<uint64_t> temp_vector(num_elements);

        if (!in_file.read(reinterpret_cast<char*>(temp_vector.data()), file_size)) {
            std::cerr << "Failed to read file data\n";
            return {};
        }

        // Convert uint64_t vector to size_t vector
        std::vector<size_t> indices_vector(temp_vector.begin(), temp_vector.end());
        return indices_vector;
    }
}
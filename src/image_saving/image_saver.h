#ifndef IMAGE_SAVER_H
#define IMAGE_SAVER_H

#include "stb_image_write.h"

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <cstdint>

namespace ImageSaver {
    void save_image_vector_to_png_16bit(const std::string& filename, const std::vector<uint16_t>& image_vector, size_t height, size_t width);
    void save_image_to_binary(const std::string& filename, const std::vector<uint16_t>& image_vector, size_t height, size_t width);
    std::tuple<size_t, size_t, std::vector<uint16_t>> read_image_from_binary(const std::string& filename);
    std::vector<size_t> read_indices_from_binary(const std::string& filename);
}
#endif // IMAGE_SAVER_H

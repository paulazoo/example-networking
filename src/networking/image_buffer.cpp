#include "image_buffer.h"

ImageBuffer::ImageBuffer() 
    : buffer_offset_(0), total_samples(0), next_pointer(nullptr), remaining_blank_samples(0) { }

uint16_t* ImageBuffer::get_start_pointer() {
    return buffer_data.data();
}

void ImageBuffer::set_total_buffer_size(unsigned int& buffer_size_to_set) {
    buffer_data.resize(buffer_size_to_set);
    total_samples = buffer_size_to_set;
    next_pointer = buffer_data.data();
    next_pointer_uint = reinterpret_cast<uintptr_t>(next_pointer);
    remaining_blank_samples = total_samples - buffer_offset_;
}

void ImageBuffer::reset_buffer() {
    buffer_offset_ = 0;
    std::fill(buffer_data.begin(), buffer_data.end(), 0);
    next_pointer = buffer_data.data();
    next_pointer_uint = reinterpret_cast<uintptr_t>(next_pointer);
    remaining_blank_samples = total_samples;
}

void ImageBuffer::reset_if_filled() {
    if (buffer_offset_ == total_samples) {
        reset_buffer();
    }
}

void ImageBuffer::advance_buffer_offset(unsigned int& number_samples_written) {
    buffer_offset_ = std::min(buffer_offset_ + number_samples_written, total_samples);
    next_pointer = buffer_data.data() + buffer_offset_;
    next_pointer_uint = reinterpret_cast<uintptr_t>(next_pointer);
    remaining_blank_samples = total_samples - buffer_offset_;
}

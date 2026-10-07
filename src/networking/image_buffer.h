#ifndef IMAGE_BUFFER_H
#define IMAGE_BUFFER_H

#include <vector>
#include <algorithm>
#include <iostream>
#include <cstdint>

class ImageBuffer {
public:
    ImageBuffer();
    uint16_t* get_start_pointer();
    void set_total_buffer_size(unsigned int& buffer_size_to_set);
    void advance_buffer_offset(unsigned int& number_samples_written);
    void reset_if_filled();
    void reset_buffer();
    
    std::vector<uint16_t> buffer_data;

    unsigned int total_samples;
    uint16_t* next_pointer;
    uintptr_t next_pointer_uint;
    unsigned int remaining_blank_samples;

private:
    unsigned int buffer_offset_;
};

#endif // IMAGE_BUFFER_H

#ifndef CPU_MEDIAN_FILTER_256_H
#define CPU_MEDIAN_FILTER_256_H


#include <vector>
#include <algorithm>
#include <cstdint>
#include <omp.h>
#include <immintrin.h>  // AVX stuff


class CpuMedianFilter256 {
public:
    CpuMedianFilter256();

    void median_filter(const std::vector<uint16_t>& image_vector, std::vector<uint16_t>& output);


private:
    static constexpr int kernel_size_ = 3; // Smallest kernel
    static constexpr int radius_ = kernel_size_ / 2;
    static constexpr int hist_size_ = 65536; // uint16 range
    static constexpr int median_index_ = (kernel_size_ * kernel_size_) / 2;
    static constexpr int image_height_ = 256;
    static constexpr int image_width_ = 256;


    int avx2_horizontal_add(__m256i v);
    uint16_t get_median_simd(const int* histogram);
};


#endif // CPU_MEDIAN_FILTER_256_H
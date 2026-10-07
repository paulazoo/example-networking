#include "cpu_median_filter_256.h"

CpuMedianFilter256::CpuMedianFilter256() {
}

int CpuMedianFilter256::avx2_horizontal_add(__m256i v) { // AVX2 version of _mm512_reduce_add_epi32
    __m128i sum128 = _mm_add_epi32(_mm256_extracti128_si256(v, 1), _mm256_castsi256_si128(v));
    sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(2, 3, 0, 1)));
    sum128 = _mm_add_epi32(sum128, _mm_shuffle_epi32(sum128, _MM_SHUFFLE(1, 0, 3, 2)));
    return _mm_cvtsi128_si32(sum128);
}


uint16_t CpuMedianFilter256::get_median_simd(const int* histogram) {
    int count = 0;
    int i = 0;


#if defined(__AVX2__)  // Intel i7-13700
    __m256i sum_vec = _mm256_setzero_si256(); // initialize all to 0
    for (; i < hist_size_; i += 8) {
        __m256i hist_vec = _mm256_loadu_si256(reinterpret_cast<const __m256i*>(&histogram[i])); // load each value into AVX2 register
        count += avx2_horizontal_add(hist_vec); // horizontal sum
        if (count > median_index_) {
            return i; // median must be at i
        }
    }
#endif
    //backup
    while (count <= median_index_ && i < hist_size_) {
        count += histogram[i++];
    }
    return i - 1;
}

// NOTE: output vector must be preallocated to the same size as input vector
void CpuMedianFilter256::median_filter(const std::vector<uint16_t>& image_vector, std::vector<uint16_t>& output) {
    alignas(64) int histogram[hist_size_] = {0};

    for (int y = 0; y < image_height_; ++y) {
        std::fill(std::begin(histogram), std::end(histogram), 0);
        for (int i = -radius_; i <= radius_; ++i) {
            int row = std::clamp(y + i, 0, image_height_ - 1);
            for (int j = 0; j < kernel_size_; ++j) {
                int col = std::clamp(j, 0, image_width_ - 1);
                histogram[image_vector[row * image_width_ + col]]++;
            }
        }


        output[y * image_width_] = get_median_simd(histogram);


        for (int x = 1; x < image_width_; ++x) {
            for (int i = -radius_; i <= radius_; ++i) {
                int row = std::clamp(y + i, 0, image_height_ - 1);
                int col_out = std::clamp(x - radius_ - 1, 0, image_width_ - 1);
                int col_in = std::clamp(x + radius_, 0, image_width_ - 1);

                histogram[image_vector[row * image_width_ + col_out]]--;
                histogram[image_vector[row * image_width_ + col_in]]++;
            }
            output[y * image_width_ + x] = get_median_simd(histogram);
        }
    }
}



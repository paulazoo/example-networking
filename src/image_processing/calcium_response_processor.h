#ifndef CALCIUM_RESPONSE_PROCESSOR_H
#define CALCIUM_RESPONSE_PROCESSOR_H


#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstdint>
#include <functional>
#include <iostream>
#include <numeric>
#include <string>
#include <tuple>
#include <vector>

#include "image_saver.h"

class CalciumResponseProcessor {
public:
    CalciumResponseProcessor(
        int32_t lower_threshold,
        int32_t upper_threshold,
        std::string target0_mask_filename,
        std::string target1_mask_filename
    );

    void initialize_calcium_response_processor(
        int32_t lower_threshold,
        int32_t upper_threshold
    );

    std::tuple<int32_t, int32_t> calculate_calcium_response_signal(
        const std::vector<uint16_t>& image_vector,
        const int32_t& image_number
    );

    int32_t map_to_feedback(
        const int32_t& calcium_response_signal
    );

private:
    std::vector<std::atomic<int32_t>> target0_f_baseline_;
    std::vector<std::atomic<int32_t>> target1_f_baseline_;
    std::string target0_mask_filename_;
    std::string target1_mask_filename_;
    std::vector<size_t> target0_roi_mask_;
    std::vector<size_t> target1_roi_mask_;
    std::vector<int32_t> response_thresholds_;

    int32_t get_roi_mask_value(
        const std::vector<size_t>& mask_vector,
        const std::vector<uint16_t>& image_vector
    );

    int32_t calculate_df_f(
        const int32_t value,
        const std::vector<std::atomic<int32_t>>& baseline_array,
        const int32_t& image_number
    );
};

#endif // CALCIUM_RESPONSE_PROCESSOR_H
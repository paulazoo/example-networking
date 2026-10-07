#include "calcium_response_processor.h"

CalciumResponseProcessor::CalciumResponseProcessor(
    int32_t lower_threshold,
    int32_t upper_threshold,
    std::string target0_mask_filename,
    std::string target1_mask_filename)
    : target0_f_baseline_(50000),  // can increase more if you want, just watch your RAM; consider ring buffer in future
      target1_f_baseline_(50000),
      target0_mask_filename_(target0_mask_filename),
      target1_mask_filename_(target1_mask_filename) {

        // read in the mask indices files to get the roi masks
        target0_roi_mask_ = ImageSaver::read_indices_from_binary(target0_mask_filename_);
        target1_roi_mask_ = ImageSaver::read_indices_from_binary(target1_mask_filename_);

        // lower and upper threshold to all 9 thresholds
        response_thresholds_.resize(9);
        if ((upper_threshold - 9) <= lower_threshold) {
            std::cerr << "No increment possible\n";
            return;
        }

        int32_t max_decrement = static_cast<int32_t>(
            std::floor(
                static_cast<double>(upper_threshold - lower_threshold) / 9.0
            )
        );

        for (size_t i {0}; i < 9; i++) {
            response_thresholds_[8 - i] = upper_threshold - (i * max_decrement);
            std::cout
                << "THRESHOLD "
                << 8 - i
                << " set as: "
                << response_thresholds_[8 - i]
                << "\n";
        }

      }

int32_t CalciumResponseProcessor::get_roi_mask_value(
    const std::vector<size_t>& mask_vector,
    const std::vector<uint16_t>& image_vector
) {
    size_t mask_size = mask_vector.size();
    std::vector<uint16_t> values(mask_size);

    for (size_t i = 0; i < mask_size; ++i) {
        // if mask_vector[i] < image_vector.size() return 1;
        values[i] = image_vector[mask_vector[i]];
    }

    size_t top_percentage_start = static_cast<size_t>(
        std::round(values.size() * 0.8)
    );

    // if (top_percentage_start == 0) return 1;
    std::nth_element(
        values.begin(),
        values.begin() + top_percentage_start,
        values.end(),
        std::greater<>()
    );

    return static_cast<int32_t>(
        std::round(
            static_cast<double>(
                std::accumulate(
                    values.begin(),
                    values.begin() + top_percentage_start,
                    0
                )
            ) / static_cast<double>(top_percentage_start)
        )
    );
}

int32_t CalciumResponseProcessor::calculate_df_f(
    const int32_t value,
    const std::vector<std::atomic<int32_t>>& baseline_array,
    const int32_t& image_number
) {
    // if (image_number <= 100) return 1;
    std::array<int32_t, 100> f_baseline_past;

    for (size_t i = 0; i < 100; ++i) {
        f_baseline_past[i] =
            baseline_array[image_number - (i + 1)].load(
                std::memory_order_acquire
            );
    }

    std::nth_element(
        f_baseline_past.begin(),
        f_baseline_past.begin() + 9,
        f_baseline_past.end()
    );

    int32_t baseline = f_baseline_past[9]; // 10th percentile

    if (baseline > 0) {
        int32_t diff = value - baseline;
        int32_t result = std::round(
            static_cast<double>(diff * 1000) /
            static_cast<double>(baseline)
        );
        return result;
    }

    return value; // if baseline is 0 somehow, return the original value as a default value
}

std::tuple<int32_t, int32_t>
CalciumResponseProcessor::calculate_calcium_response_signal(
    const std::vector<uint16_t>& image_vector,
    const int32_t& image_number
) {
    int32_t target0_value =
        get_roi_mask_value(target0_roi_mask_, image_vector);

    int32_t target1_value =
        get_roi_mask_value(target1_roi_mask_, image_vector);
    
    // if frame was shuttered, use previous value; note noise is at 50
    if (target0_value < 50) {
        int32_t previous_value;
        if (image_number >= 1) {
            previous_value = target0_f_baseline_[image_number - 1].load(std::memory_order_acquire);
        } else {
            previous_value = 50;
        }
        target0_f_baseline_[image_number].store(previous_value, std::memory_order_release);
    } else {
        target0_f_baseline_[image_number].store(target0_value, std::memory_order_release);
    }
    if (target1_value < 50) {
        int32_t previous_value;
        if (image_number >= 1) {
            previous_value = target1_f_baseline_[image_number - 1].load(std::memory_order_acquire);
        } else {
            previous_value = 50;
        }
        target1_f_baseline_[image_number].store(previous_value, std::memory_order_release);
    } else {
        target1_f_baseline_[image_number].store(target1_value, std::memory_order_release);
    }

    if (image_number <= 100) return {0, 0};

    int32_t target0_df_f =
        calculate_df_f(
            target0_value,
            target0_f_baseline_,
            image_number
        );

    int32_t target1_df_f =
        calculate_df_f(
            target1_value,
            target1_f_baseline_,
            image_number
        );

    return {target0_df_f, target1_df_f};
}

int32_t CalciumResponseProcessor::map_to_feedback(
    const int32_t& calcium_response_signal
) {
    // O(logN) binary search is faster for ONLY 10 comparisons than linear O(N) search
    // If calcium_response_signal is less than the highest threshold, map to the appropriate lower index
    // If calcium_response_signal is equal to or greater than the highest threshold, it == response_thresholds.end() and map to the last valid index (9).
    auto it = std::upper_bound(
        response_thresholds_.begin(),
        response_thresholds_.end(),
        calcium_response_signal
    );

    int32_t index = std::distance(response_thresholds_.begin(), it);

    return std::max(0, std::min(index, 9));
}
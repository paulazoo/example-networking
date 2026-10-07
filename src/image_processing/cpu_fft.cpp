#include "cpu_fft.h"

CpuFFT::CpuFFT(size_t image_height, size_t image_width, const std::string& reference_image_filename)
: image_height_(image_height),
  image_width_(image_width) {

    // initialize hanning window
    hanning_window_.resize(image_height_ * image_width_);
    // Hanning window from Claude
    for (int i = 0; i < image_height_; ++i) {
        for (int j = 0; j < image_width_; ++j) {
            double hanning_y = 0.5 * (1 - cos(2 * M_PI * i / static_cast<double>(image_height_ - 1)));
            double hanning_x = 0.5 * (1 - cos(2 * M_PI * j / static_cast<double>(image_width_ - 1)));
            hanning_window_[(i * image_width_) + j] = hanning_x * hanning_y;
        }
    }

    // initialize reference image
    auto read_image_tuple_result = ImageSaver::read_image_from_binary(reference_image_filename);
    if (image_height_ != std::get<0>(read_image_tuple_result)) {
        throw std::runtime_error("Reference image height does not match expected image_height_.");
    };
    if (image_width_ != std::get<1>(read_image_tuple_result)) {
        throw std::runtime_error("Reference image width does not match expected image_width_.");
    };
    reference_image_ = std::get<2>(read_image_tuple_result);

}


void CpuFFT::compute_cross_correlation(fftw_complex* in1, fftw_complex* in2, fftw_complex* out) {
    #pragma omp parallel for
    for (size_t i = 0; i < image_height_; ++i) { // WARNING: each image_height_ loop gets separate thread, so ideally run this before second message arrives
        for (size_t j = 0; j < image_width_; ++j) {
            size_t index = i * image_width_ + j;
            in2[index][1] = -in2[index][1]; // Conjugate second image
            out[index][0] = in1[index][0] * in2[index][0] - in1[index][1] * in2[index][1]; // Real correlation
            out[index][1] = in1[index][0] * in2[index][1] + in1[index][1] * in2[index][0]; // Imaginary correlation
        }
    }
}


// Function to shift the image based on calculated translation from Claude
void CpuFFT::shift_image(std::vector<uint16_t>& image, int shift_x, int shift_y) {
    std::vector<uint16_t> shifted_image(image_width_ * image_height_, 0); // Zero-filled


    for (size_t i = 0; i < image_height_; ++i) {
        for (size_t j = 0; j < image_width_; ++j) {
            int new_x = j + shift_x;
            int new_y = i + shift_y;


            // Ensure the new indices are within bounds
            if (new_x >= 0 && new_x < image_width_ && new_y >= 0 && new_y < image_height_) {
                shifted_image[new_y * image_width_ + new_x] = image[i * image_width_ + j];
            }
        }
    }


    // Copy back to the original image
    image = std::move(shifted_image);
}


void CpuFFT::fft_align_images(std::vector<uint16_t>& input_image) {
    fftw_complex *fftw_template = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * image_width_ * image_height_);
    fftw_complex *fftw_input = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * image_width_ * image_height_);
    fftw_complex *fftw_output = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * image_width_ * image_height_);
    for (int i = 0; i < image_height_; ++i) {
        for (int j = 0; j < image_width_; ++j) {
            fftw_template[i * image_width_ + j][0] = static_cast<double>(reference_image_[i * image_width_ + j]); // fftw3 only takes double
            fftw_input[i * image_width_ + j][0] = static_cast<double>(input_image[i * image_width_ + j]); // fftw3 only takes double
            fftw_template[i * image_width_ + j][1] = 0.0; // no imaginary
            fftw_input[i * image_width_ + j][1] = 0.0; // no imaginary
        }
    }


    // ChatGPT says hanning window will help with edge artifacts
    for (int i = 0; i < image_height_ * image_width_; ++i) {
        fftw_template[i][0] *= hanning_window_[i];
        fftw_template[i][1] *= hanning_window_[i];
        fftw_input[i][0] *= hanning_window_[i];
        fftw_input[i][1] *= hanning_window_[i];
    }


    fftw_plan plan_template = fftw_plan_dft_2d(image_height_, image_width_, fftw_template, fftw_template, FFTW_FORWARD, FFTW_ESTIMATE); // FFTW_ESIMATE fastest if no wisdoms
    fftw_plan plan_input = fftw_plan_dft_2d(image_height_, image_width_, fftw_input, fftw_input, FFTW_FORWARD, FFTW_ESTIMATE);
    fftw_plan plan_inverse = fftw_plan_dft_2d(image_height_, image_width_, fftw_output, fftw_output, FFTW_BACKWARD, FFTW_ESTIMATE);
    fftw_execute(plan_template);
    fftw_execute(plan_input);
    compute_cross_correlation(fftw_template, fftw_input, fftw_output);
    fftw_execute(plan_inverse);


    int max_index = 0;
    double max_so_far = 0.0;
    for (int i = 0; i < image_height_; ++i) {
        for (int j = 0; j < image_width_; ++j) {
            int index = i * image_width_ + j;
            double magnitude_squared = fftw_output[index][0] * fftw_output[index][0] +
                                    fftw_output[index][1] * fftw_output[index][1];
            if (magnitude_squared > max_so_far) {
                max_so_far = magnitude_squared;
                max_index = index;
            }
        }
    }
    int max_y = max_index / image_width_;
    int max_x = max_index % image_width_;


    // Adjust for wrap-around due to FFT shift
    if (max_x > image_width_ / 2) max_x -= image_width_;
    if (max_y > image_height_ / 2) max_y -= image_height_;


    // std::cout << "peak: " << max_x << " " << max_y << "\n";


    shift_image(input_image, max_x, max_y);


    fftw_destroy_plan(plan_template);
    fftw_destroy_plan(plan_input);
    fftw_destroy_plan(plan_inverse);
    fftw_free(fftw_template);
    fftw_free(fftw_input);
    fftw_free(fftw_output);
}
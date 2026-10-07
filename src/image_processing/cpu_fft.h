#ifndef CPU_FFT_H
#define CPU_FFT_H

#include <vector>
#include <cstdint>
#include <stdexcept>
#include <cmath>
#include <fftw3.h>
#include <omp.h>

#include "image_saver.h"

class CpuFFT {
public:
    CpuFFT(size_t image_height, size_t image_width, const std::string& reference_image_filename);

    void fft_align_images(std::vector<uint16_t>& input_image);

private:
    const size_t image_height_;
    const size_t image_width_;
    std::vector<double> hanning_window_;
    std::vector<uint16_t> reference_image_;

    void initialize_hanning_window();
    void initialize_reference_image();
    void compute_cross_correlation(fftw_complex* in1, fftw_complex* in2, fftw_complex* out);

    // Function to shift the image based on calculated translation from Claude
    void shift_image(std::vector<uint16_t>& image, int shiftX, int shiftY);
};

#endif // CPU_FFT_H
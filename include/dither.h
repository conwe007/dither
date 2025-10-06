#ifndef __DITHER_H
#define __DITHER_H

#include "convolve.h" // convolve
#include "error_diffusion.h" // for error diffusion constants
#include "grayscale.h" // channel_value()
#include "image.h" // data structure for colors
#include "lodepng.h" // for loading input pngs and saving to output pngs
#include "palette.h" // datastructure for colors
#include <vector> // std::vector

class Dither
{
public:
    Dither(std::size_t frames, bool gamma_correction);
    std::vector<std::vector<int>> get_matrix();
    void set_palette(Palette palette);
    std::size_t load(const char* file_name);
    std::size_t save(const char* file_name);
    void grayscale(GrayscaleMethod method);
    void reduce();
    void error_diffusion(ErrorDiffusionAlgorithm algorithm, bool alternate);
    void ordered(std::vector<std::vector<int>> threshold_matrix);
    void convolution(Kernel kernel_type, EdgeHandling edge_handling);
    void temporal();

private:
    Image image;
    Palette palette;
    bool gamma_correction;

    void error_diffusion_standard(ErrorDiffusionAlgorithm algorithm);
    void error_diffusion_alternate(ErrorDiffusionAlgorithm algorithm);
    std::vector<std::vector<double>> normalize_threshold_matrix(std::vector<std::vector<int>> threshold_matrix);
    std::vector<std::vector<int>> scale_threshold_matrix(std::vector<std::vector<int>> threshold_matrix, int min, int max);
};

#endif

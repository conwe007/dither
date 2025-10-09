#include "bayer.h"
#include "benchmark.h"
#include "cli.h"
#include "convolve.h"
#include "dither.h"
#include "error_diffusion.h"
#include "fourier2d.h"
#include "grayscale.h"
#include "noise2d.h"
#include "ordered.h"
#include "palette.h"
#include <cmath>
#include <format>
#include <iostream>
#include <numbers>
#include <string>
#include <unordered_map>
#include <vector>

std::vector<std::vector<int>> load_matrix_from_png(std::string file_name);
template <typename T>
std::vector<std::vector<T>> create_matrix_from_noise(Noise2D<T> noise, std::size_t width, std::size_t height);

std::string error_diffusion(std::string file_name, Palette palette, ErrorDiffusionAlgorithm algorithm, bool gamma_correction, bool alternate, bool benchmark);
std::string error_diffusion_all(std::string file_name, Palette palette, bool gamma_correction, bool benchmark);

std::string ordered(std::string file_name, Palette palette, ThresholdMatrix threshold_matrix_type, bool gamma_correction, bool benchmark);
std::string ordered_all(std::string file_name, Palette palette, bool gamma_correction, bool benchmark);

std::string convolution(std::string file_name, Kernel kernel_type, EdgeHandling edge_handling, bool gamma_correction, bool benchmark);
std::string convolution_all(std::string file_name, EdgeHandling edge_handling, bool gamma_correction, bool benchmark);

std::string temporal(std::string file_name, std::string method, Palette palette, std::size_t frames, bool gamma_correction, bool benchmark);
std::string temporal_all(std::string file_name, std::string method, Palette palette, bool gamma_correction, bool benchmark);

std::string convolve_dither(std::string file_name, Kernel kernel_type, EdgeHandling edge_handling, Palette palette, ErrorDiffusionAlgorithm algorithm, bool gamma_correction, bool alternate, bool fourier, bool benchmark);
std::string convolve_dither_all(std::string file_name, EdgeHandling edge_handling, Palette palette, ErrorDiffusionAlgorithm algorithm, bool gamma_correction, bool alternate, bool fourier, bool benchmark);

std::string generate_bayer(int size, int output_levels, bool fourier, bool benchmark);
std::string generate_bayer_all(int output_levels, bool fourier, bool benchmark);

std::string generate_blue_noise(int width, int height, double sigma, int output_levels, bool fourier, bool benchmark);
std::string generate_blue_noise_all(double sigma, int output_levels, bool fourier, bool benchmark);
std::string generate_brown_noise(int width, int height, double leaky_integrator, std::size_t kernel_size, double sigma, int output_levels, bool fourier, bool benchmark);
std::string generate_brown_noise_all(double leaky_integrator, std::size_t kernel_size, double sigma, int output_levels, bool fourier, bool benchmark);
std::string generate_white_noise(int width, int height, int output_levels, bool fourier, bool benchmark);
std::string generate_white_noise_all(int output_levels, bool fourier, bool benchmark);

int main(int argc, const char* argv[])
{
    double gamma = 2.2;

    std::string file_path_threshold_matrix = "output\\threshold_matrix\\";
    Palette palette_black_white = Palette("BLACK_WHITE", Palette::preset_palettes.at(PresetPalette::BLACK_WHITE), gamma);
    Palette palette_1bit_monitor_glow = Palette("_1BIT_MONITOR_GLOW", Palette::preset_palettes.at(PresetPalette::_1BIT_MONITOR_GLOW), gamma);
    Palette palette_titanstone = Palette("TITANSTONE", Palette::preset_palettes.at(PresetPalette::TITANSTONE), gamma);
    Palette palette_2bit_demichrome = Palette("_2BIT_DEMICHROME", Palette::preset_palettes.at(PresetPalette::_2BIT_DEMICHROME), gamma);
    Palette palette_twilight5 = Palette("TWILIGHT5", Palette::preset_palettes.at(PresetPalette::TWILIGHT5), gamma);

    int output_levels = Color::CHANNEL_MAX + 1;
    double sigma_blue_noise = 1.9;
    double leaky_integrator = 1.0;
    size_t kernel_size = 3;
    double sigma_brown_noise = 1.0;
    
    // std::cout << generate_bayer_all(output_levels, true, true) << std::endl;
    // std::cout << generate_blue_noise_all(sigma_blue_noise, output_levels, true, true) << std::endl;
    // std::cout << generate_brown_noise_all(leaky_integrator, kernel_size, sigma_brown_noise, output_levels, true, true) << std::endl;
    // std::cout << generate_white_noise_all(output_levels, true, true) << std::endl;

    // std::cout << error_diffusion_all("golden_gate", palette_black_white, true, true) << std::endl;
    // std::cout << ordered_all("golden_gate", palette_black_white, true, true) << std::endl;
    // std::cout << convolution_all("golden_gate", EdgeHandling::EXTEND, true, true) << std::endl;
    // std::cout << convolve_dither_all("golden_gate", EdgeHandling::EXTEND, palette_black_white, ErrorDiffusionAlgorithm::ATKINSON, true, false, true, true) << std::endl;
    std::cout << temporal_all("sphere", "RANDOM", palette_twilight5, true, true);

    std::cout << "finished" << std::endl;
    return 0;
}

std::vector<std::vector<int>> load_matrix_from_png(std::string file_name)
{
    Image image = Image();
    image.load(file_name.c_str());
    size_t height = image.get_height();
    size_t width = image.get_width();
    std::vector<std::vector<int>> matrix = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            Color pixel = image.get_pixel(x, y);
            int16_t channel_value = Grayscale::channel_value(pixel, GrayscaleMethod::BT709);
            matrix[y][x] = channel_value;
        }
    }

    return matrix;
}

template <typename T>
std::vector<std::vector<T>> create_matrix_from_noise(Noise2D<T> noise, std::size_t width, std::size_t height)
{
    std::vector<std::vector<T>> matrix = std::vector<std::vector<T>>(height, std::vector<T>(width, static_cast<T>(0)));

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            matrix[y][x] = noise.get_noise_at(x, y);
        }
    }

    return matrix;
}

std::string error_diffusion(std::string file_name, Palette palette, ErrorDiffusionAlgorithm algorithm, bool gamma_correction, bool alternate, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    std::string file_path_input = "input\\" + file_name + ".png";
    std::string file_path_output = "output\\error_diffusion\\" + file_name;
    std::string file_path_suffix = "";
    Dither dither = Dither(1, gamma_correction);

    dither.set_palette(palette);
    dither.load(file_path_input.c_str());

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%s %s time: ", ErrorDiffusion::ALGORITHM_STRING.at(algorithm).c_str(), alternate ? "alternate" : "standard");
        output += heading;
        bm.start();
    }

    dither.error_diffusion(algorithm, alternate);

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    dither.save((file_path_output + "_" + ErrorDiffusion::ALGORITHM_STRING.at(algorithm) + (alternate ? "_alternate" : "_standard") + ".png").c_str());

    return output;
}

std::string error_diffusion_all(std::string file_name, Palette palette, bool gamma_correction, bool benchmark)
{
    std::string output = "Error Diffusion:\n";

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::LINEAR, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::LINEAR, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::FLOYD_STEINBERG, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::FLOYD_STEINBERG, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::JARVICE_JUDICE_NINKE, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::STUCKI, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::STUCKI, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::ATKINSON, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::ATKINSON, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::BURKES, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::BURKES, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_TWO_ROW, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_TWO_ROW, gamma_correction, true, benchmark);

    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_LITE, gamma_correction, false, benchmark);
    output += error_diffusion(file_name, palette, ErrorDiffusionAlgorithm::SIERRA_LITE, gamma_correction, true, benchmark);

    return output;
}

std::string ordered(std::string file_name, Palette palette, ThresholdMatrix threshold_matrix_type, bool gamma_correction, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    std::string file_path_threshold = "output\\threshold_matrix\\";
    std::string file_path_input = "input\\" + file_name + ".png";
    std::string file_path_output = "output\\ordered\\" + file_name;
    std::string file_path_suffix = "";
    std::vector<std::vector<int>> threshold_matrix = load_matrix_from_png(file_path_threshold + THRESHOLD_MATRIX_STRING.at(threshold_matrix_type) + ".png");
    Dither dither = Dither(1, gamma_correction);

    dither.set_palette(palette);
    dither.load(file_path_input.c_str());

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%s time: ", THRESHOLD_MATRIX_STRING.at(threshold_matrix_type).c_str());
        output += heading;
        bm.start();
    }

    dither.ordered(threshold_matrix);

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    dither.save((file_path_output + "_" + THRESHOLD_MATRIX_STRING.at(threshold_matrix_type) + ".png").c_str());

    return output;
}

std::string ordered_all(std::string file_name, Palette palette, bool gamma_correction, bool benchmark)
{
    std::string output = "Ordered:\n";

    output += ordered(file_name, palette, ThresholdMatrix::BAYER_2X2, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BAYER_4X4, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BAYER_8X8, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BAYER_16X16, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BAYER_32X32, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BAYER_64X64, gamma_correction, benchmark);

    output += ordered(file_name, palette, ThresholdMatrix::BLUE_NOISE_2X2, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BLUE_NOISE_4X4, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BLUE_NOISE_8X8, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BLUE_NOISE_16X16, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BLUE_NOISE_32X32, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BLUE_NOISE_64X64, gamma_correction, benchmark);

    output += ordered(file_name, palette, ThresholdMatrix::BROWN_NOISE_2X2, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BROWN_NOISE_4X4, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BROWN_NOISE_8X8, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BROWN_NOISE_16X16, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BROWN_NOISE_32X32, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::BROWN_NOISE_64X64, gamma_correction, benchmark);

    output += ordered(file_name, palette, ThresholdMatrix::WHITE_NOISE_2X2, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::WHITE_NOISE_4X4, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::WHITE_NOISE_8X8, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::WHITE_NOISE_16X16, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::WHITE_NOISE_32X32, gamma_correction, benchmark);
    output += ordered(file_name, palette, ThresholdMatrix::WHITE_NOISE_64X64, gamma_correction, benchmark);
    
    return output;
}

std::string convolution(std::string file_name, Kernel kernel_type, EdgeHandling edge_handling, bool gamma_correction, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    std::string file_path_input = "input\\" + file_name + ".png";
    std::string file_path_output = "output\\convolution\\" + file_name;
    std::string file_path_suffix = "";
    Dither dither = Dither(1, gamma_correction);

    dither.load(file_path_input.c_str());

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%s time: ", KERNEL_STRING.at(kernel_type).c_str());
        output += heading;
        bm.start();
    }

    dither.convolution(kernel_type, edge_handling);

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    dither.save((file_path_output + "_" + KERNEL_STRING.at(kernel_type) + ".png").c_str());

    return output;
}

std::string convolution_all(std::string file_name, EdgeHandling edge_handling, bool gamma_correction, bool benchmark)
{
    std::string output = "Convolution:\n";

    output += convolution(file_name, Kernel::RIDGE_4, edge_handling, gamma_correction, benchmark);
    output += convolution(file_name, Kernel::RIDGE_8, edge_handling, gamma_correction, benchmark);
    output += convolution(file_name, Kernel::SHARPEN_4, edge_handling, gamma_correction, benchmark);
    output += convolution(file_name, Kernel::SHARPEN_8, edge_handling, gamma_correction, benchmark);
    output += convolution(file_name, Kernel::BOX_BLUR, edge_handling, gamma_correction, benchmark);
    output += convolution(file_name, Kernel::GAUSSIAN_BLUR, edge_handling, gamma_correction, benchmark);
    output += convolution(file_name, Kernel::UNSHARP_MASK, edge_handling, gamma_correction, benchmark);
    
    return output;
}

std::string temporal(std::string file_name, std::string method, Palette palette, std::size_t frames, bool gamma_correction, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    std::string file_path_input = "input\\" + file_name + ".png";
    std::string file_path_output = "output\\temporal\\" + file_name;
    std::string file_path_suffix = "";
    Dither dither = Dither(frames, gamma_correction);

    dither.set_palette(palette);
    dither.load(file_path_input.c_str());

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "frames[%llu] time: ", frames);
        output += heading;
        bm.start();
    }

    dither.temporal(method);

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    dither.save((file_path_output + "_" + std::to_string(frames) + ".gif").c_str());

    return output;
}

std::string temporal_all(std::string file_name, std::string method, Palette palette, bool gamma_correction, bool benchmark)
{
    std::string output = "Temporal:\n";

    output += temporal(file_name, method, palette, 2, gamma_correction, benchmark);
    output += temporal(file_name, method, palette, 4, gamma_correction, benchmark);
    output += temporal(file_name, method, palette, 8, gamma_correction, benchmark);
    output += temporal(file_name, method, palette, 16, gamma_correction, benchmark);

    return output;
}

std::string convolve_dither(std::string file_name, Kernel kernel_type, EdgeHandling edge_handling, Palette palette, ErrorDiffusionAlgorithm algorithm, bool gamma_correction, bool alternate, bool fourier, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    std::string file_path_suffix = ".png";
    std::string file_path_input = "input\\" + file_name + file_path_suffix;
    std::string file_path_output = "output\\convolve_dither\\" + file_name;
    Dither dither = Dither(1, gamma_correction);
    std::size_t output_levels = Color::CHANNEL_MAX + 1;

    dither.set_palette(palette);
    dither.load(file_path_input.c_str());

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%s time: ", KERNEL_STRING.at(kernel_type).c_str());
        output += heading;
        bm.start();
    }

    dither.convolution(kernel_type, edge_handling);
    dither.error_diffusion(algorithm, alternate);

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    dither.save((file_path_output + "_" + KERNEL_STRING.at(kernel_type) + "_" + ErrorDiffusion::ALGORITHM_STRING.at(algorithm) + file_path_suffix).c_str());

    if(fourier)
    {
        Image image = Image();

        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%s fourier time: ", KERNEL_STRING.at(kernel_type).c_str());
            output += heading;
            bm.start();
        }

        Fourier2D<int> fourier_2d = Fourier2D<int>(dither.get_matrix(), true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            bm.stop();
            output += std::to_string(bm.time_us()) + " us\n";;
        }

        char file_name[1000];
        sprintf(file_name, "%s_%s_fourier.png", file_path_output.c_str(), KERNEL_STRING.at(kernel_type).c_str());
        image.create_from_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string convolve_dither_all(std::string file_name, EdgeHandling edge_handling, Palette palette, ErrorDiffusionAlgorithm algorithm, bool gamma_correction, bool alternate, bool fourier, bool benchmark)
{
    std::string output = "Convolve Dither:\n";

    output += convolve_dither(file_name, Kernel::RIDGE_4, edge_handling, palette, algorithm, gamma_correction, alternate, fourier, benchmark);
    output += convolve_dither(file_name, Kernel::RIDGE_8, edge_handling, palette, algorithm, gamma_correction, alternate, fourier, benchmark);
    output += convolve_dither(file_name, Kernel::SHARPEN_4, edge_handling, palette, algorithm, gamma_correction, alternate, fourier, benchmark);
    output += convolve_dither(file_name, Kernel::SHARPEN_8, edge_handling, palette, algorithm, gamma_correction, alternate, fourier, benchmark);
    output += convolve_dither(file_name, Kernel::BOX_BLUR, edge_handling, palette, algorithm, gamma_correction, alternate, fourier, benchmark);
    output += convolve_dither(file_name, Kernel::GAUSSIAN_BLUR, edge_handling, palette, algorithm, gamma_correction, alternate, fourier, benchmark);
    output += convolve_dither(file_name, Kernel::UNSHARP_MASK, edge_handling, palette, algorithm, gamma_correction, alternate, fourier, benchmark);
    
    return output;
}

std::string generate_bayer(int size, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\bayer_%ix%i.png", size, size);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", size, size);
        output += heading;
        bm.start();
    }

    Bayer bayer = Bayer(size, output_levels);
    bayer.generate_bayer_matrix();

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    image.create_from_matrix(bayer.get_threshold_matrix());
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", size, size);
            output += heading;
            bm.start();
        }

        Fourier2D<int> fourier_2d = Fourier2D<int>(bayer.get_threshold_matrix(), true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            bm.stop();
            output += std::to_string(bm.time_us()) + " us\n";;
        }

        sprintf(file_name, "output\\threshold_matrix\\bayer_%ix%i_fourier.png", size, size);
        image.create_from_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_bayer_all(int output_levels, bool fourier, bool benchmark)
{
    std::string output = "Bayer:\n";

    output += generate_bayer(2, output_levels, fourier, benchmark);
    output += generate_bayer(4, output_levels, fourier, benchmark);
    output += generate_bayer(8, output_levels, fourier, benchmark);
    output += generate_bayer(16, output_levels, fourier, benchmark);
    output += generate_bayer(32, output_levels, fourier, benchmark);
    output += generate_bayer(64, output_levels, fourier, benchmark);

    return output;
}

std::string generate_blue_noise(int width, int height, double sigma, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    Noise2D<int> blue_noise = Noise2D<int>(width, height, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\blue_noise_%ix%i.png", width, height);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", width, height);
        output += heading;
        bm.start();
    }
    
    blue_noise.generate_blue_noise(sigma);

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    std::vector<std::vector<int>> threshold_matrix = create_matrix_from_noise(blue_noise, width, height);

    image.create_from_matrix(threshold_matrix);
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", width, height);
            output += heading;
            bm.start();
        }

        Fourier2D<int> fourier_2d = Fourier2D<int>(threshold_matrix, true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            bm.stop();
            output += std::to_string(bm.time_us()) + " us\n";;
        }

        sprintf(file_name, "output\\threshold_matrix\\blue_noise_%ix%i_fourier.png", width, height);
        image.create_from_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_blue_noise_all(double sigma, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "Blue Noise:\n";

    output += generate_blue_noise(2, 2, sigma, output_levels, fourier, benchmark);
    output += generate_blue_noise(4, 4, sigma, output_levels, fourier, benchmark);
    output += generate_blue_noise(8, 8, sigma, output_levels, fourier, benchmark);
    output += generate_blue_noise(16, 16, sigma, output_levels, fourier, benchmark);
    output += generate_blue_noise(32, 32, sigma, output_levels, fourier, benchmark);
    output += generate_blue_noise(64, 64, sigma, output_levels, fourier, benchmark);

    return output;
}

std::string generate_brown_noise(int width, int height, double leaky_integrator, std::size_t kernel_size, double sigma, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    Noise2D<int> brown_noise = Noise2D<int>(width, height, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\brown_noise_%ix%i.png", width, height);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", width, height);
        output += heading;
        bm.start();
    }

    brown_noise.generate_brown_noise(leaky_integrator, kernel_size, sigma);

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    std::vector<std::vector<int>> threshold_matrix = create_matrix_from_noise(brown_noise, width, height);

    image.create_from_matrix(threshold_matrix);
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", width, height);
            output += heading;
            bm.start();
        }

        Fourier2D<int> fourier_2d = Fourier2D<int>(threshold_matrix, true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            bm.stop();
            output += std::to_string(bm.time_us()) + " us\n";;
        }

        sprintf(file_name, "output\\threshold_matrix\\brown_noise_%ix%i_fourier.png", width, height);
        image.create_from_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_brown_noise_all(double leaky_integrator, std::size_t kernel_size, double sigma, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "Brown Noise:\n";

    output += generate_brown_noise(2, 2, leaky_integrator, kernel_size, sigma, output_levels, fourier, benchmark);
    output += generate_brown_noise(4, 4, leaky_integrator, kernel_size, sigma, output_levels, fourier, benchmark);
    output += generate_brown_noise(8, 8, leaky_integrator, kernel_size, sigma, output_levels, fourier, benchmark);
    output += generate_brown_noise(16, 16, leaky_integrator, kernel_size, sigma, output_levels, fourier, benchmark);
    output += generate_brown_noise(32, 32, leaky_integrator, kernel_size, sigma, output_levels, fourier, benchmark);
    output += generate_brown_noise(64, 64, leaky_integrator, kernel_size, sigma, output_levels, fourier, benchmark);

    return output;
}

std::string generate_white_noise(int width, int height, int output_levels, bool fourier, bool benchmark)
{
    std::string output = "";
    Benchmark bm = Benchmark();
    Noise2D<int> white_noise = Noise2D<int>(width, height, output_levels);
    Image image = Image();
    char file_name[1000];
    sprintf(file_name, "output\\threshold_matrix\\white_noise_%ix%i.png", width, height);

    if(benchmark)
    {
        char heading[100];
        sprintf(heading, "%ix%i time: ", width, height);
        output += heading;
        bm.start();
    }

    white_noise.generate_white_noise();

    if(benchmark)
    {
        bm.stop();
        output += std::to_string(bm.time_us()) + " us\n";;
    }

    std::vector<std::vector<int>> threshold_matrix = create_matrix_from_noise(white_noise, width, height);

    image.create_from_matrix(threshold_matrix);
    image.save(file_name);

    if(fourier)
    {
        if(benchmark)
        {
            char heading[100];
            sprintf(heading, "%ix%i fourier time: ", width, height);
            output += heading;
            bm.start();
        }

        Fourier2D<int> fourier_2d = Fourier2D<int>(threshold_matrix, true, true);
        fourier_2d.dft();
        fourier_2d.normalize_transform(output_levels);

        if(benchmark)
        {
            bm.stop();
            output += std::to_string(bm.time_us()) + " us\n";;
        }

        sprintf(file_name, "output\\threshold_matrix\\white_noise_%ix%i_fourier.png", width, height);
        image.create_from_matrix(fourier_2d.get_transform());
        image.save(file_name);
    }

    return output;
}

std::string generate_white_noise_all(int output_levels, bool fourier, bool benchmark)
{
    std::string output = "White Noise:\n";

    output += generate_white_noise(2, 2, output_levels, fourier, benchmark);
    output += generate_white_noise(4, 4, output_levels, fourier, benchmark);
    output += generate_white_noise(8, 8, output_levels, fourier, benchmark);
    output += generate_white_noise(16, 16, output_levels, fourier, benchmark);
    output += generate_white_noise(32, 32, output_levels, fourier, benchmark);
    output += generate_white_noise(64, 64, output_levels, fourier, benchmark);

    return output;
}

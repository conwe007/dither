#ifndef __CONVOLVE_H
#define __CONVOLVE_H

#include "helper.h"
#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

template <typename T, typename K>
std::vector<std::vector<T>> convolve(std::vector<std::vector<T>> matrix, std::vector<std::vector<K>> kernel, double leaky_integrator);

template <typename T>
std::vector<std::vector<T>> gaussian_kernel(std::size_t size, double sigma);

enum Kernel
{
    RIDGE_4,
    RIDGE_8,
    SHARPEN_4,
    SHARPEN_8,
    BOX_BLUR,
    GAUSSIAN_BLUR,
    UNSHARP_MASK
};

static const std::unordered_map<Kernel, std::vector<std::vector<double>>, EnumHash> KERNEL_VALUES = {
    {Kernel::RIDGE_4, KERNEL_RIDGE_4},
    {Kernel::RIDGE_8, KERNEL_RIDGE_8},
    {Kernel::SHARPEN_4, KERNEL_SHARPEN_4},
    {Kernel::SHARPEN_8, KERNEL_SHARPEN_8},
    {Kernel::BOX_BLUR, KERNEL_BOX_BLUR},
    {Kernel::GAUSSIAN_BLUR, KERNEL_GAUSSIAN_BLUR},
    {Kernel::UNSHARP_MASK, KERNEL_UNSHARP_MASK}
};

static const std::unordered_map<Kernel, std::string, EnumHash> KERNEL_STRING = {
    {Kernel::RIDGE_4, "ridge_4"},
    {Kernel::RIDGE_8, "ridge_8"},
    {Kernel::SHARPEN_4, "sharpen_4"},
    {Kernel::SHARPEN_8, "sharpen_8"},
    {Kernel::BOX_BLUR, "box_blur"},
    {Kernel::GAUSSIAN_BLUR, "gaussian_blur"},
    {Kernel::UNSHARP_MASK, "unsharp_mask"}
};

const std::vector<std::vector<double>> KERNEL_RIDGE_4 = {
    {+0.0, -1.0, +0.0},
    {-1.0, +4.0, -1.0},
    {+0.0, -1.0, +0.0}
};

const std::vector<std::vector<double>> KERNEL_RIDGE_8 = {
    {-1.0, -1.0, -1.0},
    {-1.0, +8.0, -1.0},
    {-1.0, -1.0, -1.0}
};

const std::vector<std::vector<double>> KERNEL_SHARPEN_4 = {
    {+0.0, -1.0, +0.0},
    {-1.0, +5.0, -1.0},
    {+0.0, -1.0, +0.0}
};

const std::vector<std::vector<double>> KERNEL_SHARPEN_8 = {
    {-1.0, -1.0, -1.0},
    {-1.0, +9.0, -1.0},
    {-1.0, -1.0, -1.0}
};

const std::vector<std::vector<double>> KERNEL_BOX_BLUR = {
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0},
    {1.0 / 9.0, 1.0 / 9.0, 1.0 / 9.0}
};

const std::vector<std::vector<double>> KERNEL_GAUSSIAN_BLUR = {
    {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902},
    {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
    {0.0219382, 0.0983203, 0.162103, 0.0983203, 0.0219382},
    {0.0133062, 0.0596343, 0.0983203, 0.0596343, 0.0133062},
    {0.00296902, 0.0133062, 0.0219382, 0.0133062, 0.00296902}
};

const std::vector<std::vector<double>> KERNEL_UNSHARP_MASK = {
    {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902},
    {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
    {-0.0219382, -0.0983203, +1.837897, -0.0983203, -0.0219382},
    {-0.0133062, -0.0596343, -0.0983203, -0.0596343, -0.0133062},
    {-0.00296902, -0.0133062, -0.0219382, -0.0133062, -0.00296902}
};

template <typename T, typename K>
std::vector<std::vector<T>> convolve(std::vector<std::vector<T>> matrix, std::vector<std::vector<K>> kernel, double leaky_integrator)
{
    const std::size_t matrix_height = matrix.size();
    const std::size_t matrix_width = matrix[0].size();
    const std::size_t kernel_height = kernel.size();
    const std::size_t kernel_width = kernel[0].size();
    const std::size_t kernel_height_half = kernel_height / 2;
    const std::size_t kernel_width_half = kernel_width / 2;
    std::vector<std::vector<T>> convolved_matrix = std::vector<std::vector<T>>(matrix_height, std::vector<T>(matrix_width, 0.0));
    double sum = 0.0;

    for(std::size_t my = 0; my < matrix_height; my++)
    {
        for(std::size_t mx = 0; mx < matrix_width; mx++)
        {
            sum = 0;
            
            for(std::size_t ky = 0; ky < kernel_height; ky++)
            {
                for(std::size_t kx = 0; kx < kernel_width; kx++)
                {
                    std::size_t dy = (matrix_height + ((my + ky - kernel_height_half) % matrix_height)) % matrix_height;
                    std::size_t dx = (matrix_width + ((mx + kx - kernel_width_half) % matrix_width)) % matrix_width;
                    sum += static_cast<double>(matrix[dy][dx]) * static_cast<double>(kernel[ky][kx]) * leaky_integrator;
                }
            }
            
            convolved_matrix[my][mx] = static_cast<T>(sum);
        }
    }

    return convolved_matrix;
}

template <typename T>
std::vector<std::vector<T>> gaussian_kernel(std::size_t size, double sigma)
{
    std::vector<std::vector<T>> kernel = std::vector<std::vector<T>>(size, std::vector<T>(size, static_cast<T>(0)));

    const double NOISE2D_PI = acos(-1.0);
    const int half_size = size / 2;
    const float two_sigma_squared = 2.0 * sigma * sigma;
    double sum = 0.0;

    for(int y = -half_size; y < half_size + 1; y++)
    {
        for(int x = -half_size; x < half_size + 1; x++)
        {
            kernel[y + half_size][x + half_size] = exp(-1 * (x * x + y * y) / two_sigma_squared) / (NOISE2D_PI * two_sigma_squared);
            sum += kernel[y + half_size][x + half_size];
        }
    }

    for(std::size_t y = 0; y < size; y++)
    {
        for(std::size_t x = 0; x < size; x++)
        {
            kernel[y][x] /= sum;
        }
    }

    return kernel;
}

#endif

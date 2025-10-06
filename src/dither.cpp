#include "dither.h"
#include <iostream>

// initializes empty image and palette
Dither::Dither(std::size_t frames, bool gamma_correction)
{
    this->image = Image(frames);
    this->palette = Palette();
    this->gamma_correction = gamma_correction;
    return;
}

// returns the matrix of grayscale pixel values being dithered
std::vector<std::vector<int>> Dither::get_matrix()
{
    return image.get_matrix_from_image();
}

// sets the palette
void Dither::set_palette(Palette palette)
{
    this->palette = palette;
    return;
}

// loads a png at the specified file path
std::size_t Dither::load(const char* file_name)
{
    return image.load(file_name);
}

// save a png to the specified file path
std::size_t Dither::save(const char* file_name)
{
    return image.save(file_name);
}

// converts image to grayscale using specified method
void Dither::grayscale(GrayscaleMethod method)
{
    std::size_t height = image.get_height();
    std::size_t width = image.get_width();
    
    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            Color color = image.get_pixel(x, y);
            int16_t channel_value = Grayscale::channel_value(color, method);
            Color color_grayscale = Color(channel_value, channel_value, channel_value, Color::CHANNEL_MAX);
            image.set_pixel(color_grayscale, x, y);
        }
    }

    return;
}

// changes each pixel in the image to the closest color in the palette
void Dither::reduce()
{
    std::size_t height = image.get_height();
    std::size_t width = image.get_width();
    
    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            Color color = image.get_pixel(x, y);
            Color palette_nearest = palette.nearest(color);
            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

// reduces the image to the colors in the palette and dithers using specified algorithm
// if alternate_direction is true, switches direction of error diffusion each row
void Dither::error_diffusion(ErrorDiffusionAlgorithm algorithm, bool alternate)
{
    if(alternate)
    {
        error_diffusion_alternate(algorithm);
    }
    else
    {
        error_diffusion_standard(algorithm);
    }

    return;
}

// reduces the image to the colors in the palette and dithers against the specified threshold matrix
void Dither::ordered(std::vector<std::vector<int>> threshold_matrix)
{
    std::size_t image_height = image.get_height();
    std::size_t image_width = image.get_width();
    std::size_t threshold_matrix_height = threshold_matrix.size();
    std::size_t threshold_matrix_width = threshold_matrix[0].size();

    palette.sort();

    Color palette_pitch_vector = palette.pitch_vector();
    int color_channel_max_half = Color::CHANNEL_MAX / 2;
    Color color;
    int threshold_value;
    int threshold_value_scaled;
    Color threshold_color_offset;
    Color palette_nearest;

    for(std::size_t y = 0; y < image_height; y++)
    {
        for(std::size_t x = 0; x < image_width; x++)
        {
            color = image.get_pixel(x, y);
            threshold_value = threshold_matrix[y % threshold_matrix_height][x % threshold_matrix_width];
            // normalize the threshold value by subracting half the color channel range (normalized values should range from -127 to 128)
            threshold_value_scaled = threshold_value - color_channel_max_half;
            // create a color offset at each position that is the average spread in the color space (palette_pitch_vector / color_channel_max) multiplied by the normalized threshold
            threshold_color_offset = Color(
                threshold_value_scaled * palette_pitch_vector.r / Color::CHANNEL_MAX,
                threshold_value_scaled * palette_pitch_vector.g / Color::CHANNEL_MAX,
                threshold_value_scaled * palette_pitch_vector.b / Color::CHANNEL_MAX,
                Color::CHANNEL_MAX);
            palette_nearest = palette.nearest(color + threshold_color_offset);
            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

// convolves the image against the specified kernel
void Dither::convolution(Kernel kernel_type, EdgeHandling edge_handling)
{
    std::size_t image_height = image.get_height();
    std::size_t image_width = image.get_width();
    std::vector<std::vector<double>> kernel = KERNEL_VALUES.at(kernel_type);
    std::size_t kernel_height = kernel.size();
    std::size_t kernel_width = kernel[0].size();
    std::size_t kernel_height_half = kernel_height / 2;
    std::size_t kernel_width_half = kernel_width / 2;
    Color color;
    std::vector<std::vector<int>> image_matrix_r;
    std::vector<std::vector<int>> image_matrix_g;
    std::vector<std::vector<int>> image_matrix_b;

    switch(edge_handling)
    {
        case EdgeHandling::EXTEND:
            image_matrix_r = std::vector<std::vector<int>>(image_height + 2 * kernel_height_half, std::vector<int>(image_width + 2 * kernel_width_half, 0));
            image_matrix_g = std::vector<std::vector<int>>(image_height + 2 * kernel_height_half, std::vector<int>(image_width + 2 * kernel_width_half, 0));
            image_matrix_b = std::vector<std::vector<int>>(image_height + 2 * kernel_height_half, std::vector<int>(image_width + 2 * kernel_width_half, 0));

            for(std::size_t x = 0; x < image_width; x++)
            {
                for(std::size_t y = -kernel_height_half; y < 0; y++)
                {
                    color = image.get_pixel(x, 0);
                    image_matrix_r[y][x + kernel_width_half] = color.r;
                    image_matrix_g[y][x + kernel_width_half] = color.g;
                    image_matrix_b[y][x + kernel_width_half] = color.b;
                }

                for(std::size_t y = image_height; y < image_height + kernel_height_half; y++)
                {
                    color = image.get_pixel(x, image_height - 1);
                    image_matrix_r[y][x + kernel_width_half] = color.r;
                    image_matrix_g[y][x + kernel_width_half] = color.g;
                    image_matrix_b[y][x + kernel_width_half] = color.b;
                }
            }

            for(std::size_t y = 0; y < image_height; y++)
            {
                for(std::size_t x = -kernel_width_half; x < 0; x++)
                {
                    color = image.get_pixel(0, y);
                    image_matrix_r[y + kernel_height_half][x] = color.r;
                    image_matrix_g[y + kernel_height_half][x] = color.g;
                    image_matrix_b[y + kernel_height_half][x] = color.b;
                }

                for(std::size_t x = 0; x < image_width; x++)
                {
                    color = image.get_pixel(x, y);
                    image_matrix_r[y + kernel_height_half][x + kernel_width_half] = color.r;
                    image_matrix_g[y + kernel_height_half][x + kernel_width_half] = color.g;
                    image_matrix_b[y + kernel_height_half][x + kernel_width_half] = color.b;
                }

                for(std::size_t x = image_width; x < image_width + kernel_width_half; x++)
                {
                    color = image.get_pixel(image_width - 1, y);
                    image_matrix_r[y + kernel_height_half][x] = color.r;
                    image_matrix_g[y + kernel_height_half][x] = color.g;
                    image_matrix_b[y + kernel_height_half][x] = color.b;
                }
            }
            
            break;

        case EdgeHandling::WRAP:
            image_matrix_r = std::vector<std::vector<int>>(image_height, std::vector<int>(image_width, 0));
            image_matrix_g = std::vector<std::vector<int>>(image_height, std::vector<int>(image_width, 0));
            image_matrix_b = std::vector<std::vector<int>>(image_height, std::vector<int>(image_width, 0));

            for(std::size_t y = 0; y < image_height; y++)
            {
                for(std::size_t x = 0; x < image_width; x++)
                {
                    color = image.get_pixel(x, y);
                    image_matrix_r[y][x] = color.r;
                    image_matrix_g[y][x] = color.g;
                    image_matrix_b[y][x] = color.b;
                }
            }

            break;
    }

    std::vector<std::vector<int>> image_matrix_r_convolved = convolve<int, double>(image_matrix_r, kernel);
    std::vector<std::vector<int>> image_matrix_g_convolved = convolve<int, double>(image_matrix_g, kernel);
    std::vector<std::vector<int>> image_matrix_b_convolved = convolve<int, double>(image_matrix_b, kernel);

    for(std::size_t y = 0; y < image_height; y++)
    {
        for(std::size_t x = 0; x < image_width; x++)
        {
            image_matrix_r_convolved[y][x] = std::clamp(image_matrix_r_convolved[y][x], 0, Color::CHANNEL_MAX);
            image_matrix_g_convolved[y][x] = std::clamp(image_matrix_g_convolved[y][x], 0, Color::CHANNEL_MAX);
            image_matrix_b_convolved[y][x] = std::clamp(image_matrix_b_convolved[y][x], 0, Color::CHANNEL_MAX);
            color = Color(image_matrix_r_convolved[y][x], image_matrix_g_convolved[y][x], image_matrix_b_convolved[y][x], Color::CHANNEL_MAX);
            image.set_pixel(color, x, y);
        }
    }

    return;
}

void Dither::temporal()
{

}

// dithers using specified algorithm, does not alternate direction on odd rows
void Dither::error_diffusion_standard(ErrorDiffusionAlgorithm algorithm)
{
    // initialize error diffusion containers
    ErrorDiffusion error_diffusion = ErrorDiffusion(algorithm);
    std::vector<std::vector<std::vector<int>>> error_matrix(image.get_height(), std::vector<std::vector<int>>(image.get_width(), std::vector<int>(3, 0)));
    std::size_t height = image.get_height();
    std::size_t width = image.get_width();
    
    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            // set current pixel to nearest palette color (accounting for accumulated error)
            Color color = image.get_pixel(x, y);

            if(gamma_correction)
            {
                color.to_linear(image.get_gamma());
            }
            
            color.r += error_matrix[y][x][Color::INDEX_R];
            color.g += error_matrix[y][x][Color::INDEX_G];
            color.b += error_matrix[y][x][Color::INDEX_B];
            Color palette_nearest = palette.nearest(color);

            std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

            for(std::size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
            {
                std::size_t new_x = x + error_diffusion.coordinates[index_error].first;
                std::size_t new_y = y + error_diffusion.coordinates[index_error].second;

                if(new_x < 0 || new_x >= width || new_y < 0 || new_y >= height)
                {
                    continue;
                }

                error_matrix[new_y][new_x][Color::INDEX_R] += static_cast<int>(current_pixel_error[Color::INDEX_R] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                error_matrix[new_y][new_x][Color::INDEX_G] += static_cast<int>(current_pixel_error[Color::INDEX_G] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                error_matrix[new_y][new_x][Color::INDEX_B] += static_cast<int>(current_pixel_error[Color::INDEX_B] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
            }

            image.set_pixel(palette_nearest, x, y);
        }
    }

    return;
}

// dithers using specified algorithm, alternates direction on odd rows
void Dither::error_diffusion_alternate(ErrorDiffusionAlgorithm algorithm)
{
    // initialize error diffusion containers
    ErrorDiffusion error_diffusion = ErrorDiffusion(algorithm);
    std::vector<std::vector<std::vector<int>>> error_matrix(image.get_height(), std::vector<std::vector<int>>(image.get_width(), std::vector<int>(Color::NUM_BYTES_COLOR - 1, 0))); // RGB values (no A)
    std::size_t height = image.get_height();
    std::size_t width = image.get_width();

    for(std::size_t y = 0; y < height; y++)
    {
        // alternate direction on odd rows
        if(y % 2 == 1)
        {
            for(int x = width - 1; x >= 0; x--)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);
                color.to_linear(image.get_gamma());
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];
                Color palette_nearest = palette.nearest(color);

                std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(std::size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
                {
                    std::size_t new_x = x - error_diffusion.coordinates[index_error].first; // flip x when we are going backwards
                    std::size_t new_y = y + error_diffusion.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= width || new_y < 0 || new_y >= height)
                    {
                        continue;
                    }

                    error_matrix[new_y][new_x][Color::INDEX_R] += static_cast<int>(current_pixel_error[Color::INDEX_R] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_G] += static_cast<int>(current_pixel_error[Color::INDEX_G] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_B] += static_cast<int>(current_pixel_error[Color::INDEX_B] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                }

                image.set_pixel(palette_nearest, x, y);
            }
        }
        else
        {
            for(std::size_t x = 0; x < width; x++)
            {
                // set current pixel to nearest palette color (accounting for accumulated error)
                Color color = image.get_pixel(x, y);
                color.to_linear(image.get_gamma());
                color.r += error_matrix[y][x][Color::INDEX_R];
                color.g += error_matrix[y][x][Color::INDEX_G];
                color.b += error_matrix[y][x][Color::INDEX_B];
                Color palette_nearest = palette.nearest(color);

                std::vector<int> current_pixel_error = {color.r - palette_nearest.r, color.g - palette_nearest.g, color.b - palette_nearest.b};

                for(std::size_t index_error = 0; index_error < error_diffusion.coordinates.size(); index_error++)
                {
                    std::size_t new_x = x + error_diffusion.coordinates[index_error].first;
                    std::size_t new_y = y + error_diffusion.coordinates[index_error].second;

                    if(new_x < 0 || new_x >= width || new_y < 0 || new_y >= height)
                    {
                        continue;
                    }

                    error_matrix[new_y][new_x][Color::INDEX_R] += static_cast<int>(current_pixel_error[Color::INDEX_R] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_G] += static_cast<int>(current_pixel_error[Color::INDEX_G] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                    error_matrix[new_y][new_x][Color::INDEX_B] += static_cast<int>(current_pixel_error[Color::INDEX_B] * error_diffusion.scalars[error_diffusion.coordinates[index_error]]);
                }

                image.set_pixel(palette_nearest, x, y);
            }
        }
    }

    return;
}

// maps the values in the specified threshold matrix to the range 0.0-1.0
std::vector<std::vector<double>> Dither::normalize_threshold_matrix(std::vector<std::vector<int>> threshold_matrix)
{
    std::size_t height = threshold_matrix.size();
    std::size_t width = threshold_matrix[0].size();
    std::vector<std::vector<double>> threshold_matrix_normalized = std::vector<std::vector<double>>(height, std::vector<double>(width, 0.0));
    int threshold_matrix_min = INT_MAX;
    int threshold_matrix_max = INT_MIN;
    
    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            threshold_matrix_min = std::min(threshold_matrix_min, threshold_matrix[y][x]);
            threshold_matrix_max = std::max(threshold_matrix_max, threshold_matrix[y][x]);
        }
    }

    double threshold_matrix_range = static_cast<double>(threshold_matrix_max) - static_cast<double>(threshold_matrix_min);

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            threshold_matrix_normalized[y][x] = static_cast<double>(threshold_matrix[y][x]) / threshold_matrix_range;
        }
    }

    return threshold_matrix_normalized;
}

// maps the values in the specified threshold matrix to the specified range
std::vector<std::vector<int>> Dither::scale_threshold_matrix(std::vector<std::vector<int>> threshold_matrix, int min, int max)
{
    std::size_t height = threshold_matrix.size();
    std::size_t width = threshold_matrix[0].size();
    std::vector<std::vector<int>> threshold_matrix_scaled = std::vector<std::vector<int>>(height, std::vector<int>(width, 0.0));
    int threshold_matrix_min = INT_MAX;
    int threshold_matrix_max = INT_MIN;
    
    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            threshold_matrix_min = std::min(threshold_matrix_min, threshold_matrix[y][x]);
            threshold_matrix_max = std::max(threshold_matrix_max, threshold_matrix[y][x]);
        }
    }

    int threshold_matrix_range = threshold_matrix_max - threshold_matrix_min;
    int new_range = max - min;

    for(std::size_t y = 0; y < height; y++)
    {
        for(std::size_t x = 0; x < width; x++)
        {
            threshold_matrix_scaled[y][x] = (threshold_matrix[y][x] - threshold_matrix_min) * new_range / threshold_matrix_range;
        }
    }

    return threshold_matrix_scaled;
}

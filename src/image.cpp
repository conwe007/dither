#include "image.h"
#include <iostream>

// initializes empty image
Image::Image()
{
    pixels.resize(0);
    width = 0;
    height = 0;
    gamma = 0.0;
    return;
}

// erases all image data and reinitializes object
void Image::clear()
{
    pixels.resize(0);
    width = 0;
    height = 0;
    gamma = 0.0;
    return;
}

// returns image's width
size_t Image::get_width()
{
    return width;
}

// returns image's height
size_t Image::get_height()
{
    return height;
}

// loads a png from the specified path
size_t Image::load(const char* file_name)
{
    clear();
    
    std::vector<unsigned char> png_buffer;
    lodepng::State state;

    size_t error = lodepng::load_file(png_buffer, file_name);

    if(!error)
    {
        error = lodepng::decode(pixels, width, height, state, png_buffer);
    }

    if(error)
    {
        std::cout << "error: load" << file_name << " - " << error << ": " << lodepng_error_text(error) << std::endl;
        return error;
    }

    if(state.info_png.gama_defined)
    {
        gamma = 100000.0 / static_cast<double>(state.info_png.gama_gamma);
    }

    return error;
}

// saves a png to the specified path
size_t Image::save(const char* file_name)
{
    size_t error = lodepng::encode(file_name, pixels, width, height);

    if(error)
    {
        std::cout << "error: save - " << error << ": "<< lodepng_error_text(error) << std::endl;
    }

    return error;
}

// gets the pixel color at (x, y)
Color Image::get_pixel(unsigned int x, unsigned int y)
{
    Color color = Color();
    int index_start = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
    color.r = pixels[index_start + Color::INDEX_R];
    color.g = pixels[index_start + Color::INDEX_G];
    color.b = pixels[index_start + Color::INDEX_B];
    color.a = pixels[index_start + Color::INDEX_A];
    return color;
}

// sets the pixel color at (x, y)
void Image::set_pixel(Color color, unsigned int x, unsigned int y)
{
    int index_start = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
    pixels[index_start + Color::INDEX_R] = color.r;
    pixels[index_start + Color::INDEX_G] = color.g;
    pixels[index_start + Color::INDEX_B] = color.b;
    pixels[index_start + Color::INDEX_A] = color.a;
    return;
}

// returns the image's gamma value
double Image::get_gamma()
{
    return gamma;
}

// fills image with a grayscale representation of specified matrix
void Image::create_from_matrix(std::vector<std::vector<int>> matrix)
{
    height = matrix.size();
    width = matrix[0].size();
    pixels.resize(width * height * Color::NUM_BYTES_COLOR);

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            unsigned char channel_value = static_cast<unsigned char>(std::clamp(matrix[y][x], 0, Color::CHANNEL_MAX));
            int index_pixels = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
            pixels[index_pixels + 0] = channel_value;
            pixels[index_pixels + 1] = channel_value;
            pixels[index_pixels + 2] = channel_value;
            pixels[index_pixels + 3] = Color::CHANNEL_MAX;
        }
    }

    return;
}

// creates a threshold matrix from the loaded image
std::vector<std::vector<int>> Image::get_matrix_from_image()
{
    std::vector<std::vector<int>> matrix = std::vector<std::vector<int>>(height, std::vector<int>(width, 0));

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            matrix[y][x] = Grayscale::channel_value(get_pixel(x, y), GrayscaleMethod::BT709); 
        }
    }

    return matrix;
}

// converts image to linear color space
void Image::to_linear()
{
    // double output_levels = (Color::CHANNEL_MAX + 1);
    // gamma = new_gamma;

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            Color color = get_pixel(x, y);
            color.to_linear(gamma);
            // color.r = static_cast<int16_t>(output_levels * srgb_to_linear(static_cast<double>(color.r) / output_levels));
            // color.g = static_cast<int16_t>(output_levels * srgb_to_linear(static_cast<double>(color.g) / output_levels));
            // color.b = static_cast<int16_t>(output_levels * srgb_to_linear(static_cast<double>(color.b) / output_levels));
            set_pixel(color, x, y);
        }
    }

    return;
}

// converts image to srgb color space
void Image::to_srgb()
{
    // double output_levels = (Color::CHANNEL_MAX + 1);

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            Color color = get_pixel(x, y);
            color.to_srgb(gamma);
            // color.r = static_cast<int16_t>(output_levels * linear_to_srgb(static_cast<double>(color.r) / output_levels));
            // color.g = static_cast<int16_t>(output_levels * linear_to_srgb(static_cast<double>(color.g) / output_levels));
            // color.b = static_cast<int16_t>(output_levels * linear_to_srgb(static_cast<double>(color.b) / output_levels));
            set_pixel(color, x, y);
        }
    }

    return;
}

// converts an SRGB value between 0-1 to a linear value between 0-1
double Image::srgb_to_linear(double value_srgb)
{
    if(value_srgb < 0.04045)
    {
        return value_srgb / 12.92;
    }

    return pow((value_srgb + 0.055) / 1.055, gamma);
}

// converts a linear value between 0-1 to an SRGB value between 0-1
double Image::linear_to_srgb(double value_linear)
{
    if(value_linear < 0.0031308)
    {
        return value_linear * 12.92;
    }

    return 1.055 * pow(value_linear, 1.0 / gamma) - 0.055;
}

#include "image.h"
#include <iostream>

// initializes empty image
Image::Image()
{
    pixels.resize(1);
    pixels[0].resize(0);
    width = 0;
    height = 0;
    gamma = 0.0;
    frames = 1;
    return;
}

// initializes empty image
Image::Image(std::size_t frames)
{
    this->pixels.resize(frames);
    
    for(std::size_t index_frame = 0; index_frame < frames; index_frame++)
    {
        this->pixels[index_frame].resize(0);
    }

    this->width = 0;
    this->height = 0;
    this->gamma = 0.0;
    this->frames = frames;

    return;
}

// erases all image data and reinitializes object
void Image::clear()
{
    pixels.resize(frames);

    for(std::size_t index_frame = 0; index_frame < frames; index_frame++)
    {
        this->pixels[index_frame].resize(0);
    }

    width = 0;
    height = 0;
    gamma = 0.0;

    return;
}

// returns image's width
std::size_t Image::get_width()
{
    return width;
}

// returns image's height
std::size_t Image::get_height()
{
    return height;
}

// loads a png from the specified path
std::size_t Image::load(const char* file_name)
{
    clear();
    
    std::vector<unsigned char> png_buffer;
    lodepng::State state;

    std::size_t error = lodepng::load_file(png_buffer, file_name);

    if(!error)
    {
        error = lodepng::decode(pixels[0], width, height, state, png_buffer);
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
std::size_t Image::save(const char* file_name)
{
    std::string file_name_string = file_name;
    std::size_t error = 0;

    if(file_name_string.contains(".png"))
    {
        error = lodepng::encode(file_name, pixels[0], width, height);

        if(error)
        {
            std::cout << "error: png save " << file_name << " - " << error << ": "<< lodepng_error_text(error) << std::endl;
        }
    }
    else if(file_name_string.contains(".gif"))
    {
        const uint32_t DELAY = 1;
        const uint32_t BIT_DEPTH = 8;
        const bool DITHER = false;
        GifWriter writer = {};
        bool error_bool = GifBegin(&writer, file_name, width, height, DELAY, BIT_DEPTH, DITHER);

        for(std::size_t index_frame = 0; index_frame < frames; index_frame++)
        {
            error_bool = GifWriteFrame(&writer, pixels[index_frame].data(), width, height, DELAY, BIT_DEPTH, DITHER);
        }

        if(error_bool)
        {
            std::cout << "error: gif save - " << file_name << std::endl;
            error = 1;
        }
    }

    return error;
}

// gets the pixel color at (x, y)
Color Image::get_pixel(unsigned int x, unsigned int y, std::size_t frame)
{
    Color color = Color();
    int index_start = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
    color.r = pixels[frame][index_start + Color::INDEX_R];
    color.g = pixels[frame][index_start + Color::INDEX_G];
    color.b = pixels[frame][index_start + Color::INDEX_B];
    color.a = pixels[frame][index_start + Color::INDEX_A];
    return color;
}

// sets the pixel color at (x, y)
void Image::set_pixel(Color color, unsigned int x, unsigned int y, std::size_t frame)
{
    int index_start = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
    pixels[frame][index_start + Color::INDEX_R] = color.r;
    pixels[frame][index_start + Color::INDEX_G] = color.g;
    pixels[frame][index_start + Color::INDEX_B] = color.b;
    pixels[frame][index_start + Color::INDEX_A] = color.a;
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
    pixels[0].resize(width * height * Color::NUM_BYTES_COLOR);

    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            unsigned char channel_value = static_cast<unsigned char>(std::clamp(matrix[y][x], 0, Color::CHANNEL_MAX));
            int index_pixels = Color::NUM_BYTES_COLOR * width * y + Color::NUM_BYTES_COLOR * x;
            pixels[0][index_pixels + 0] = channel_value;
            pixels[0][index_pixels + 1] = channel_value;
            pixels[0][index_pixels + 2] = channel_value;
            pixels[0][index_pixels + 3] = Color::CHANNEL_MAX;
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
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            Color color = get_pixel(x, y);
            color.to_linear(gamma);
            set_pixel(color, x, y);
        }
    }

    return;
}

// converts image to srgb color space
void Image::to_srgb()
{
    for(size_t y = 0; y < height; y++)
    {
        for(size_t x = 0; x < width; x++)
        {
            Color color = get_pixel(x, y);
            color.to_srgb(gamma);
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

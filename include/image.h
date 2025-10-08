#ifndef __IMAGE_H
#define __IMAGE_H

#include "color.h" // Color struct
#include "gif.h" // 
#include "grayscale.h" // Grayscale::channel_value
#include "lodepng.h" // png encode/decode functions
#include <algorithm> // std::clamp
#include <vector> // std::vector

class Image
{
public:
    Image();
    Image(std::size_t frames);
    void clear();
    std::size_t get_width();
    std::size_t get_height();
    std::size_t load(const char* file_name);
    std::size_t save(const char* file_name);
    Color get_pixel(unsigned int x, unsigned int y, std::size_t frame = 0);
    void set_pixel(Color color, unsigned int x, unsigned int y, std::size_t frame = 0);
    double get_gamma();
    std::size_t get_frames();
    void create_from_matrix(std::vector<std::vector<int>> matrix);
    std::vector<std::vector<int>> get_matrix_from_image();
    void to_linear();
    void to_srgb();

private:
    double srgb_to_linear(double value_srgb);
    double linear_to_srgb(double value_linear);
    
    std::vector<std::vector<unsigned char>> pixels;
    unsigned int width;
    unsigned int height;
    double gamma;
    std::size_t frames;
};

#endif

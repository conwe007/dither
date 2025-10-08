#ifndef __COLOR_H
#define __COLOR_H

#include <cmath> // sqrt()
#include <cstdint> // int16_t, uint32_t
#include <iomanip> // std::hex
#include <sstream> // std::stringstream
#include <string> // std::string

struct Color
{
    Color();
    Color(int16_t r, int16_t g, int16_t b, int16_t a);
    Color(uint32_t hex_code);
    bool operator==(const Color& other) const;
    bool operator<(const Color& other) const;
    Color operator-(const Color& other) const;
    Color operator+(const Color& other) const;
    Color operator*(const double scalar) const;
    uint32_t hex();
    double get_lightness();

    int distance_squared(Color other);
    double distance(Color other);
    int distance_redmean_squared(Color other);
    double distance_redmean(Color other);
    int distance_grayscale(Color other);
    static double distance_between(Color color1, Color color2);

    void to_linear(double gamma);
    void to_srgb(double gamma);

    std::string to_string();
    std::string to_string_int();

    static inline constexpr int NUM_BYTES_COLOR = 4;
    static inline constexpr int INDEX_R = 0;
    static inline constexpr int INDEX_G = 1;
    static inline constexpr int INDEX_B = 2;
    static inline constexpr int INDEX_A = 3;

    static inline constexpr int CHANNEL_MAX = 255;
    static inline constexpr int NUM_CHANNELS = 256;

    int16_t r;
    int16_t g;
    int16_t b;
    int16_t a;
};

#endif

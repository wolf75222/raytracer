#include "utils/image.h"
#include <fstream>
#include <algorithm>
#include <cmath>
#include <stdexcept>

ImageBuffer::ImageBuffer(int width, int height)
    : width_(width), height_(height), pixels_(width * height, Color(0, 0, 0)) {}

void ImageBuffer::set_pixel(int x, int y, const Color& color) {
    pixels_[index(x, y)] = color;
}

Color ImageBuffer::get_pixel(int x, int y) const {
    return pixels_[index(x, y)];
}

// ACES filmic tone mapping + sRGB gamma
static double aces_tonemap(double x) {
    // ACES filmic curve: meilleur rendu que simple sqrt gamma
    double a = x * (2.51 * x + 0.03);
    double b = x * (2.43 * x + 0.59) + 0.14;
    return std::max(0.0, std::min(1.0, a / b));
}

int ImageBuffer::to_byte(double linear) {
    double tonemapped = aces_tonemap(std::max(0.0, linear));
    // sRGB gamma approximation
    double corrected = std::sqrt(tonemapped);
    return static_cast<int>(255.999 * corrected);
}

void ImageBuffer::write_ppm(std::ostream& out) const {
    out << "P3\n" << width_ << ' ' << height_ << "\n255\n";
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const Color& c = pixels_[index(x, y)];
            out << to_byte(c.x()) << ' '
                << to_byte(c.y()) << ' '
                << to_byte(c.z()) << '\n';
        }
    }
}

void ImageBuffer::write_ppm(const std::string& filename) const {
    std::ofstream file(filename);
    if (!file)
        throw std::runtime_error("Cannot open file: " + filename);
    write_ppm(file);
}

void ImageBuffer::write_ppm_binary(const std::string& filename) const {
    std::ofstream file(filename, std::ios::binary);
    if (!file)
        throw std::runtime_error("Cannot open file: " + filename);

    // Header P6 (toujours en texte)
    file << "P6\n" << width_ << ' ' << height_ << "\n255\n";

    // Pixel data en binaire (3 octets par pixel)
    for (int y = 0; y < height_; ++y) {
        for (int x = 0; x < width_; ++x) {
            const Color& c = pixels_[index(x, y)];
            unsigned char rgb[3] = {
                (unsigned char)to_byte(c.x()),
                (unsigned char)to_byte(c.y()),
                (unsigned char)to_byte(c.z())
            };
            file.write(reinterpret_cast<char*>(rgb), 3);
        }
    }
}

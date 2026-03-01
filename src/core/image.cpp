#include "core/image.h"
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

int ImageBuffer::to_byte(double linear) {
    // Clamp to [0, 1], apply gamma 2.0 correction, map to [0, 255]
    double clamped = std::max(0.0, std::min(1.0, linear));
    double corrected = std::sqrt(clamped);
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

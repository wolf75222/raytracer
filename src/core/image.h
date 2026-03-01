#ifndef RT_IMAGE_H
#define RT_IMAGE_H

#include "vec3.h"
#include <vector>
#include <string>
#include <ostream>

class ImageBuffer {
public:
    ImageBuffer(int width, int height);

    int width() const { return width_; }
    int height() const { return height_; }

    void set_pixel(int x, int y, const Color& color);
    Color get_pixel(int x, int y) const;

    // Write PPM P3 (text) format
    void write_ppm(std::ostream& out) const;
    void write_ppm(const std::string& filename) const;

private:
    int width_;
    int height_;
    std::vector<Color> pixels_;

    int index(int x, int y) const { return y * width_ + x; }

    // Apply gamma correction (gamma 2.0 = sqrt)
    static int to_byte(double linear);
};

#endif // RT_IMAGE_H

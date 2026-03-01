#include <gtest/gtest.h>
#include "core/image.h"
#include <sstream>
#include <cmath>

TEST(ImageBuffer, Dimensions) {
    ImageBuffer img(3, 2);
    EXPECT_EQ(img.width(), 3);
    EXPECT_EQ(img.height(), 2);
}

TEST(ImageBuffer, SetGetPixel) {
    ImageBuffer img(2, 2);
    Color c(0.5, 0.3, 0.1);
    img.set_pixel(1, 0, c);
    Color got = img.get_pixel(1, 0);
    EXPECT_DOUBLE_EQ(got.x(), 0.5);
    EXPECT_DOUBLE_EQ(got.y(), 0.3);
    EXPECT_DOUBLE_EQ(got.z(), 0.1);
}

TEST(ImageBuffer, DefaultPixelIsBlack) {
    ImageBuffer img(1, 1);
    Color c = img.get_pixel(0, 0);
    EXPECT_DOUBLE_EQ(c.x(), 0.0);
    EXPECT_DOUBLE_EQ(c.y(), 0.0);
    EXPECT_DOUBLE_EQ(c.z(), 0.0);
}

TEST(ImageBuffer, WritePpmHeader) {
    ImageBuffer img(3, 2);
    std::ostringstream oss;
    img.write_ppm(oss);
    std::string output = oss.str();

    // Check header
    EXPECT_TRUE(output.substr(0, 2) == "P3");
    EXPECT_NE(output.find("3 2"), std::string::npos);
    EXPECT_NE(output.find("255"), std::string::npos);
}

TEST(ImageBuffer, WritePpmGammaCorrection) {
    // Linear 0.25 -> sqrt(0.25) = 0.5 -> int(0.5 * 255.999) = 127
    ImageBuffer img(1, 1);
    img.set_pixel(0, 0, Color(0.25, 0.25, 0.25));

    std::ostringstream oss;
    img.write_ppm(oss);
    std::string output = oss.str();

    // The pixel value should be 127 (gamma corrected)
    EXPECT_NE(output.find("127 127 127"), std::string::npos);
}

TEST(ImageBuffer, WritePpmBlackPixel) {
    ImageBuffer img(1, 1);
    // Default black
    std::ostringstream oss;
    img.write_ppm(oss);
    std::string output = oss.str();
    EXPECT_NE(output.find("0 0 0"), std::string::npos);
}

TEST(ImageBuffer, WritePpmWhitePixel) {
    ImageBuffer img(1, 1);
    img.set_pixel(0, 0, Color(1.0, 1.0, 1.0));
    std::ostringstream oss;
    img.write_ppm(oss);
    std::string output = oss.str();
    EXPECT_NE(output.find("255 255 255"), std::string::npos);
}

#include <gtest/gtest.h>
#include "utils/image.h"
#include <sstream>
#include <fstream>
#include <string>
#include <cmath>
#include <cstdio>

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
    // ACES tone mapping + gamma : linear 0.25 -> tonemapped -> gamma
    ImageBuffer img(1, 1);
    img.set_pixel(0, 0, Color(0.25, 0.25, 0.25));

    std::ostringstream oss;
    img.write_ppm(oss);
    std::string output = oss.str();

    // Avec ACES, la valeur sera differente de 127 mais > 0 et < 255
    // Verifions que le pixel n'est ni noir ni blanc
    EXPECT_EQ(output.find("0 0 0"), std::string::npos); // pas noir
    EXPECT_EQ(output.find("255 255 255"), std::string::npos); // pas blanc
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
    // ACES tone mapping: linear 1.0 -> ~0.98 -> gamma -> ~252
    // Doit etre proche de 255 mais pas forcement exact
    EXPECT_EQ(output.find("0 0 0"), std::string::npos); // pas noir
}

TEST(ImageBuffer, WritePpmBinary) {
    ImageBuffer img(2, 2);
    img.set_pixel(0, 0, Color(1.0, 0.0, 0.0));
    img.set_pixel(1, 0, Color(0.0, 1.0, 0.0));
    img.set_pixel(0, 1, Color(0.0, 0.0, 1.0));
    img.set_pixel(1, 1, Color(1.0, 1.0, 1.0));

    std::string tmpfile = "test_binary_output.ppm";
    img.write_ppm_binary(tmpfile);

    // Relire et verifier le header
    std::ifstream f(tmpfile, std::ios::binary);
    EXPECT_TRUE(f.good());
    std::string magic;
    int w, h, maxval;
    f >> magic >> w >> h >> maxval;
    EXPECT_EQ(magic, "P6");
    EXPECT_EQ(w, 2);
    EXPECT_EQ(h, 2);
    EXPECT_EQ(maxval, 255);

    // Skip newline after header
    f.get();

    // Lire les 4 pixels (12 bytes)
    unsigned char pixels[12];
    f.read(reinterpret_cast<char*>(pixels), 12);
    EXPECT_TRUE(f.good());

    // Pixel (0,0) = rouge -> R > 200, G = 0, B = 0 (ACES modifie legerement)
    EXPECT_GT(pixels[0], 200);
    EXPECT_EQ(pixels[1], 0);
    EXPECT_EQ(pixels[2], 0);

    // Pixel (1,0) = vert -> R = 0, G > 200, B = 0
    EXPECT_EQ(pixels[3], 0);
    EXPECT_GT(pixels[4], 200);
    EXPECT_EQ(pixels[5], 0);

    f.close();
    std::remove(tmpfile.c_str());
}

#include <gtest/gtest.h>
#include "core/camera.h"
#include <cmath>

constexpr double EPS = 1e-4;

class CameraTest : public ::testing::Test {
protected:
    Camera cam;

    void SetUp() override {
        cam.aspect_ratio = 2.0;
        cam.image_width = 4;
        cam.samples_per_pixel = 1;
        cam.max_depth = 10;
        cam.vfov = 90.0;
        cam.lookfrom = Point3(0, 0, 0);
        cam.lookat = Point3(0, 0, -1);
        cam.vup = Vec3(0, 1, 0);
        cam.defocus_angle = 0.0;
        cam.focus_dist = 1.0;
        cam.initialize();
    }
};

TEST_F(CameraTest, ImageDimensions) {
    EXPECT_EQ(cam.get_image_width(), 4);
    EXPECT_EQ(cam.get_image_height(), 2);
}

TEST_F(CameraTest, CenterRayPointsForward) {
    // Sample multiple times; with 1spp the randomness is still there
    // but center pixel should point roughly toward -Z
    // For a 4x2 image, center-ish pixel is (1,0) or (2,1)
    // We test that rays generally point in the -Z direction
    Ray r = cam.get_ray(2, 1);
    Vec3 dir = unit_vector(r.direction());
    EXPECT_LT(dir.z(), 0.0);  // should point toward -Z
}

TEST_F(CameraTest, RayOriginAtCameraPosition) {
    Ray r = cam.get_ray(0, 0);
    // No defocus, origin should be at camera position
    EXPECT_NEAR(r.origin().x(), 0.0, EPS);
    EXPECT_NEAR(r.origin().y(), 0.0, EPS);
    EXPECT_NEAR(r.origin().z(), 0.0, EPS);
}

TEST_F(CameraTest, CornerRaysSpread) {
    Ray top_left = cam.get_ray(0, 0);
    Ray bottom_right = cam.get_ray(3, 1);

    Vec3 d1 = unit_vector(top_left.direction());
    Vec3 d2 = unit_vector(bottom_right.direction());

    // Top-left should point up-left, bottom-right should point down-right
    // They should have different X and Y components
    EXPECT_NE(d1.x(), d2.x());
    EXPECT_NE(d1.y(), d2.y());
}

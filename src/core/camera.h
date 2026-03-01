#ifndef RT_CAMERA_H
#define RT_CAMERA_H

#include "vec3.h"
#include "ray.h"

class Camera {
public:
    // Configuration (set before calling initialize())
    double aspect_ratio = 16.0 / 9.0;
    int image_width = 400;
    int samples_per_pixel = 100;
    int max_depth = 50;

    double vfov = 90.0;        // vertical field of view in degrees
    Point3 lookfrom = Point3(0, 0, 0);
    Point3 lookat = Point3(0, 0, -1);
    Vec3 vup = Vec3(0, 1, 0);

    double defocus_angle = 0.0;  // variation angle of rays through each pixel
    double focus_dist = 10.0;    // distance from lookfrom to the plane of perfect focus

    // Call once after setting configuration
    void initialize();

    // Get image dimensions
    int get_image_width() const { return image_width; }
    int get_image_height() const { return image_height_; }

    // Generate a ray for pixel (i, j) with random sub-pixel offset
    Ray get_ray(int i, int j) const;

private:
    int image_height_;
    Point3 center_;
    Point3 pixel00_loc_;    // location of pixel (0,0)
    Vec3 pixel_delta_u_;    // offset to pixel to the right
    Vec3 pixel_delta_v_;    // offset to pixel below
    Vec3 u_, v_, w_;        // camera basis vectors
    Vec3 defocus_disk_u_;
    Vec3 defocus_disk_v_;

    Vec3 sample_square() const;
    Point3 defocus_disk_sample() const;
};

#endif // RT_CAMERA_H

#include "core/camera.h"
#include "core/random_utils.h"
#include <cmath>
#include <algorithm>

void Camera::initialize() {
    image_height_ = std::max(1, static_cast<int>(image_width / aspect_ratio));

    center_ = lookfrom;

    // Viewport dimensions
    double theta = vfov * 3.14159265358979323846 / 180.0;
    double h = std::tan(theta / 2.0);
    double viewport_height = 2.0 * h * focus_dist;
    double viewport_width = viewport_height * (static_cast<double>(image_width) / image_height_);

    // Camera coordinate frame
    w_ = unit_vector(lookfrom - lookat);
    u_ = unit_vector(cross(vup, w_));
    v_ = cross(w_, u_);

    // Vectors across the horizontal and down the vertical viewport edges
    Vec3 viewport_u = viewport_width * u_;
    Vec3 viewport_v = viewport_height * (-v_);

    // Pixel deltas
    pixel_delta_u_ = viewport_u / static_cast<double>(image_width);
    pixel_delta_v_ = viewport_v / static_cast<double>(image_height_);

    // Upper left pixel location
    Point3 viewport_upper_left = center_ - focus_dist * w_
                                  - viewport_u / 2.0
                                  - viewport_v / 2.0;
    pixel00_loc_ = viewport_upper_left + 0.5 * (pixel_delta_u_ + pixel_delta_v_);

    // Defocus disk basis vectors
    double defocus_radius = focus_dist * std::tan((defocus_angle / 2.0) * 3.14159265358979323846 / 180.0);
    defocus_disk_u_ = u_ * defocus_radius;
    defocus_disk_v_ = v_ * defocus_radius;
}

Ray Camera::get_ray(int i, int j) const {
    Vec3 offset = sample_square();
    Point3 pixel_sample = pixel00_loc_
                         + ((i + offset.x()) * pixel_delta_u_)
                         + ((j + offset.y()) * pixel_delta_v_);

    Point3 ray_origin = (defocus_angle <= 0.0) ? center_ : defocus_disk_sample();
    Vec3 ray_direction = pixel_sample - ray_origin;

    return Ray(ray_origin, ray_direction);
}

Vec3 Camera::sample_square() const {
    return Vec3(random_double() - 0.5, random_double() - 0.5, 0.0);
}

Point3 Camera::defocus_disk_sample() const {
    Vec3 p = random_in_unit_disk();
    return center_ + (p[0] * defocus_disk_u_) + (p[1] * defocus_disk_v_);
}

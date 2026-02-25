#include "core/plane.h"
#include <cmath>

Plane::Plane(const Point3& point, const Vec3& normal, const Material* material)
    : point_(point), normal_(unit_vector(normal)), material_(material) {}

bool Plane::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    double denom = dot(r.direction(), normal_);

    // Ray parallel to plane
    if (std::fabs(denom) < 1e-8)
        return false;

    double t = dot(point_ - r.origin(), normal_) / denom;

    if (!ray_t.surrounds(t))
        return false;

    rec.t = t;
    rec.p = r.at(t);
    rec.set_face_normal(r, normal_);
    rec.material = material_;

    return true;
}

AABB Plane::bounding_box() const {
    // Infinite plane: use a very large bounding box
    // Slightly thicken along normal to avoid zero-volume AABB
    constexpr double big = 1e4;
    constexpr double eps = 0.0001;
    Point3 lo = point_ - Vec3(big, big, big);
    Point3 hi = point_ + Vec3(big, big, big);
    // Ensure non-zero thickness along normal direction
    for (int i = 0; i < 3; ++i) {
        if (std::fabs(normal_[i]) > 0.99) {
            lo.e[i] = point_[i] - eps;
            hi.e[i] = point_[i] + eps;
        }
    }
    return AABB(lo, hi);
}

#include "core/sphere.h"
#include <cmath>

Sphere::Sphere(const Point3& center, double radius, const Material* material)
    : center_(center), radius_(radius), material_(material) {}

bool Sphere::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    Vec3 oc = r.origin() - center_;
    double a = r.direction().length_squared();
    double half_b = dot(oc, r.direction());
    double c = oc.length_squared() - radius_ * radius_;

    double discriminant = half_b * half_b - a * c;
    if (discriminant < 0.0)
        return false;

    double sqrtd = std::sqrt(discriminant);

    // Find nearest root in the acceptable range
    double root = (-half_b - sqrtd) / a;
    if (!ray_t.surrounds(root)) {
        root = (-half_b + sqrtd) / a;
        if (!ray_t.surrounds(root))
            return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    Vec3 outward_normal = (rec.p - center_) / radius_;
    rec.set_face_normal(r, outward_normal);
    rec.material = material_;

    return true;
}

AABB Sphere::bounding_box() const {
    Vec3 rvec(radius_, radius_, radius_);
    return AABB(center_ - rvec, center_ + rvec);
}

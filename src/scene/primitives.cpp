#include "scene/primitives.cuh"
#include <cmath>

// ============================================================
// Sphere
// ============================================================

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
    double inv_a = 1.0 / a;
    double root = (-half_b - sqrtd) * inv_a;
    if (!ray_t.surrounds(root)) {
        root = (-half_b + sqrtd) * inv_a;
        if (!ray_t.surrounds(root))
            return false;
    }

    rec.t = root;
    rec.p = r.at(rec.t);
    Vec3 outward_normal = (rec.p - center_) * (1.0 / radius_);
    rec.set_face_normal(r, outward_normal);
    rec.material = material_;

    return true;
}

AABB Sphere::bounding_box() const {
    Vec3 rvec(radius_, radius_, radius_);
    return AABB(center_ - rvec, center_ + rvec);
}

// ============================================================
// Plane
// ============================================================

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

// ============================================================
// Cylinder (surface quadrique, axe Y)
// Equation : (px - cx)^2 + (pz - cz)^2 = r^2, base_.y <= py <= base_.y + height_
// ============================================================

Cylinder::Cylinder(const Point3& base, double radius, double height, const Material* material)
    : base_(base), radius_(radius), height_(height), material_(material) {}

bool Cylinder::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    // Intersection rayon-cylindre infini (axe Y)
    double ox = r.origin().x() - base_.x();
    double oz = r.origin().z() - base_.z();
    double dx = r.direction().x();
    double dz = r.direction().z();

    double a = dx*dx + dz*dz;
    double half_b = ox*dx + oz*dz;
    double c = ox*ox + oz*oz - radius_*radius_;

    double disc = half_b*half_b - a*c;
    if (disc < 0.0) return false;

    double sqrtd = std::sqrt(disc);
    double root = (-half_b - sqrtd) / a;

    // Tester les deux racines
    for (int i = 0; i < 2; i++) {
        if (i == 1) root = (-half_b + sqrtd) / a;
        if (!ray_t.surrounds(root)) continue;

        double y = r.origin().y() + root * r.direction().y();
        if (y < base_.y() || y > base_.y() + height_) continue;

        rec.t = root;
        rec.p = r.at(root);
        // Normale du cylindre : direction radiale (pas de composante Y)
        Vec3 outward_normal = Vec3(rec.p.x() - base_.x(), 0, rec.p.z() - base_.z()) / radius_;
        rec.set_face_normal(r, outward_normal);
        rec.material = material_;
        return true;
    }
    return false;
}

AABB Cylinder::bounding_box() const {
    return AABB(
        Point3(base_.x() - radius_, base_.y(), base_.z() - radius_),
        Point3(base_.x() + radius_, base_.y() + height_, base_.z() + radius_)
    );
}

// ============================================================
// Triangle (Moller-Trumbore)
// ============================================================

Triangle::Triangle(const Point3& v0, const Point3& v1, const Point3& v2, const Material* material)
    : v0_(v0), v1_(v1), v2_(v2), material_(material) {
    normal_ = unit_vector(cross(v1 - v0, v2 - v0));
}

bool Triangle::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    Vec3 edge1 = v1_ - v0_;
    Vec3 edge2 = v2_ - v0_;
    Vec3 h = cross(r.direction(), edge2);
    double a = dot(edge1, h);

    if (std::fabs(a) < 1e-8)
        return false; // rayon parallele au triangle

    double f = 1.0 / a;
    Vec3 s = r.origin() - v0_;
    double u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;

    Vec3 q = cross(s, edge1);
    double v = f * dot(r.direction(), q);
    if (v < 0.0 || u + v > 1.0)
        return false;

    double t = f * dot(edge2, q);
    if (!ray_t.surrounds(t))
        return false;

    rec.t = t;
    rec.p = r.at(t);
    rec.set_face_normal(r, normal_);
    rec.material = material_;
    return true;
}

AABB Triangle::bounding_box() const {
    Point3 lo(
        std::fmin(v0_.x(), std::fmin(v1_.x(), v2_.x())) - 0.0001,
        std::fmin(v0_.y(), std::fmin(v1_.y(), v2_.y())) - 0.0001,
        std::fmin(v0_.z(), std::fmin(v1_.z(), v2_.z())) - 0.0001
    );
    Point3 hi(
        std::fmax(v0_.x(), std::fmax(v1_.x(), v2_.x())) + 0.0001,
        std::fmax(v0_.y(), std::fmax(v1_.y(), v2_.y())) + 0.0001,
        std::fmax(v0_.z(), std::fmax(v1_.z(), v2_.z())) + 0.0001
    );
    return AABB(lo, hi);
}

// ============================================================
// HittableList
// ============================================================

void HittableList::add(std::shared_ptr<Hittable> object) {
    bbox_ = surrounding_box(bbox_, object->bounding_box());
    objects_.push_back(std::move(object));
}

void HittableList::clear() {
    objects_.clear();
}

bool HittableList::hit(const Ray& r, Interval ray_t, HitRecord& rec) const {
    HitRecord temp_rec;
    bool hit_anything = false;
    double closest_so_far = ray_t.max;

    for (const auto& object : objects_) {
        if (object->hit(r, Interval(ray_t.min, closest_so_far), temp_rec)) {
            hit_anything = true;
            closest_so_far = temp_rec.t;
            rec = temp_rec;
        }
    }

    return hit_anything;
}

AABB HittableList::bounding_box() const {
    return bbox_;
}

#ifndef RT_SCENE_PRIMITIVES_CUH
#define RT_SCENE_PRIMITIVES_CUH

#include "math/vec3.cuh"
#include "math/ray.cuh"
#include "math/interval.cuh"
#include "math/aabb.cuh"
#include <memory>
#include <vector>

class Material;

// --- HitRecord ---

struct HitRecord {
    Point3 p;
    Vec3 normal;
    const Material* material = nullptr;
    double t = 0.0;
    bool front_face = true;

    // Sets normal to always point against the incident ray
    void set_face_normal(const Ray& r, const Vec3& outward_normal) {
        front_face = dot(r.direction(), outward_normal) < 0.0;
        normal = front_face ? outward_normal : -outward_normal;
    }
};

// --- Hittable (abstract base) ---

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const = 0;
    virtual AABB bounding_box() const = 0;
};

// --- Sphere ---

class Sphere : public Hittable {
public:
    Sphere(const Point3& center, double radius, const Material* material);

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

    const Point3& center() const { return center_; }
    double radius() const { return radius_; }

private:
    Point3 center_;
    double radius_;
    const Material* material_;
};

// --- Plane ---

class Plane : public Hittable {
public:
    Plane(const Point3& point, const Vec3& normal, const Material* material);

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

private:
    Point3 point_;
    Vec3 normal_;   // stored as unit vector
    const Material* material_;
};

// --- Cylinder (surface quadrique, axe Y, hauteur finie) ---

class Cylinder : public Hittable {
public:
    Cylinder(const Point3& base, double radius, double height, const Material* material);

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

private:
    Point3 base_;
    double radius_;
    double height_;
    const Material* material_;
};

// --- Triangle (Moller-Trumbore intersection) ---

class Triangle : public Hittable {
public:
    Triangle(const Point3& v0, const Point3& v1, const Point3& v2, const Material* material);

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

private:
    Point3 v0_, v1_, v2_;
    Vec3 normal_;
    const Material* material_;
};

// --- HittableList ---

class HittableList : public Hittable {
public:
    HittableList() = default;

    void add(std::shared_ptr<Hittable> object);
    void clear();

    bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const override;
    AABB bounding_box() const override;

    const std::vector<std::shared_ptr<Hittable>>& objects() const { return objects_; }

private:
    std::vector<std::shared_ptr<Hittable>> objects_;
    AABB bbox_;
};

#endif // RT_SCENE_PRIMITIVES_CUH

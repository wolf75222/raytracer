#ifndef RT_SPHERE_H
#define RT_SPHERE_H

#include "hittable.h"
#include "vec3.h"

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

#endif // RT_SPHERE_H

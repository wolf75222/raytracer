#ifndef RT_PLANE_H
#define RT_PLANE_H

#include "hittable.h"
#include "vec3.h"

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

#endif // RT_PLANE_H

#ifndef RT_HIT_RECORD_H
#define RT_HIT_RECORD_H

#include "vec3.h"
#include "ray.h"

class Material;

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

#endif // RT_HIT_RECORD_H

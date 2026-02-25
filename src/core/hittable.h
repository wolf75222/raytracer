#ifndef RT_HITTABLE_H
#define RT_HITTABLE_H

#include "ray.h"
#include "interval.h"
#include "hit_record.h"
#include "aabb.h"

class Hittable {
public:
    virtual ~Hittable() = default;

    virtual bool hit(const Ray& r, Interval ray_t, HitRecord& rec) const = 0;
    virtual AABB bounding_box() const = 0;
};

#endif // RT_HITTABLE_H

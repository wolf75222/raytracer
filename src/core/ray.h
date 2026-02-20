#ifndef RT_RAY_H
#define RT_RAY_H

#include "vec3.h"

class Ray {
public:
    Point3 orig;
    Vec3 dir;
    Vec3 inv_dir;       // precomputed 1/direction (Williams et al.)
    int dir_sign[3];    // 0 if positive, 1 if negative

    RT_HOSTDEV Ray() {}
    RT_HOSTDEV Ray(const Point3& origin, const Vec3& direction)
        : orig(origin), dir(direction)
    {
        inv_dir = Vec3(1.0 / direction.x(), 1.0 / direction.y(), 1.0 / direction.z());
        dir_sign[0] = (inv_dir.x() < 0) ? 1 : 0;
        dir_sign[1] = (inv_dir.y() < 0) ? 1 : 0;
        dir_sign[2] = (inv_dir.z() < 0) ? 1 : 0;
    }

    RT_HOSTDEV const Point3& origin() const { return orig; }
    RT_HOSTDEV const Vec3& direction() const { return dir; }

    RT_HOSTDEV Point3 at(double t) const {
        return orig + t * dir;
    }
};

#endif // RT_RAY_H

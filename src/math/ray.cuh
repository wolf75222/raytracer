// ray.cuh - Rayon parametrique P(t) = origin + t * direction.
// Precalcule inv_dir et dir_sign a la construction pour accelerer
// le test slab AABB (Williams et al. 2005) : on evite 6 divisions par
// test d'intersection boite, critique dans la traversee BVH.

#ifndef RT_MATH_RAY_CUH
#define RT_MATH_RAY_CUH

#include "math/vec3.cuh"

class Ray {
public:
    Point3 orig;
    Vec3 dir;
    Vec3 inv_dir;       // 1/direction, precalcule (Williams et al.)
    int dir_sign[3];    // 0 si positif, 1 si negatif : indexation directe min/max AABB

    RT_HOSTDEV Ray() {}
    RT_HOSTDEV Ray(const Point3& origin, const Vec3& direction)
        : orig(origin), dir(direction)
    {
        // Precalcul une seule fois par rayon, reutilise a chaque noeud BVH
        inv_dir = Vec3(1.0 / direction.x(), 1.0 / direction.y(), 1.0 / direction.z());
        dir_sign[0] = (inv_dir.x() < 0) ? 1 : 0;
        dir_sign[1] = (inv_dir.y() < 0) ? 1 : 0;
        dir_sign[2] = (inv_dir.z() < 0) ? 1 : 0;
    }

    RT_HOSTDEV const Point3& origin() const { return orig; }
    RT_HOSTDEV const Vec3& direction() const { return dir; }

    // Point le long du rayon a la distance t
    RT_HOSTDEV Point3 at(double t) const {
        return orig + t * dir;
    }
};

#endif // RT_MATH_RAY_CUH

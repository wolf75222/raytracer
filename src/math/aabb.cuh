// aabb.cuh - Axis-Aligned Bounding Box pour l'acceleration BVH.
// Chaque noeud BVH stocke une AABB ; la traversee teste le rayon
// contre la boite avant de descendre dans les enfants.
// Le test slab (Williams et al. 2005) utilise inv_dir precalcule
// dans Ray pour transformer 6 divisions en 6 multiplications.

#ifndef RT_MATH_AABB_CUH
#define RT_MATH_AABB_CUH

#include "math/vec3.cuh"
#include "math/ray.cuh"
#include "math/interval.cuh"
#include <algorithm>
#include <cmath>

class AABB {
public:
    Interval x, y, z; // bornes sur chaque axe

    RT_HOSTDEV AABB() {} // intervalles vides par defaut

    RT_HOSTDEV AABB(const Interval& x, const Interval& y, const Interval& z)
        : x(x), y(y), z(z) {}

    // Construction depuis deux coins opposes (ordre quelconque)
    RT_HOSTDEV AABB(const Point3& a, const Point3& b) {
        x = (a[0] <= b[0]) ? Interval(a[0], b[0]) : Interval(b[0], a[0]);
        y = (a[1] <= b[1]) ? Interval(a[1], b[1]) : Interval(b[1], a[1]);
        z = (a[2] <= b[2]) ? Interval(a[2], b[2]) : Interval(b[2], a[2]);
    }

    // Acces generique par indice d'axe (0=x, 1=y, 2=z)
    // Utilise par le BVH pour choisir l'axe de split
    RT_HOSTDEV const Interval& axis_interval(int n) const {
        if (n == 1) return y;
        if (n == 2) return z;
        return x;
    }

    RT_HOSTDEV Point3 min_point() const { return Point3(x.min, y.min, z.min); }
    RT_HOSTDEV Point3 max_point() const { return Point3(x.max, y.max, z.max); }

    // Test slab de Williams et al. : intersection rayon-boite en 6 mul + 6 cmp.
    // inv_dir precalcule dans Ray transforme (bound - origin) / dir en multiplication.
    // On resserre progressivement [t_min, t_max] par axe ; si l'intervalle
    // devient vide (t_max < t_min), le rayon rate la boite -> early exit.
    RT_HOSTDEV bool hit(const Ray& r, double t_min, double t_max) const {
        const Point3& ro = r.orig;
        const Vec3& inv = r.inv_dir;

        // Axe X
        double tx1 = (x.min - ro[0]) * inv[0];
        double tx2 = (x.max - ro[0]) * inv[0];
        if (tx1 > tx2) { double tmp = tx1; tx1 = tx2; tx2 = tmp; }
        if (tx1 > t_min) t_min = tx1;
        if (tx2 < t_max) t_max = tx2;
        if (t_max < t_min) return false;

        // Axe Y
        double ty1 = (y.min - ro[1]) * inv[1];
        double ty2 = (y.max - ro[1]) * inv[1];
        if (ty1 > ty2) { double tmp = ty1; ty1 = ty2; ty2 = tmp; }
        if (ty1 > t_min) t_min = ty1;
        if (ty2 < t_max) t_max = ty2;
        if (t_max < t_min) return false;

        // Axe Z
        double tz1 = (z.min - ro[2]) * inv[2];
        double tz2 = (z.max - ro[2]) * inv[2];
        if (tz1 > tz2) { double tmp = tz1; tz1 = tz2; tz2 = tmp; }
        if (tz1 > t_min) t_min = tz1;
        if (tz2 < t_max) t_max = tz2;
        if (t_max < t_min) return false;

        return true;
    }

    // Surcharge pour les tests qui passent un Interval
    RT_HOSTDEV bool hit(const Ray& r, Interval ray_t) const {
        return hit(r, ray_t.min, ray_t.max);
    }

    // Surface totale de la boite : utilisee par la SAH (Surface Area Heuristic)
    // pour estimer le cout de split lors de la construction du BVH.
    // Cout ~ surface_enfant / surface_parent * nb_primitives_enfant
    RT_HOSTDEV double surface_area() const {
        double dx = x.size(), dy = y.size(), dz = z.size();
        return 2.0 * (dx*dy + dy*dz + dz*dx);
    }

    // Axe le plus long : heuristique de split quand SAH n'est pas utilisee
    RT_HOSTDEV int longest_axis() const {
        double sx = x.size(), sy = y.size(), sz = z.size();
        if (sx > sy) return (sx > sz) ? 0 : 2;
        return (sy > sz) ? 1 : 2;
    }

    static const AABB empty;
    static const AABB universe;
};

inline const AABB AABB::empty = AABB(Interval::empty, Interval::empty, Interval::empty);
inline const AABB AABB::universe = AABB(Interval::universe, Interval::universe, Interval::universe);

// Union de deux AABB : construit la plus petite boite englobant les deux.
// Utilise recursivement lors de la construction bottom-up du BVH.
inline RT_HOSTDEV AABB surrounding_box(const AABB& a, const AABB& b) {
    return AABB(
        Interval(std::fmin(a.x.min, b.x.min), std::fmax(a.x.max, b.x.max)),
        Interval(std::fmin(a.y.min, b.y.min), std::fmax(a.y.max, b.y.max)),
        Interval(std::fmin(a.z.min, b.z.min), std::fmax(a.z.max, b.z.max))
    );
}

#endif // RT_MATH_AABB_CUH

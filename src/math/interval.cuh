// interval.cuh - Intervalle ferme [min, max] sur les reels.
// Utilise partout : parametrage t valide des rayons, bornes AABB,
// clamping des couleurs avant tone mapping.

#ifndef RT_MATH_INTERVAL_CUH
#define RT_MATH_INTERVAL_CUH

#include <cmath>
#include <limits>

#include "math/vec3.cuh" // pour RT_HOSTDEV

class Interval {
public:
    double min, max;

    // Par defaut : intervalle vide (min > max, aucun t valide)
    RT_HOSTDEV Interval()
        : min(+std::numeric_limits<double>::infinity()),
          max(-std::numeric_limits<double>::infinity()) {}

    RT_HOSTDEV Interval(double min, double max) : min(min), max(max) {}

    RT_HOSTDEV double size() const { return max - min; }

    // Inclusion large (bornes incluses)
    RT_HOSTDEV bool contains(double x) const { return min <= x && x <= max; }

    // Inclusion stricte (bornes exclues) : utilise pour ignorer les self-intersections
    // a t~0 (shadow acne) en testant t dans ]epsilon, +inf[
    RT_HOSTDEV bool surrounds(double x) const { return min < x && x < max; }

    RT_HOSTDEV double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const Interval empty;
    static const Interval universe;
};

// Singletons inline (C++17) : evitent les problemes d'ordre d'initialisation
inline const Interval Interval::empty    = Interval(+std::numeric_limits<double>::infinity(),
                                                     -std::numeric_limits<double>::infinity());
inline const Interval Interval::universe = Interval(-std::numeric_limits<double>::infinity(),
                                                     +std::numeric_limits<double>::infinity());

#endif // RT_MATH_INTERVAL_CUH

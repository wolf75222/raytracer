#ifndef RT_INTERVAL_H
#define RT_INTERVAL_H

#include <cmath>
#include <limits>

class Interval {
public:
    double min, max;

    RT_HOSTDEV Interval()
        : min(+std::numeric_limits<double>::infinity()),
          max(-std::numeric_limits<double>::infinity()) {} // empty by default

    RT_HOSTDEV Interval(double min, double max) : min(min), max(max) {}

    RT_HOSTDEV double size() const { return max - min; }

    RT_HOSTDEV bool contains(double x) const { return min <= x && x <= max; }

    RT_HOSTDEV bool surrounds(double x) const { return min < x && x < max; }

    RT_HOSTDEV double clamp(double x) const {
        if (x < min) return min;
        if (x > max) return max;
        return x;
    }

    static const Interval empty;
    static const Interval universe;
};

inline const Interval Interval::empty    = Interval(+std::numeric_limits<double>::infinity(),
                                                     -std::numeric_limits<double>::infinity());
inline const Interval Interval::universe = Interval(-std::numeric_limits<double>::infinity(),
                                                     +std::numeric_limits<double>::infinity());

#endif // RT_INTERVAL_H

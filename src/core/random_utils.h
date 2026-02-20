#ifndef RT_RANDOM_UTILS_H
#define RT_RANDOM_UTILS_H

#include "vec3.h"
#include <cstdint>

// xoshiro256+ : extremely fast, high quality PRNG (Blackman & Vigna)
// 32 bytes state vs 2.5KB for mt19937, ~4x faster per call
struct Xoshiro256Plus {
    uint64_t s[4];

    Xoshiro256Plus() {
        // Seed from address entropy + counter
        uint64_t seed = reinterpret_cast<uint64_t>(this);
        seed ^= static_cast<uint64_t>(__LINE__) * 6364136223846793005ULL;
        s[0] = splitmix64(seed);
        s[1] = splitmix64(seed);
        s[2] = splitmix64(seed);
        s[3] = splitmix64(seed);
    }

    static uint64_t splitmix64(uint64_t& state) {
        uint64_t z = (state += 0x9e3779b97f4a7c15ULL);
        z = (z ^ (z >> 30)) * 0xbf58476d1ce4e5b9ULL;
        z = (z ^ (z >> 27)) * 0x94d049bb133111ebULL;
        return z ^ (z >> 31);
    }

    static uint64_t rotl(uint64_t x, int k) {
        return (x << k) | (x >> (64 - k));
    }

    uint64_t next() {
        const uint64_t result = s[0] + s[3];
        const uint64_t t = s[1] << 17;
        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);
        return result;
    }

    // [0, 1) as double
    double next_double() {
        return (next() >> 11) * 0x1.0p-53;
    }
};

// Thread-local fast RNG
inline Xoshiro256Plus& get_rng() {
    thread_local Xoshiro256Plus rng;
    return rng;
}

inline double random_double() {
    return get_rng().next_double();
}

inline double random_double(double min, double max) {
    return min + (max - min) * random_double();
}

inline Vec3 random_vec3() {
    return Vec3(random_double(), random_double(), random_double());
}

inline Vec3 random_vec3(double min, double max) {
    return Vec3(random_double(min, max), random_double(min, max), random_double(min, max));
}

inline Vec3 random_in_unit_sphere() {
    while (true) {
        Vec3 p = random_vec3(-1.0, 1.0);
        if (p.length_squared() < 1.0)
            return p;
    }
}

inline Vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

inline Vec3 random_on_hemisphere(const Vec3& normal) {
    Vec3 on_sphere = random_unit_vector();
    if (dot(on_sphere, normal) > 0.0)
        return on_sphere;
    return -on_sphere;
}

inline Vec3 random_in_unit_disk() {
    while (true) {
        Vec3 p(random_double(-1.0, 1.0), random_double(-1.0, 1.0), 0.0);
        if (p.length_squared() < 1.0)
            return p;
    }
}

#endif // RT_RANDOM_UTILS_H

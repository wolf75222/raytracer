// vec3.cuh - Vecteur 3D double precision, brique de base du path tracer.
// Header-only : force l'inlining partout (hot path intersections + shading).
// RT_HOSTDEV permet la compilation croisee CPU (MSVC) / GPU (nvcc).

#ifndef RT_MATH_VEC3_CUH
#define RT_MATH_VEC3_CUH

#include <cmath>
#include <iostream>

// Macro de portabilite CPU/GPU : declenche __host__ __device__ uniquement
// quand nvcc compile le fichier, transparent pour MSVC.
#ifdef __CUDACC__
#define RT_HOSTDEV __host__ __device__
#else
#define RT_HOSTDEV
#endif

class Vec3 {
public:
    double e[3]; // stockage brut, pas de padding

    RT_HOSTDEV Vec3() : e{0.0, 0.0, 0.0} {}
    RT_HOSTDEV Vec3(double x, double y, double z) : e{x, y, z} {}

    RT_HOSTDEV double x() const { return e[0]; }
    RT_HOSTDEV double y() const { return e[1]; }
    RT_HOSTDEV double z() const { return e[2]; }

    RT_HOSTDEV Vec3 operator-() const { return Vec3(-e[0], -e[1], -e[2]); }
    RT_HOSTDEV double operator[](int i) const { return e[i]; }
    RT_HOSTDEV double& operator[](int i) { return e[i]; }

    RT_HOSTDEV Vec3& operator+=(const Vec3& v) {
        e[0] += v.e[0]; e[1] += v.e[1]; e[2] += v.e[2];
        return *this;
    }

    RT_HOSTDEV Vec3& operator-=(const Vec3& v) {
        e[0] -= v.e[0]; e[1] -= v.e[1]; e[2] -= v.e[2];
        return *this;
    }

    RT_HOSTDEV Vec3& operator*=(double t) {
        e[0] *= t; e[1] *= t; e[2] *= t;
        return *this;
    }

    // Multiplication par l'inverse : evite 3 divisions
    RT_HOSTDEV Vec3& operator/=(double t) {
        return *this *= (1.0 / t);
    }

    RT_HOSTDEV double length_squared() const {
        return e[0]*e[0] + e[1]*e[1] + e[2]*e[2];
    }

    RT_HOSTDEV double length() const {
        return std::sqrt(length_squared());
    }

    // Detection de vecteur quasi-nul apres scatter :
    // evite les NaN en normalisant un vecteur degenere
    RT_HOSTDEV bool near_zero() const {
        constexpr double s = 1e-8;
        return (std::fabs(e[0]) < s) && (std::fabs(e[1]) < s) && (std::fabs(e[2]) < s);
    }
};

// Alias semantiques : meme type sous-jacent, intention differente
using Point3 = Vec3;
using Color = Vec3;

// --- Operateurs arithmetiques ---

inline RT_HOSTDEV Vec3 operator+(const Vec3& u, const Vec3& v) {
    return Vec3(u.e[0]+v.e[0], u.e[1]+v.e[1], u.e[2]+v.e[2]);
}

inline RT_HOSTDEV Vec3 operator-(const Vec3& u, const Vec3& v) {
    return Vec3(u.e[0]-v.e[0], u.e[1]-v.e[1], u.e[2]-v.e[2]);
}

// Produit composante par composante (Hadamard) :
// sert a moduler la couleur par l'albedo du materiau
inline RT_HOSTDEV Vec3 operator*(const Vec3& u, const Vec3& v) {
    return Vec3(u.e[0]*v.e[0], u.e[1]*v.e[1], u.e[2]*v.e[2]);
}

inline RT_HOSTDEV Vec3 operator*(double t, const Vec3& v) {
    return Vec3(t*v.e[0], t*v.e[1], t*v.e[2]);
}

inline RT_HOSTDEV Vec3 operator*(const Vec3& v, double t) {
    return t * v;
}

inline RT_HOSTDEV Vec3 operator/(const Vec3& v, double t) {
    return (1.0/t) * v;
}

// --- Fonctions geometriques ---

inline RT_HOSTDEV double dot(const Vec3& u, const Vec3& v) {
    return u.e[0]*v.e[0] + u.e[1]*v.e[1] + u.e[2]*v.e[2];
}

inline RT_HOSTDEV Vec3 cross(const Vec3& u, const Vec3& v) {
    return Vec3(
        u.e[1]*v.e[2] - u.e[2]*v.e[1],
        u.e[2]*v.e[0] - u.e[0]*v.e[2],
        u.e[0]*v.e[1] - u.e[1]*v.e[0]
    );
}

inline RT_HOSTDEV Vec3 unit_vector(const Vec3& v) {
    return v / v.length();
}

// Reflexion speculaire : v incident, n normale unitaire
inline RT_HOSTDEV Vec3 reflect(const Vec3& v, const Vec3& n) {
    return v - 2.0 * dot(v, n) * n;
}

// Refraction via loi de Snell-Descartes.
// uv doit etre unitaire. etai_over_etat = n1/n2 (indices de refraction).
// Decomposition en composante perpendiculaire + parallele a la normale.
inline RT_HOSTDEV Vec3 refract(const Vec3& uv, const Vec3& n, double etai_over_etat) {
    double cos_theta = std::fmin(dot(-uv, n), 1.0);
    Vec3 r_out_perp = etai_over_etat * (uv + cos_theta * n);
    Vec3 r_out_parallel = -std::sqrt(std::fabs(1.0 - r_out_perp.length_squared())) * n;
    return r_out_perp + r_out_parallel;
}

inline std::ostream& operator<<(std::ostream& out, const Vec3& v) {
    return out << v.e[0] << ' ' << v.e[1] << ' ' << v.e[2];
}

#endif // RT_MATH_VEC3_CUH

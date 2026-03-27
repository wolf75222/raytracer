// random.cuh - Fonctions aleatoires CPU pour le Monte Carlo.
// thread_local : chaque thread du pool CPU a son propre generateur,
// pas de mutex ni contention. Le GPU utilise curand a la place (gpu_renderer.cu).

#ifndef RT_MATH_RANDOM_CUH
#define RT_MATH_RANDOM_CUH

#include "math/vec3.cuh"
#include <random>

// Mersenne Twister 19937 : periode 2^19937-1, qualite statistique suffisante
// pour le path tracing. Seed via random_device (entropie hardware si disponible).
inline double random_double() {
    thread_local std::mt19937 gen(std::random_device{}());
    thread_local std::uniform_real_distribution<double> dist(0.0, 1.0);
    return dist(gen);
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

// Rejection sampling dans le cube [-1,1]^3 : ~52% d'acceptance.
// Plus simple et aussi rapide que Box-Muller pour 3 composantes.
inline Vec3 random_in_unit_sphere() {
    while (true) {
        Vec3 p = random_vec3(-1.0, 1.0);
        if (p.length_squared() < 1.0)
            return p;
    }
}

// Direction aleatoire uniforme sur la sphere unite :
// normaliser un point uniforme dans la boule donne une distribution
// uniforme sur la surface (distribution de Lambert pour le diffus).
inline Vec3 random_unit_vector() {
    return unit_vector(random_in_unit_sphere());
}

// Hemisphere cosinus-pondere implicite : si le vecteur pointe
// du mauvais cote de la normale, on le retourne.
inline Vec3 random_on_hemisphere(const Vec3& normal) {
    Vec3 on_sphere = random_unit_vector();
    if (dot(on_sphere, normal) > 0.0)
        return on_sphere;
    return -on_sphere;
}

// Disque unite : utilise pour le depth of field (DOF).
// L'ouverture de la lentille est echantillonnee uniformement sur un disque.
inline Vec3 random_in_unit_disk() {
    while (true) {
        Vec3 p(random_double(-1.0, 1.0), random_double(-1.0, 1.0), 0.0);
        if (p.length_squared() < 1.0)
            return p;
    }
}

#endif // RT_MATH_RANDOM_CUH

#include "scene/material.cuh"
#include "scene/primitives.cuh"
#include "math/random.cuh"
#include <cmath>
#include <algorithm>

// ============================================================
// Lambertian
// ============================================================

Lambertian::Lambertian(const Color& albedo) : albedo_(albedo) {
    type_ = Type::Lambertian;
}

bool Lambertian::scatter(const Ray& /*r_in*/, const HitRecord& rec,
                          Color& attenuation, Ray& scattered) const {
    Vec3 scatter_direction = rec.normal + random_unit_vector();

    // Catch degenerate scatter direction
    if (scatter_direction.near_zero())
        scatter_direction = rec.normal;

    scattered = Ray(rec.p, scatter_direction);
    attenuation = albedo_;
    return true;
}

// ============================================================
// Metal
// ============================================================

Metal::Metal(const Color& albedo, double fuzz)
    : albedo_(albedo), fuzz_(std::min(fuzz, 1.0)) {
    type_ = Type::Metal;
}

bool Metal::scatter(const Ray& r_in, const HitRecord& rec,
                     Color& attenuation, Ray& scattered) const {
    Vec3 reflected = reflect(unit_vector(r_in.direction()), rec.normal);
    reflected = reflected + fuzz_ * random_in_unit_sphere();
    scattered = Ray(rec.p, reflected);
    attenuation = albedo_;
    return (dot(scattered.direction(), rec.normal) > 0.0);
}

// ============================================================
// Dielectric
// ============================================================

Dielectric::Dielectric(double refraction_index)
    : refraction_index_(refraction_index) {
    type_ = Type::Dielectric;
}

bool Dielectric::scatter(const Ray& r_in, const HitRecord& rec,
                          Color& attenuation, Ray& scattered) const {
    attenuation = Color(1.0, 1.0, 1.0);
    double ri = rec.front_face ? (1.0 / refraction_index_) : refraction_index_;

    Vec3 unit_dir = unit_vector(r_in.direction());
    double cos_theta = std::fmin(dot(-unit_dir, rec.normal), 1.0);
    double sin_theta = std::sqrt(1.0 - cos_theta * cos_theta);

    bool cannot_refract = ri * sin_theta > 1.0;
    Vec3 direction;

    if (cannot_refract || reflectance(cos_theta, ri) > random_double())
        direction = reflect(unit_dir, rec.normal);
    else
        direction = refract(unit_dir, rec.normal, ri);

    scattered = Ray(rec.p, direction);
    return true;
}

double Dielectric::reflectance(double cosine, double refraction_index) {
    // Schlick's approximation
    double r0 = (1.0 - refraction_index) / (1.0 + refraction_index);
    r0 = r0 * r0;
    double omc = 1.0 - cosine;
    double omc2 = omc * omc;
    return r0 + (1.0 - r0) * omc2 * omc2 * omc;
}

// ============================================================
// Checker (damier procedural)
// ============================================================

Checker::Checker(const Color& color1, const Color& color2, double scale)
    : color1_(color1), color2_(color2), scale_(scale) {
    type_ = Type::Checker;
}

bool Checker::scatter(const Ray& /*r_in*/, const HitRecord& rec,
                       Color& attenuation, Ray& scattered) const {
    // Pattern damier base sur la position du hit
    double inv_scale = 1.0 / scale_;
    int ix = (int)std::floor(rec.p.x() * inv_scale);
    int iy = (int)std::floor(rec.p.y() * inv_scale);
    int iz = (int)std::floor(rec.p.z() * inv_scale);
    bool is_even = ((ix + iy + iz) % 2 == 0);

    attenuation = is_even ? color1_ : color2_;

    Vec3 scatter_direction = rec.normal + random_unit_vector();
    if (scatter_direction.near_zero()) scatter_direction = rec.normal;
    scattered = Ray(rec.p, scatter_direction);
    return true;
}

// ============================================================
// Emissive (area light)
// ============================================================

Emissive::Emissive(const Color& emit_color, double intensity)
    : emit_(emit_color), intensity_(intensity) {
    type_ = Type::Emissive;
}

bool Emissive::scatter(const Ray& /*r_in*/, const HitRecord& /*rec*/,
                        Color& /*attenuation*/, Ray& /*scattered*/) const {
    return false; // emissive ne scatter pas, juste emet
}

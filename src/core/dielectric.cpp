#include "core/dielectric.h"
#include "core/hit_record.h"
#include "core/random_utils.h"
#include <cmath>

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
    return r0 + (1.0 - r0) * std::pow(1.0 - cosine, 5.0);
}

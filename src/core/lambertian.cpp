#include "core/lambertian.h"
#include "core/hit_record.h"
#include "core/random_utils.h"

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

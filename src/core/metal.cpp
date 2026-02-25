#include "core/metal.h"
#include "core/hit_record.h"
#include "core/random_utils.h"
#include <algorithm>

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

#ifndef RT_LAMBERTIAN_H
#define RT_LAMBERTIAN_H

#include "material.h"

class Lambertian : public Material {
public:
    explicit Lambertian(const Color& albedo);

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override;

    const Color& albedo() const { return albedo_; }

private:
    Color albedo_;
};

#endif // RT_LAMBERTIAN_H

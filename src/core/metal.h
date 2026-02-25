#ifndef RT_METAL_H
#define RT_METAL_H

#include "material.h"

class Metal : public Material {
public:
    Metal(const Color& albedo, double fuzz);

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override;

    const Color& albedo() const { return albedo_; }
    double fuzz() const { return fuzz_; }

private:
    Color albedo_;
    double fuzz_;
};

#endif // RT_METAL_H

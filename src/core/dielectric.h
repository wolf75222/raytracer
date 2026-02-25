#ifndef RT_DIELECTRIC_H
#define RT_DIELECTRIC_H

#include "material.h"

class Dielectric : public Material {
public:
    explicit Dielectric(double refraction_index);

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override;

    double refraction_index() const { return refraction_index_; }

    // Schlick's approximation for reflectance (public for devirtualized scatter)
    static double reflectance(double cosine, double refraction_index);

private:
    double refraction_index_;
};

#endif // RT_DIELECTRIC_H

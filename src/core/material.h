#ifndef RT_MATERIAL_H
#define RT_MATERIAL_H

#include "vec3.h"
#include "ray.h"

struct HitRecord; // forward declaration

class Material {
public:
    // Type tag pour dispatch rapide sans vtable
    enum class Type : uint8_t { Lambertian, Metal, Dielectric, Unknown };

    virtual ~Material() = default;

    virtual bool scatter(const Ray& r_in, const HitRecord& rec,
                         Color& attenuation, Ray& scattered) const = 0;

    Type type() const { return type_; }

protected:
    Type type_ = Type::Unknown;
};

#endif // RT_MATERIAL_H

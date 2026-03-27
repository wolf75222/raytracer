#ifndef RT_SCENE_MATERIAL_CUH
#define RT_SCENE_MATERIAL_CUH

#include "math/vec3.cuh"
#include "math/ray.cuh"
#include <cstdint>

struct HitRecord; // forward declaration

// --- Base Material class ---

class Material {
public:
    // Type tag pour dispatch rapide sans vtable
    enum class Type : uint8_t { Lambertian, Metal, Dielectric, Checker, Emissive, Unknown };

    virtual ~Material() = default;

    virtual bool scatter(const Ray& r_in, const HitRecord& rec,
                         Color& attenuation, Ray& scattered) const = 0;

    Type type() const { return type_; }

protected:
    Type type_ = Type::Unknown;
};

// --- Lambertian ---

class Lambertian : public Material {
public:
    explicit Lambertian(const Color& albedo);

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override;

    const Color& albedo() const { return albedo_; }

private:
    Color albedo_;
};

// --- Metal ---

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

// --- Dielectric ---

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

// --- Checker (damier procedural) ---

class Checker : public Material {
public:
    Checker(const Color& color1, const Color& color2, double scale = 1.0);

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override;

    const Color& color1() const { return color1_; }
    const Color& color2() const { return color2_; }
    double scale() const { return scale_; }

private:
    Color color1_, color2_;
    double scale_;
};

// --- Emissive (area light) ---

class Emissive : public Material {
public:
    Emissive(const Color& emit_color, double intensity = 1.0);

    bool scatter(const Ray& r_in, const HitRecord& rec,
                 Color& attenuation, Ray& scattered) const override;

    Color emitted() const { return emit_ * intensity_; }
    const Color& emit_color() const { return emit_; }
    double intensity() const { return intensity_; }

private:
    Color emit_;
    double intensity_;
};

#endif // RT_SCENE_MATERIAL_CUH

#include <gtest/gtest.h>
#include "scene/material.cuh"
#include "scene/primitives.cuh"
#include <cmath>

constexpr double EPS = 1e-6;

// Helper to create a basic hit record
HitRecord make_hit(const Material* mat) {
    HitRecord rec;
    rec.p = Point3(0, 0, 0);
    rec.normal = Vec3(0, 1, 0);
    rec.t = 1.0;
    rec.front_face = true;
    rec.material = mat;
    return rec;
}

TEST(Lambertian, AlwaysScatters) {
    Lambertian mat(Color(0.5, 0.5, 0.5));
    HitRecord rec = make_hit(&mat);
    Ray r_in(Point3(0, 1, 0), Vec3(0, -1, 0));

    Color attenuation;
    Ray scattered;
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(mat.scatter(r_in, rec, attenuation, scattered));
        EXPECT_NEAR(attenuation.x(), 0.5, EPS);
        EXPECT_NEAR(attenuation.y(), 0.5, EPS);
        EXPECT_NEAR(attenuation.z(), 0.5, EPS);
    }
}

TEST(Lambertian, ScatterDirectionInHemisphere) {
    Lambertian mat(Color(1, 1, 1));
    HitRecord rec = make_hit(&mat);
    Ray r_in(Point3(0, 1, 0), Vec3(0, -1, 0));

    for (int i = 0; i < 100; ++i) {
        Color attenuation;
        Ray scattered;
        mat.scatter(r_in, rec, attenuation, scattered);
        // Scattered direction should not be zero-length
        EXPECT_GT(scattered.direction().length(), 1e-8);
    }
}

TEST(Metal, PerfectReflection) {
    Metal mat(Color(0.8, 0.8, 0.8), 0.0);
    HitRecord rec = make_hit(&mat);
    // Ray coming in at 45 degrees
    Ray r_in(Point3(-1, 1, 0), unit_vector(Vec3(1, -1, 0)));

    Color attenuation;
    Ray scattered;
    bool did_scatter = mat.scatter(r_in, rec, attenuation, scattered);
    EXPECT_TRUE(did_scatter);

    // Reflected ray should go up at 45 degrees
    Vec3 dir = unit_vector(scattered.direction());
    EXPECT_NEAR(dir.y(), std::fabs(dir.x()), 0.01);
    EXPECT_GT(dir.y(), 0.0);
}

TEST(Metal, FuzzClamped) {
    // Fuzz > 1 should be clamped to 1
    Metal mat(Color(0.8, 0.8, 0.8), 5.0);
    HitRecord rec = make_hit(&mat);
    Ray r_in(Point3(-1, 1, 0), unit_vector(Vec3(1, -1, 0)));

    Color attenuation;
    Ray scattered;
    // Should still work, just very fuzzy
    mat.scatter(r_in, rec, attenuation, scattered);
}

TEST(Dielectric, AlwaysScatters) {
    Dielectric mat(1.5);
    HitRecord rec = make_hit(&mat);
    Ray r_in(Point3(0, 1, 0), Vec3(0, -1, 0));

    Color attenuation;
    Ray scattered;
    for (int i = 0; i < 50; ++i) {
        EXPECT_TRUE(mat.scatter(r_in, rec, attenuation, scattered));
        // Attenuation should be white (no absorption)
        EXPECT_NEAR(attenuation.x(), 1.0, EPS);
        EXPECT_NEAR(attenuation.y(), 1.0, EPS);
        EXPECT_NEAR(attenuation.z(), 1.0, EPS);
    }
}

TEST(Dielectric, NormalIncidenceRefraction) {
    Dielectric mat(1.5);
    HitRecord rec = make_hit(&mat);
    // Ray straight down
    Ray r_in(Point3(0, 1, 0), Vec3(0, -1, 0));

    Color attenuation;
    Ray scattered;
    mat.scatter(r_in, rec, attenuation, scattered);

    // At normal incidence, direction should still be roughly downward
    EXPECT_LT(scattered.direction().y(), 0.0);
}

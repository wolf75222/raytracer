#include <gtest/gtest.h>
#include "scene/primitives.cuh"
#include "scene/material.cuh"
#include "math/interval.cuh"
#include <cmath>
#include <limits>

constexpr double EPS = 1e-6;

class SphereTest : public ::testing::Test {
protected:
    Lambertian mat{Color(0.5, 0.5, 0.5)};
    Sphere sphere{Point3(0, 0, -1), 0.5, &mat};
};

TEST_F(SphereTest, HitFrontOn) {
    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(sphere.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 0.5, EPS);
    EXPECT_NEAR(rec.p.z(), -0.5, EPS);
    EXPECT_TRUE(rec.front_face);
    // Normal should point towards ray (outward)
    EXPECT_NEAR(rec.normal.z(), 1.0, EPS);
}

TEST_F(SphereTest, Miss) {
    Ray r(Point3(0, 10, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_FALSE(sphere.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(SphereTest, HitFromInside) {
    // Ray originating inside the sphere
    Ray r(Point3(0, 0, -1), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(sphere.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_FALSE(rec.front_face);
}

TEST_F(SphereTest, HitBehindRay) {
    // Sphere behind the ray origin
    Ray r(Point3(0, 0, 0), Vec3(0, 0, 1));
    HitRecord rec;
    EXPECT_FALSE(sphere.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(SphereTest, TangentRay) {
    // Ray just grazing the sphere
    Sphere big_sphere(Point3(0, 0, -5), 1.0, &mat);
    Ray r(Point3(1, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    // Should hit tangentially
    bool hit = big_sphere.hit(r, Interval(0.001, 1000.0), rec);
    if (hit) {
        EXPECT_NEAR(rec.p.x(), 1.0, EPS);
    }
}

TEST_F(SphereTest, NormalIsUnitVector) {
    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    sphere.hit(r, Interval(0.001, 1000.0), rec);
    EXPECT_NEAR(rec.normal.length(), 1.0, EPS);
}

TEST_F(SphereTest, MaterialPointer) {
    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    sphere.hit(r, Interval(0.001, 1000.0), rec);
    EXPECT_EQ(rec.material, &mat);
}

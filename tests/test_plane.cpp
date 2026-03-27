#include <gtest/gtest.h>
#include "scene/primitives.cuh"
#include "scene/material.cuh"
#include "math/interval.cuh"
#include <cmath>

constexpr double EPS = 1e-6;

class PlaneTest : public ::testing::Test {
protected:
    Lambertian mat{Color(0.5, 0.5, 0.5)};
    // Horizontal plane at y=0, normal pointing up
    Plane plane{Point3(0, 0, 0), Vec3(0, 1, 0), &mat};
};

TEST_F(PlaneTest, HitFromAbove) {
    Ray r(Point3(0, 5, 0), Vec3(0, -1, 0));
    HitRecord rec;
    EXPECT_TRUE(plane.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 5.0, EPS);
    EXPECT_NEAR(rec.p.y(), 0.0, EPS);
    EXPECT_TRUE(rec.front_face);
}

TEST_F(PlaneTest, HitFromBelow) {
    Ray r(Point3(0, -3, 0), Vec3(0, 1, 0));
    HitRecord rec;
    EXPECT_TRUE(plane.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 3.0, EPS);
    EXPECT_FALSE(rec.front_face);
}

TEST_F(PlaneTest, ParallelRayMisses) {
    Ray r(Point3(0, 5, 0), Vec3(1, 0, 0));
    HitRecord rec;
    EXPECT_FALSE(plane.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(PlaneTest, HitBehindRay) {
    // Plane is behind the ray
    Ray r(Point3(0, 5, 0), Vec3(0, 1, 0));
    HitRecord rec;
    EXPECT_FALSE(plane.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(PlaneTest, HitAtAngle) {
    Ray r(Point3(0, 2, -2), Vec3(0, -1, 1));
    HitRecord rec;
    EXPECT_TRUE(plane.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 2.0, EPS);
    EXPECT_NEAR(rec.p.y(), 0.0, EPS);
}

TEST_F(PlaneTest, MaterialPointer) {
    Ray r(Point3(0, 5, 0), Vec3(0, -1, 0));
    HitRecord rec;
    plane.hit(r, Interval(0.001, 1000.0), rec);
    EXPECT_EQ(rec.material, &mat);
}

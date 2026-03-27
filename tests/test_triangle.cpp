#include <gtest/gtest.h>
#include "scene/primitives.cuh"
#include "scene/material.cuh"
#include "math/interval.cuh"
#include <cmath>

constexpr double EPS = 1e-6;

class TriangleTest : public ::testing::Test {
protected:
    Lambertian mat{Color(0.5, 0.5, 0.5)};
    // Triangle dans le plan XY, face vers +Z
    Triangle tri{Point3(0, 0, 0), Point3(1, 0, 0), Point3(0, 1, 0), &mat};
};

TEST_F(TriangleTest, HitCenter) {
    // Rayon vers le centre du triangle (1/3, 1/3, 0) depuis z=1
    Ray r(Point3(0.25, 0.25, 1), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(tri.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 1.0, EPS);
    EXPECT_NEAR(rec.p.z(), 0.0, EPS);
}

TEST_F(TriangleTest, MissOutside) {
    // Rayon a cote du triangle
    Ray r(Point3(2, 2, 1), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_FALSE(tri.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(TriangleTest, HitEdge) {
    // Rayon sur le bord du triangle
    Ray r(Point3(0.5, 0.0, 1), Vec3(0, 0, -1));
    HitRecord rec;
    // Sur le bord exact, peut etre true ou false selon l'epsilon
    // On verifie juste qu'il ne crash pas
    tri.hit(r, Interval(0.001, 1000.0), rec);
}

TEST_F(TriangleTest, ParallelRay) {
    // Rayon parallele au plan du triangle
    Ray r(Point3(0, 0, 0.5), Vec3(1, 0, 0));
    HitRecord rec;
    EXPECT_FALSE(tri.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(TriangleTest, HitFromBehind) {
    // Rayon depuis l'arriere du triangle
    Ray r(Point3(0.25, 0.25, -1), Vec3(0, 0, 1));
    HitRecord rec;
    EXPECT_TRUE(tri.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_FALSE(rec.front_face);
}

TEST_F(TriangleTest, NormalIsCorrect) {
    Ray r(Point3(0.25, 0.25, 1), Vec3(0, 0, -1));
    HitRecord rec;
    tri.hit(r, Interval(0.001, 1000.0), rec);
    // Normale doit pointer vers +Z (face au rayon)
    EXPECT_NEAR(rec.normal.z(), 1.0, EPS);
    EXPECT_NEAR(rec.normal.length(), 1.0, EPS);
}

TEST_F(TriangleTest, BoundingBox) {
    AABB bbox = tri.bounding_box();
    // Le triangle est dans [0,1] x [0,1] x [0,0] (avec un peu de padding)
    EXPECT_LE(bbox.x.min, 0.0);
    EXPECT_GE(bbox.x.max, 1.0);
    EXPECT_LE(bbox.y.min, 0.0);
    EXPECT_GE(bbox.y.max, 1.0);
}

TEST_F(TriangleTest, MaterialPointer) {
    Ray r(Point3(0.25, 0.25, 1), Vec3(0, 0, -1));
    HitRecord rec;
    tri.hit(r, Interval(0.001, 1000.0), rec);
    EXPECT_EQ(rec.material, &mat);
}

TEST(TriangleScene, InHittableList) {
    Lambertian mat(Color(1, 0, 0));
    HittableList list;
    list.add(std::make_shared<Triangle>(
        Point3(-1, 0, -2), Point3(1, 0, -2), Point3(0, 2, -2), &mat));

    Ray r(Point3(0, 0.5, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(list.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 2.0, EPS);
}

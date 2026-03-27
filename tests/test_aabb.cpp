#include <gtest/gtest.h>
#include "math/aabb.cuh"
#include "math/ray.cuh"

constexpr double EPS = 1e-6;

TEST(AABB, ConstructFromPoints) {
    AABB box(Point3(-1, -2, -3), Point3(1, 2, 3));
    EXPECT_DOUBLE_EQ(box.x.min, -1.0);
    EXPECT_DOUBLE_EQ(box.x.max, 1.0);
    EXPECT_DOUBLE_EQ(box.y.min, -2.0);
    EXPECT_DOUBLE_EQ(box.y.max, 2.0);
    EXPECT_DOUBLE_EQ(box.z.min, -3.0);
    EXPECT_DOUBLE_EQ(box.z.max, 3.0);
}

TEST(AABB, ConstructFromPointsReversed) {
    // Points dans le mauvais ordre -> doit quand meme fonctionner
    AABB box(Point3(3, 2, 1), Point3(-3, -2, -1));
    EXPECT_DOUBLE_EQ(box.x.min, -3.0);
    EXPECT_DOUBLE_EQ(box.x.max, 3.0);
}

TEST(AABB, HitFrontOn) {
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));
    Ray r(Point3(0, 0, 5), Vec3(0, 0, -1));
    EXPECT_TRUE(box.hit(r, Interval(0.001, 1000.0)));
}

TEST(AABB, Miss) {
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));
    Ray r(Point3(5, 5, 5), Vec3(0, 0, -1));
    EXPECT_FALSE(box.hit(r, Interval(0.001, 1000.0)));
}

TEST(AABB, HitFromInside) {
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));
    Ray r(Point3(0, 0, 0), Vec3(1, 0, 0));
    EXPECT_TRUE(box.hit(r, Interval(0.001, 1000.0)));
}

TEST(AABB, HitBehind) {
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));
    Ray r(Point3(0, 0, 5), Vec3(0, 0, 1)); // going away
    EXPECT_FALSE(box.hit(r, Interval(0.001, 1000.0)));
}

TEST(AABB, HitDiagonal) {
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));
    Ray r(Point3(-5, -5, -5), Vec3(1, 1, 1));
    EXPECT_TRUE(box.hit(r, Interval(0.001, 1000.0)));
}

TEST(AABB, LongestAxis) {
    AABB box1(Point3(0, 0, 0), Point3(3, 1, 2));
    EXPECT_EQ(box1.longest_axis(), 0);

    AABB box2(Point3(0, 0, 0), Point3(1, 5, 2));
    EXPECT_EQ(box2.longest_axis(), 1);

    AABB box3(Point3(0, 0, 0), Point3(1, 2, 7));
    EXPECT_EQ(box3.longest_axis(), 2);
}

TEST(AABB, SurfaceArea) {
    AABB box(Point3(0, 0, 0), Point3(2, 3, 4));
    // SA = 2*(2*3 + 3*4 + 4*2) = 2*(6+12+8) = 52
    EXPECT_NEAR(box.surface_area(), 52.0, EPS);
}

TEST(AABB, SurroundingBox) {
    AABB a(Point3(-1, -1, -1), Point3(0, 0, 0));
    AABB b(Point3(0, 0, 0), Point3(1, 1, 1));
    AABB c = surrounding_box(a, b);
    EXPECT_DOUBLE_EQ(c.x.min, -1.0);
    EXPECT_DOUBLE_EQ(c.x.max, 1.0);
    EXPECT_DOUBLE_EQ(c.y.min, -1.0);
    EXPECT_DOUBLE_EQ(c.y.max, 1.0);
}

TEST(AABB, ThinBoxHit) {
    // Boite tres fine (plan-like)
    AABB box(Point3(-10, -0.001, -10), Point3(10, 0.001, 10));
    Ray r(Point3(0, 5, 0), Vec3(0, -1, 0));
    EXPECT_TRUE(box.hit(r, Interval(0.001, 1000.0)));
}

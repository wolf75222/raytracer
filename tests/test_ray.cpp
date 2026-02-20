#include <gtest/gtest.h>
#include "core/ray.h"

constexpr double EPS = 1e-10;

TEST(Ray, DefaultConstructor) {
    Ray r;
    // Should compile and not crash
    (void)r;
}

TEST(Ray, ConstructorAndAccessors) {
    Point3 origin(1, 2, 3);
    Vec3 direction(4, 5, 6);
    Ray r(origin, direction);

    EXPECT_DOUBLE_EQ(r.origin().x(), 1.0);
    EXPECT_DOUBLE_EQ(r.origin().y(), 2.0);
    EXPECT_DOUBLE_EQ(r.origin().z(), 3.0);
    EXPECT_DOUBLE_EQ(r.direction().x(), 4.0);
    EXPECT_DOUBLE_EQ(r.direction().y(), 5.0);
    EXPECT_DOUBLE_EQ(r.direction().z(), 6.0);
}

TEST(Ray, AtZero) {
    Ray r(Point3(1, 2, 3), Vec3(4, 5, 6));
    Point3 p = r.at(0.0);
    EXPECT_NEAR(p.x(), 1.0, EPS);
    EXPECT_NEAR(p.y(), 2.0, EPS);
    EXPECT_NEAR(p.z(), 3.0, EPS);
}

TEST(Ray, AtOne) {
    Ray r(Point3(1, 2, 3), Vec3(4, 5, 6));
    Point3 p = r.at(1.0);
    EXPECT_NEAR(p.x(), 5.0, EPS);
    EXPECT_NEAR(p.y(), 7.0, EPS);
    EXPECT_NEAR(p.z(), 9.0, EPS);
}

TEST(Ray, AtArbitrary) {
    Ray r(Point3(0, 0, 0), Vec3(1, 0, 0));
    Point3 p = r.at(2.5);
    EXPECT_NEAR(p.x(), 2.5, EPS);
    EXPECT_NEAR(p.y(), 0.0, EPS);
    EXPECT_NEAR(p.z(), 0.0, EPS);
}

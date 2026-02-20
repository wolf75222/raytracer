#include <gtest/gtest.h>
#include "core/vec3.h"
#include "core/random_utils.h"
#include <cmath>

constexpr double EPS = 1e-10;

TEST(Vec3, DefaultConstructor) {
    Vec3 v;
    EXPECT_DOUBLE_EQ(v.x(), 0.0);
    EXPECT_DOUBLE_EQ(v.y(), 0.0);
    EXPECT_DOUBLE_EQ(v.z(), 0.0);
}

TEST(Vec3, ParameterizedConstructor) {
    Vec3 v(1.0, 2.0, 3.0);
    EXPECT_DOUBLE_EQ(v.x(), 1.0);
    EXPECT_DOUBLE_EQ(v.y(), 2.0);
    EXPECT_DOUBLE_EQ(v.z(), 3.0);
}

TEST(Vec3, IndexAccess) {
    Vec3 v(4.0, 5.0, 6.0);
    EXPECT_DOUBLE_EQ(v[0], 4.0);
    EXPECT_DOUBLE_EQ(v[1], 5.0);
    EXPECT_DOUBLE_EQ(v[2], 6.0);
}

TEST(Vec3, Negation) {
    Vec3 v(1.0, -2.0, 3.0);
    Vec3 neg = -v;
    EXPECT_DOUBLE_EQ(neg.x(), -1.0);
    EXPECT_DOUBLE_EQ(neg.y(), 2.0);
    EXPECT_DOUBLE_EQ(neg.z(), -3.0);
}

TEST(Vec3, Addition) {
    Vec3 a(1, 2, 3), b(4, 5, 6);
    Vec3 c = a + b;
    EXPECT_DOUBLE_EQ(c.x(), 5.0);
    EXPECT_DOUBLE_EQ(c.y(), 7.0);
    EXPECT_DOUBLE_EQ(c.z(), 9.0);
}

TEST(Vec3, Subtraction) {
    Vec3 a(5, 7, 9), b(1, 2, 3);
    Vec3 c = a - b;
    EXPECT_DOUBLE_EQ(c.x(), 4.0);
    EXPECT_DOUBLE_EQ(c.y(), 5.0);
    EXPECT_DOUBLE_EQ(c.z(), 6.0);
}

TEST(Vec3, ComponentWiseMultiplication) {
    Vec3 a(2, 3, 4), b(5, 6, 7);
    Vec3 c = a * b;
    EXPECT_DOUBLE_EQ(c.x(), 10.0);
    EXPECT_DOUBLE_EQ(c.y(), 18.0);
    EXPECT_DOUBLE_EQ(c.z(), 28.0);
}

TEST(Vec3, ScalarMultiplication) {
    Vec3 v(1, 2, 3);
    Vec3 c = 2.0 * v;
    EXPECT_DOUBLE_EQ(c.x(), 2.0);
    EXPECT_DOUBLE_EQ(c.y(), 4.0);
    EXPECT_DOUBLE_EQ(c.z(), 6.0);

    Vec3 d = v * 3.0;
    EXPECT_DOUBLE_EQ(d.x(), 3.0);
    EXPECT_DOUBLE_EQ(d.y(), 6.0);
    EXPECT_DOUBLE_EQ(d.z(), 9.0);
}

TEST(Vec3, Division) {
    Vec3 v(4, 6, 8);
    Vec3 c = v / 2.0;
    EXPECT_DOUBLE_EQ(c.x(), 2.0);
    EXPECT_DOUBLE_EQ(c.y(), 3.0);
    EXPECT_DOUBLE_EQ(c.z(), 4.0);
}

TEST(Vec3, PlusEquals) {
    Vec3 v(1, 2, 3);
    v += Vec3(4, 5, 6);
    EXPECT_DOUBLE_EQ(v.x(), 5.0);
    EXPECT_DOUBLE_EQ(v.y(), 7.0);
    EXPECT_DOUBLE_EQ(v.z(), 9.0);
}

TEST(Vec3, TimesEquals) {
    Vec3 v(1, 2, 3);
    v *= 2.0;
    EXPECT_DOUBLE_EQ(v.x(), 2.0);
    EXPECT_DOUBLE_EQ(v.y(), 4.0);
    EXPECT_DOUBLE_EQ(v.z(), 6.0);
}

TEST(Vec3, DotProduct) {
    Vec3 a(1, 2, 3), b(4, 5, 6);
    EXPECT_NEAR(dot(a, b), 32.0, EPS);
}

TEST(Vec3, CrossProduct) {
    Vec3 a(1, 0, 0), b(0, 1, 0);
    Vec3 c = cross(a, b);
    EXPECT_NEAR(c.x(), 0.0, EPS);
    EXPECT_NEAR(c.y(), 0.0, EPS);
    EXPECT_NEAR(c.z(), 1.0, EPS);
}

TEST(Vec3, CrossProductAnticommutative) {
    Vec3 a(1, 2, 3), b(4, 5, 6);
    Vec3 ab = cross(a, b);
    Vec3 ba = cross(b, a);
    EXPECT_NEAR(ab.x(), -ba.x(), EPS);
    EXPECT_NEAR(ab.y(), -ba.y(), EPS);
    EXPECT_NEAR(ab.z(), -ba.z(), EPS);
}

TEST(Vec3, Length) {
    Vec3 v(3, 4, 0);
    EXPECT_NEAR(v.length(), 5.0, EPS);
    EXPECT_NEAR(v.length_squared(), 25.0, EPS);
}

TEST(Vec3, UnitVector) {
    Vec3 v(3, 4, 0);
    Vec3 u = unit_vector(v);
    EXPECT_NEAR(u.length(), 1.0, EPS);
    EXPECT_NEAR(u.x(), 0.6, EPS);
    EXPECT_NEAR(u.y(), 0.8, EPS);
}

TEST(Vec3, Reflect) {
    Vec3 v(1, -1, 0);
    Vec3 n(0, 1, 0);
    Vec3 r = reflect(v, n);
    EXPECT_NEAR(r.x(), 1.0, EPS);
    EXPECT_NEAR(r.y(), 1.0, EPS);
    EXPECT_NEAR(r.z(), 0.0, EPS);
}

TEST(Vec3, RefractStraightOn) {
    // Normal incidence: refracted ray should continue straight
    Vec3 v = unit_vector(Vec3(0, -1, 0));
    Vec3 n(0, 1, 0);
    Vec3 r = refract(v, n, 1.0);
    EXPECT_NEAR(r.x(), 0.0, EPS);
    EXPECT_NEAR(r.y(), -1.0, EPS);
    EXPECT_NEAR(r.z(), 0.0, EPS);
}

TEST(Vec3, NearZero) {
    Vec3 small(1e-9, 1e-9, 1e-9);
    EXPECT_TRUE(small.near_zero());

    Vec3 big(0.1, 0, 0);
    EXPECT_FALSE(big.near_zero());
}

TEST(RandomUtils, RandomDoubleRange) {
    for (int i = 0; i < 100; ++i) {
        double r = random_double();
        EXPECT_GE(r, 0.0);
        EXPECT_LT(r, 1.0);
    }
}

TEST(RandomUtils, RandomInUnitSphere) {
    for (int i = 0; i < 100; ++i) {
        Vec3 p = random_in_unit_sphere();
        EXPECT_LT(p.length_squared(), 1.0);
    }
}

TEST(RandomUtils, RandomUnitVector) {
    for (int i = 0; i < 100; ++i) {
        Vec3 v = random_unit_vector();
        EXPECT_NEAR(v.length(), 1.0, 1e-6);
    }
}

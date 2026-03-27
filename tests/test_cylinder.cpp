#include <gtest/gtest.h>
#include "scene/primitives.cuh"
#include "scene/material.cuh"
#include "math/interval.cuh"
#include <cmath>

constexpr double EPS = 1e-6;

class CylinderTest : public ::testing::Test {
protected:
    Lambertian mat{Color(0.5, 0.5, 0.5)};
    // Cylindre a l'origine, rayon 1, hauteur 2 (y de 0 a 2)
    Cylinder cyl{Point3(0, 0, 0), 1.0, 2.0, &mat};
};

TEST_F(CylinderTest, HitSide) {
    // Rayon horizontal vers le cylindre
    Ray r(Point3(3, 1, 0), Vec3(-1, 0, 0));
    HitRecord rec;
    EXPECT_TRUE(cyl.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 2.0, EPS); // touche a x=1
    EXPECT_NEAR(rec.p.x(), 1.0, EPS);
}

TEST_F(CylinderTest, MissAbove) {
    // Rayon au-dessus du cylindre
    Ray r(Point3(3, 5, 0), Vec3(-1, 0, 0));
    HitRecord rec;
    EXPECT_FALSE(cyl.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(CylinderTest, MissBelow) {
    // Rayon en-dessous
    Ray r(Point3(3, -1, 0), Vec3(-1, 0, 0));
    HitRecord rec;
    EXPECT_FALSE(cyl.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(CylinderTest, MissCompletely) {
    // Rayon qui passe a cote
    Ray r(Point3(3, 1, 5), Vec3(-1, 0, 0));
    HitRecord rec;
    EXPECT_FALSE(cyl.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(CylinderTest, NormalIsRadial) {
    Ray r(Point3(3, 1, 0), Vec3(-1, 0, 0));
    HitRecord rec;
    cyl.hit(r, Interval(0.001, 1000.0), rec);
    // Normale doit etre radiale (pas de composante Y)
    EXPECT_NEAR(rec.normal.y(), 0.0, EPS);
    EXPECT_NEAR(rec.normal.length(), 1.0, EPS);
}

TEST_F(CylinderTest, HitFromInside) {
    Ray r(Point3(0, 1, 0), Vec3(1, 0, 0));
    HitRecord rec;
    EXPECT_TRUE(cyl.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_FALSE(rec.front_face);
}

TEST_F(CylinderTest, BoundingBox) {
    AABB bbox = cyl.bounding_box();
    EXPECT_LE(bbox.x.min, -1.0);
    EXPECT_GE(bbox.x.max, 1.0);
    EXPECT_LE(bbox.y.min, 0.0);
    EXPECT_GE(bbox.y.max, 2.0);
}

TEST_F(CylinderTest, MaterialPointer) {
    Ray r(Point3(3, 1, 0), Vec3(-1, 0, 0));
    HitRecord rec;
    cyl.hit(r, Interval(0.001, 1000.0), rec);
    EXPECT_EQ(rec.material, &mat);
}

TEST_F(CylinderTest, HitAtAngle) {
    // Rayon diagonal
    Ray r(Point3(3, 0.5, 3), Vec3(-1, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(cyl.hit(r, Interval(0.001, 1000.0), rec));
    // Le point doit etre sur le cylindre
    double dx = rec.p.x();
    double dz = rec.p.z();
    EXPECT_NEAR(dx*dx + dz*dz, 1.0, 0.01);
}

#include <gtest/gtest.h>
#include "core/hittable_list.h"
#include "core/sphere.h"
#include "core/lambertian.h"
#include "core/interval.h"

constexpr double EPS = 1e-6;

TEST(HittableList, EmptyListMisses) {
    HittableList list;
    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_FALSE(list.hit(r, Interval(0.001, 1000.0), rec));
}

TEST(HittableList, SingleObject) {
    Lambertian mat(Color(0.5, 0.5, 0.5));
    HittableList list;
    list.add(std::make_shared<Sphere>(Point3(0, 0, -2), 0.5, &mat));

    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(list.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 1.5, EPS);
}

TEST(HittableList, ReturnsClosest) {
    Lambertian mat(Color(0.5, 0.5, 0.5));
    HittableList list;
    // Near sphere at z=-2, far sphere at z=-5
    list.add(std::make_shared<Sphere>(Point3(0, 0, -5), 0.5, &mat));
    list.add(std::make_shared<Sphere>(Point3(0, 0, -2), 0.5, &mat));

    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(list.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 1.5, EPS);  // hit the nearer sphere
}

TEST(HittableList, Clear) {
    Lambertian mat(Color(0.5, 0.5, 0.5));
    HittableList list;
    list.add(std::make_shared<Sphere>(Point3(0, 0, -2), 0.5, &mat));
    list.clear();

    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_FALSE(list.hit(r, Interval(0.001, 1000.0), rec));
}

TEST(HittableList, AllMiss) {
    Lambertian mat(Color(0.5, 0.5, 0.5));
    HittableList list;
    list.add(std::make_shared<Sphere>(Point3(10, 10, 10), 0.5, &mat));
    list.add(std::make_shared<Sphere>(Point3(-10, -10, -10), 0.5, &mat));

    Ray r(Point3(0, 0, 0), Vec3(0, 1, 0));
    HitRecord rec;
    EXPECT_FALSE(list.hit(r, Interval(0.001, 1000.0), rec));
}

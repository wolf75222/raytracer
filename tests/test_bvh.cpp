#include <gtest/gtest.h>
#include "core/bvh.h"
#include "core/hittable_list.h"
#include "core/sphere.h"
#include "core/lambertian.h"
#include "core/interval.h"
#include <cmath>

constexpr double EPS = 1e-6;

class BVHTest : public ::testing::Test {
protected:
    Lambertian mat{Color(0.5, 0.5, 0.5)};
};

TEST_F(BVHTest, SingleSphere) {
    HittableList list;
    list.add(std::make_shared<Sphere>(Point3(0, 0, -2), 0.5, &mat));

    BVHNode bvh(list);
    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(bvh.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 1.5, EPS);
}

TEST_F(BVHTest, MissesAll) {
    HittableList list;
    list.add(std::make_shared<Sphere>(Point3(10, 10, 10), 0.5, &mat));
    list.add(std::make_shared<Sphere>(Point3(-10, -10, -10), 0.5, &mat));

    BVHNode bvh(list);
    Ray r(Point3(0, 0, 0), Vec3(0, 1, 0));
    HitRecord rec;
    EXPECT_FALSE(bvh.hit(r, Interval(0.001, 1000.0), rec));
}

TEST_F(BVHTest, FindsClosest) {
    HittableList list;
    list.add(std::make_shared<Sphere>(Point3(0, 0, -5), 0.5, &mat));
    list.add(std::make_shared<Sphere>(Point3(0, 0, -2), 0.5, &mat));
    list.add(std::make_shared<Sphere>(Point3(0, 0, -8), 0.5, &mat));

    BVHNode bvh(list);
    Ray r(Point3(0, 0, 0), Vec3(0, 0, -1));
    HitRecord rec;
    EXPECT_TRUE(bvh.hit(r, Interval(0.001, 1000.0), rec));
    EXPECT_NEAR(rec.t, 1.5, EPS);  // closest sphere at z=-2
}

TEST_F(BVHTest, MatchesBruteForce) {
    // Build a scene with many spheres, compare BVH vs brute force
    HittableList list;
    for (int i = 0; i < 50; ++i) {
        Point3 center(i * 2.0 - 50, 0, -10 - i);
        list.add(std::make_shared<Sphere>(center, 0.5, &mat));
    }

    BVHNode bvh(list);

    // Test several rays
    for (int k = 0; k < 20; ++k) {
        Vec3 dir(k * 0.2 - 2.0, 0.1, -1.0);
        Ray r(Point3(0, 0, 0), dir);
        HitRecord bvh_rec, list_rec;
        bool bvh_hit = bvh.hit(r, Interval(0.001, 1000.0), bvh_rec);
        bool list_hit = list.hit(r, Interval(0.001, 1000.0), list_rec);

        EXPECT_EQ(bvh_hit, list_hit);
        if (bvh_hit && list_hit) {
            EXPECT_NEAR(bvh_rec.t, list_rec.t, EPS);
        }
    }
}

TEST_F(BVHTest, BoundingBoxContainsAll) {
    HittableList list;
    list.add(std::make_shared<Sphere>(Point3(-3, 0, 0), 1.0, &mat));
    list.add(std::make_shared<Sphere>(Point3(3, 0, 0), 1.0, &mat));

    BVHNode bvh(list);
    AABB bbox = bvh.bounding_box();

    EXPECT_LE(bbox.x.min, -4.0);
    EXPECT_GE(bbox.x.max, 4.0);
}

TEST(AABB, HitBasic) {
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));
    Ray r(Point3(0, 0, 5), Vec3(0, 0, -1));
    EXPECT_TRUE(box.hit(r, Interval(0.001, 1000.0)));
}

TEST(AABB, Miss) {
    AABB box(Point3(-1, -1, -1), Point3(1, 1, 1));
    Ray r(Point3(5, 5, 5), Vec3(0, 0, -1));
    EXPECT_FALSE(box.hit(r, Interval(0.001, 1000.0)));
}

TEST(AABB, LongestAxis) {
    AABB box(Point3(0, 0, 0), Point3(3, 1, 2));
    EXPECT_EQ(box.longest_axis(), 0);  // X is longest

    AABB box2(Point3(0, 0, 0), Point3(1, 5, 2));
    EXPECT_EQ(box2.longest_axis(), 1);  // Y is longest
}

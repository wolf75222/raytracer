#include <gtest/gtest.h>
#include "math/random.cuh"
#include "math/vec3.cuh"
#include <cmath>
#include <set>

TEST(Random, DoubleInRange) {
    for (int i = 0; i < 1000; i++) {
        double r = random_double();
        EXPECT_GE(r, 0.0);
        EXPECT_LT(r, 1.0);
    }
}

TEST(Random, DoubleInCustomRange) {
    for (int i = 0; i < 1000; i++) {
        double r = random_double(-5.0, 5.0);
        EXPECT_GE(r, -5.0);
        EXPECT_LT(r, 5.0);
    }
}

TEST(Random, InUnitSphere) {
    for (int i = 0; i < 500; i++) {
        Vec3 p = random_in_unit_sphere();
        EXPECT_LT(p.length_squared(), 1.0);
    }
}

TEST(Random, UnitVectorIsUnit) {
    for (int i = 0; i < 500; i++) {
        Vec3 v = random_unit_vector();
        EXPECT_NEAR(v.length(), 1.0, 1e-6);
    }
}

TEST(Random, InUnitDisk) {
    for (int i = 0; i < 500; i++) {
        Vec3 p = random_in_unit_disk();
        EXPECT_LT(p.length_squared(), 1.0);
        EXPECT_DOUBLE_EQ(p.z(), 0.0); // z doit etre 0
    }
}

TEST(Random, OnHemisphere) {
    Vec3 normal(0, 1, 0);
    for (int i = 0; i < 500; i++) {
        Vec3 v = random_on_hemisphere(normal);
        EXPECT_GT(dot(v, normal), 0.0); // doit etre dans le meme hemisphere
        EXPECT_NEAR(v.length(), 1.0, 1e-6);
    }
}

TEST(Random, Distribution) {
    // Verifier que le RNG n'est pas degenere (produit des valeurs variees)
    int buckets[10] = {};
    for (int i = 0; i < 10000; i++) {
        int b = (int)(random_double() * 10);
        if (b >= 10) b = 9;
        buckets[b]++;
    }
    for (int i = 0; i < 10; i++) {
        // Chaque bucket devrait avoir ~1000 +/- 200
        EXPECT_GT(buckets[i], 700);
        EXPECT_LT(buckets[i], 1300);
    }
}

TEST(StdRng, DifferentValues) {
    // std::mt19937 doit produire des valeurs variees
    double first = random_double();
    bool all_same = true;
    for (int i = 0; i < 100; i++) {
        if (random_double() != first) {
            all_same = false;
            break;
        }
    }
    EXPECT_FALSE(all_same);
}

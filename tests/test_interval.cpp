#include <gtest/gtest.h>
#include "core/vec3.h"
#include "core/interval.h"

TEST(Interval, DefaultIsEmpty) {
    Interval i;
    EXPECT_GT(i.min, i.max);
}

TEST(Interval, Contains) {
    Interval i(0.0, 1.0);
    EXPECT_TRUE(i.contains(0.0));
    EXPECT_TRUE(i.contains(0.5));
    EXPECT_TRUE(i.contains(1.0));
    EXPECT_FALSE(i.contains(-0.1));
    EXPECT_FALSE(i.contains(1.1));
}

TEST(Interval, Surrounds) {
    Interval i(0.0, 1.0);
    EXPECT_FALSE(i.surrounds(0.0));  // boundary excluded
    EXPECT_TRUE(i.surrounds(0.5));
    EXPECT_FALSE(i.surrounds(1.0));  // boundary excluded
}

TEST(Interval, Clamp) {
    Interval i(0.0, 1.0);
    EXPECT_DOUBLE_EQ(i.clamp(-1.0), 0.0);
    EXPECT_DOUBLE_EQ(i.clamp(0.5), 0.5);
    EXPECT_DOUBLE_EQ(i.clamp(2.0), 1.0);
}

TEST(Interval, Size) {
    Interval i(2.0, 5.0);
    EXPECT_DOUBLE_EQ(i.size(), 3.0);
}

TEST(Interval, EmptyAndUniverse) {
    EXPECT_GT(Interval::empty.min, Interval::empty.max);
    EXPECT_LT(Interval::universe.min, Interval::universe.max);
}

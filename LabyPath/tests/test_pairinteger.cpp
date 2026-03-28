/**
 * @file test_pairinteger.cpp
 * @brief Unit tests for PairInteger ordered pair with hashing
 */

#include <functional>
#include <gtest/gtest.h>
#include "basic/PairInteger.h"
#include <unordered_set>


namespace laby::basic {
namespace {

TEST(PairIntegerTest, DefaultConstruction) {
    PairInteger const pi;
    EXPECT_EQ(pi.first(), 0U);
    EXPECT_EQ(pi.second(), 0U);
}

TEST(PairIntegerTest, OrderedConstruction) {
    // When first < second, they should be stored as-is (min, max)
    PairInteger const pi(3, 7);
    EXPECT_EQ(pi.first(), 3U);
    EXPECT_EQ(pi.second(), 7U);
}

TEST(PairIntegerTest, NormalizesOrder) {
    // When first > second, the pair should be normalized to (min, max)
    PairInteger const pi(7, 3);
    EXPECT_EQ(pi.first(), 3U);
    EXPECT_EQ(pi.second(), 7U);
}

TEST(PairIntegerTest, EqualValues) {
    PairInteger const pi(5, 5);
    EXPECT_EQ(pi.first(), 5U);
    EXPECT_EQ(pi.second(), 5U);
}

TEST(PairIntegerTest, Equality) {
    PairInteger const a(3, 7);
    PairInteger const b(7, 3);
    // Both should be normalized to (3, 7)
    EXPECT_EQ(a, b);
}

TEST(PairIntegerTest, Inequality) {
    PairInteger const a(3, 7);
    PairInteger const b(3, 8);
    EXPECT_FALSE(a == b);
}

TEST(PairIntegerTest, HashConsistency) {
    // Same logical pair should have same hash
    PairInteger const a(3, 7);
    PairInteger const b(7, 3);
    std::hash<PairInteger> const hasher;
    EXPECT_EQ(hasher(a), hasher(b));
}

TEST(PairIntegerTest, UseInUnorderedSet) {
    std::unordered_set<PairInteger> s;
    s.insert(PairInteger(3, 7));
    s.insert(PairInteger(7, 3)); // Same pair, should not duplicate
    s.insert(PairInteger(1, 2));
    EXPECT_EQ(s.size(), 2U);
}

TEST(PairIntegerTest, SetFirst) {
    PairInteger pi(3, 7);
    pi.setFirst(10);
    EXPECT_EQ(pi.first(), 10U);
    EXPECT_EQ(pi.second(), 7U);
}

TEST(PairIntegerTest, SetSecond) {
    PairInteger pi(3, 7);
    pi.setSecond(10);
    EXPECT_EQ(pi.first(), 3U);
    EXPECT_EQ(pi.second(), 10U);
}

} // namespace
} // namespace laby::basic


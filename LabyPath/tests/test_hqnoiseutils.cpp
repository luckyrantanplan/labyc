/**
 * @file test_hqnoiseutils.cpp
 * @brief Unit tests for HqNoiseUtils static math functions.
 */

#include <gtest/gtest.h>
#include <cmath>
#include <complex>
#include "generator/HqNoise.h"

using laby::generator::HqNoiseUtils;

// ── fastFloor ──────────────────────────────────────────────────────────────

TEST(HqNoiseUtilsTest, FastFloorPositiveInteger) {
    EXPECT_EQ(HqNoiseUtils::fastFloor(3.0), 3);
}

TEST(HqNoiseUtilsTest, FastFloorPositiveFraction) {
    EXPECT_EQ(HqNoiseUtils::fastFloor(3.7), 3);
}

TEST(HqNoiseUtilsTest, FastFloorZero) {
    EXPECT_EQ(HqNoiseUtils::fastFloor(0.0), 0);
}

TEST(HqNoiseUtilsTest, FastFloorNegativeFraction) {
    // fastFloor(-0.5) should be -1 (floor behavior for negatives)
    EXPECT_EQ(HqNoiseUtils::fastFloor(-0.5), -1);
}

TEST(HqNoiseUtilsTest, FastFloorNegativeInteger) {
    // fastFloor(-3.0) should return -3 (same as std::floor for exact integers)
    EXPECT_EQ(HqNoiseUtils::fastFloor(-3.0), -3);
}

// ── lerp ───────────────────────────────────────────────────────────────────

TEST(HqNoiseUtilsTest, LerpAtZero) {
    EXPECT_DOUBLE_EQ(HqNoiseUtils::lerp(10.0, 20.0, 0.0), 10.0);
}

TEST(HqNoiseUtilsTest, LerpAtOne) {
    EXPECT_DOUBLE_EQ(HqNoiseUtils::lerp(10.0, 20.0, 1.0), 20.0);
}

TEST(HqNoiseUtilsTest, LerpAtHalf) {
    EXPECT_DOUBLE_EQ(HqNoiseUtils::lerp(0.0, 100.0, 0.5), 50.0);
}

TEST(HqNoiseUtilsTest, LerpNegativeValues) {
    EXPECT_DOUBLE_EQ(HqNoiseUtils::lerp(-10.0, 10.0, 0.5), 0.0);
}

TEST(HqNoiseUtilsTest, LerpBeyondOne) {
    // Extrapolation: lerp(0, 10, 2.0) = 20.0
    EXPECT_DOUBLE_EQ(HqNoiseUtils::lerp(0.0, 10.0, 2.0), 20.0);
}

// ── lerpC (complex) ────────────────────────────────────────────────────────

TEST(HqNoiseUtilsTest, LerpComplexAtZero) {
    std::complex<double> const a(1.0, 2.0);
    std::complex<double> const b(3.0, 4.0);
    auto result = HqNoiseUtils::lerpC(a, b, 0.0);
    EXPECT_DOUBLE_EQ(result.real(), 1.0);
    EXPECT_DOUBLE_EQ(result.imag(), 2.0);
}

TEST(HqNoiseUtilsTest, LerpComplexAtOne) {
    std::complex<double> const a(1.0, 2.0);
    std::complex<double> const b(3.0, 4.0);
    auto result = HqNoiseUtils::lerpC(a, b, 1.0);
    EXPECT_DOUBLE_EQ(result.real(), 3.0);
    EXPECT_DOUBLE_EQ(result.imag(), 4.0);
}

TEST(HqNoiseUtilsTest, LerpComplexAtHalf) {
    std::complex<double> const a(0.0, 0.0);
    std::complex<double> const b(10.0, 20.0);
    auto result = HqNoiseUtils::lerpC(a, b, 0.5);
    EXPECT_DOUBLE_EQ(result.real(), 5.0);
    EXPECT_DOUBLE_EQ(result.imag(), 10.0);
}

// ── HqNoise2D::sgn ────────────────────────────────────────────────────────

TEST(HqNoise2DSgnTest, EvenSumNegative) {
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise2D::sgn(0, 0), -1.0);
}

TEST(HqNoise2DSgnTest, OddSumPositive) {
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise2D::sgn(1, 0), 1.0);
}

TEST(HqNoise2DSgnTest, AlternatesPattern) {
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise2D::sgn(0, 1), 1.0);
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise2D::sgn(1, 1), -1.0);
}

// ── HqNoise1D::sgn ────────────────────────────────────────────────────────

TEST(HqNoise1DSgnTest, EvenNegative) {
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise1D::sgn(0), -1.0);
}

TEST(HqNoise1DSgnTest, OddPositive) {
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise1D::sgn(1), 1.0);
}

TEST(HqNoise1DSgnTest, Alternates) {
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise1D::sgn(2), -1.0);
    EXPECT_DOUBLE_EQ(laby::generator::HqNoise1D::sgn(3), 1.0);
}

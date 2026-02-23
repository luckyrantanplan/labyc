/**
 * @file test_color.cpp
 * @brief Unit tests for Color utility class
 */

#include <gtest/gtest.h>
#include "basic/Color.h"

namespace laby {
namespace basic {
namespace {

TEST(ColorTest, CreateAndExtractRed) {
    uint32_t color = Color::create(200, 0, 0);
    EXPECT_EQ(Color::get_red(color), 200U);
    EXPECT_EQ(Color::get_green(color), 0U);
    EXPECT_EQ(Color::get_blue(color), 0U);
}

TEST(ColorTest, CreateAndExtractGreen) {
    uint32_t color = Color::create(0, 150, 0);
    EXPECT_EQ(Color::get_red(color), 0U);
    EXPECT_EQ(Color::get_green(color), 150U);
    EXPECT_EQ(Color::get_blue(color), 0U);
}

TEST(ColorTest, CreateAndExtractBlue) {
    uint32_t color = Color::create(0, 0, 255);
    EXPECT_EQ(Color::get_red(color), 0U);
    EXPECT_EQ(Color::get_green(color), 0U);
    EXPECT_EQ(Color::get_blue(color), 255U);
}

TEST(ColorTest, CreateAndExtractMixed) {
    uint32_t color = Color::create(10, 20, 30);
    EXPECT_EQ(Color::get_red(color), 10U);
    EXPECT_EQ(Color::get_green(color), 20U);
    EXPECT_EQ(Color::get_blue(color), 30U);
}

TEST(ColorTest, NormalizedRedValues) {
    uint32_t color = Color::create(255, 0, 0);
    EXPECT_NEAR(Color::get_red_normalized(color), 1.0, 1e-9);
    EXPECT_NEAR(Color::get_green_normalized(color), 0.0, 1e-9);
    EXPECT_NEAR(Color::get_blue_normalized(color), 0.0, 1e-9);
}

TEST(ColorTest, NormalizedMidValues) {
    uint32_t color = Color::create(128, 128, 128);
    double expected = 128.0 / 255.0;
    EXPECT_NEAR(Color::get_red_normalized(color), expected, 1e-9);
    EXPECT_NEAR(Color::get_green_normalized(color), expected, 1e-9);
    EXPECT_NEAR(Color::get_blue_normalized(color), expected, 1e-9);
}

TEST(ColorTest, SetRedPreservesOthers) {
    uint32_t color = Color::create(10, 20, 30);
    uint32_t modified = Color::set_red(color, 100);
    EXPECT_EQ(Color::get_red(modified), 100U);
    EXPECT_EQ(Color::get_green(modified), 20U);
    EXPECT_EQ(Color::get_blue(modified), 30U);
}

TEST(ColorTest, SetGreenPreservesOthers) {
    uint32_t color = Color::create(10, 20, 30);
    uint32_t modified = Color::set_green(color, 100);
    EXPECT_EQ(Color::get_red(modified), 10U);
    EXPECT_EQ(Color::get_green(modified), 100U);
    EXPECT_EQ(Color::get_blue(modified), 30U);
}

TEST(ColorTest, SetBluePreservesOthers) {
    uint32_t color = Color::create(10, 20, 30);
    uint32_t modified = Color::set_blue(color, 100);
    EXPECT_EQ(Color::get_red(modified), 10U);
    EXPECT_EQ(Color::get_green(modified), 20U);
    EXPECT_EQ(Color::get_blue(modified), 100U);
}

TEST(ColorTest, BlackColor) {
    uint32_t color = Color::create(0, 0, 0);
    EXPECT_EQ(Color::get_red(color), 0U);
    EXPECT_EQ(Color::get_green(color), 0U);
    EXPECT_EQ(Color::get_blue(color), 0U);
}

TEST(ColorTest, WhiteColor) {
    uint32_t color = Color::create(255, 255, 255);
    EXPECT_EQ(Color::get_red(color), 255U);
    EXPECT_EQ(Color::get_green(color), 255U);
    EXPECT_EQ(Color::get_blue(color), 255U);
}

TEST(ColorTest, RoundTripAllChannels) {
    // Create, extract, set each channel, verify round-trip
    for (uint32_t r = 0; r < 256; r += 51) {
        for (uint32_t g = 0; g < 256; g += 51) {
            for (uint32_t b = 0; b < 256; b += 51) {
                uint32_t color = Color::create(r, g, b);
                EXPECT_EQ(Color::get_red(color), r);
                EXPECT_EQ(Color::get_green(color), g);
                EXPECT_EQ(Color::get_blue(color), b);
            }
        }
    }
}

} // namespace
} // namespace basic
} // namespace laby

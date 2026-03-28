/*
 * Color.h
 *
 *  Created on: Aug 9, 2018
 *      Author: florian
 */

#ifndef BASIC_COLOR_H_
#define BASIC_COLOR_H_

#include <cstdint>
#include <svgpp/factory/integer_color.hpp>


namespace laby::basic {

class Color {
public:
    static constexpr double colorComponentMax = 0x000000FF;
    static constexpr uint32_t mask = 0x000000FF;

    static auto getChannel(const uint32_t color, const int offset) -> uint32_t {
        return (color >> offset) & 0x000000FF;
    }

    static auto getRedNormalized(const uint32_t color) -> double {
        return getRed(color) / colorComponentMax;
    }

    static auto getGreenNormalized(const uint32_t color) -> double {
        return getGreen(color) / colorComponentMax;
    }

    static auto getBlueNormalized(const uint32_t color) -> double {
        return getBlue(color) / colorComponentMax;
    }

    static auto getRed(const uint32_t color) -> uint32_t {
        return getChannel(color, svgpp::factory::color::rgb8_policy::r_offset);
    }

    static auto getGreen(const uint32_t color) -> uint32_t {
        return getChannel(color, svgpp::factory::color::rgb8_policy::g_offset);
    }

    static auto getBlue(const uint32_t color) -> uint32_t {
        return getChannel(color, svgpp::factory::color::rgb8_policy::b_offset);
    }

    static auto create(const uint32_t r, const uint32_t g, const uint32_t b) -> uint32_t {
        using svgpp::factory::color::rgb8_policy;
        return rgb8_policy::preset_bits | (r << rgb8_policy::r_offset) | (g << rgb8_policy::g_offset) | (b << rgb8_policy::b_offset);
    }

    static auto setRed(const uint32_t color, const uint32_t red) -> uint32_t {
        return create(red, getGreen(color), getBlue(color));
    }

    static auto setGreen(const uint32_t color, const uint32_t green) -> uint32_t {
        return create(getRed(color), green, getBlue(color));
    }

    static auto setBlue(const uint32_t color, const uint32_t blue) -> uint32_t {
        return create(getRed(color), getGreen(color), blue);
    }
};

} // namespace laby::basic


#endif /* BASIC_COLOR_H_ */

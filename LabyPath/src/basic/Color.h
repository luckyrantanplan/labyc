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
    static constexpr uint32_t kChannelMask = 0x000000FFU;
    static constexpr double kColorComponentMax = static_cast<double>(kChannelMask);

    static auto getChannel(const uint32_t color, const uint32_t bitOffset) -> uint32_t {
        return (color >> bitOffset) & kChannelMask;
    }

    static auto getRedNormalized(const uint32_t color) -> double {
        return getRed(color) / kColorComponentMax;
    }

    static auto getGreenNormalized(const uint32_t color) -> double {
        return getGreen(color) / kColorComponentMax;
    }

    static auto getBlueNormalized(const uint32_t color) -> double {
        return getBlue(color) / kColorComponentMax;
    }

    static auto getRed(const uint32_t color) -> uint32_t {
        return getChannel(color,
                          static_cast<uint32_t>(svgpp::factory::color::rgb8_policy::r_offset));
    }

    static auto getGreen(const uint32_t color) -> uint32_t {
        return getChannel(color,
                          static_cast<uint32_t>(svgpp::factory::color::rgb8_policy::g_offset));
    }

    static auto getBlue(const uint32_t color) -> uint32_t {
        return getChannel(color,
                          static_cast<uint32_t>(svgpp::factory::color::rgb8_policy::b_offset));
    }

    static auto create(const uint32_t red, const uint32_t green, const uint32_t blue)
        -> uint32_t {
        using svgpp::factory::color::rgb8_policy;
        return static_cast<uint32_t>(rgb8_policy::preset_bits) |
               (red << static_cast<uint32_t>(rgb8_policy::r_offset)) |
               (green << static_cast<uint32_t>(rgb8_policy::g_offset)) |
               (blue << static_cast<uint32_t>(rgb8_policy::b_offset));
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

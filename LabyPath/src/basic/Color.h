/*
 * Color.h
 *
 *  Created on: Aug 9, 2018
 *      Author: florian
 */

#ifndef BASIC_COLOR_H_
#define BASIC_COLOR_H_

#include <bits/stdint-uintn.h>
#include <svgpp/factory/integer_color.hpp>

namespace laby {
namespace basic {

class Color {
public:
    static constexpr double q = 0x000000FF;
    static constexpr uint32_t mask = 0x000000FF;

    static uint32_t get_colorp(const uint32_t color, const int offset) {
        return color >> offset & 0x000000FF;

    }

    static double get_redp(const uint32_t color) {
        return get_red(color) / q;
    }

    static double get_greenp(const uint32_t color) {
        return get_green(color) / q;
    }
    static double get_bluep(const uint32_t color) {
        return get_blue(color) / q;
    }

    static uint32_t get_red(const uint32_t color) {
        return get_colorp(color, svgpp::factory::color::rgb8_policy::r_offset);
    }

    static uint32_t get_green(const uint32_t color) {
        return get_colorp(color, svgpp::factory::color::rgb8_policy::g_offset);
    }

    static uint32_t get_blue(const uint32_t color) {
        return get_colorp(color, svgpp::factory::color::rgb8_policy::b_offset);
    }

    static uint32_t create(const uint32_t r, const uint32_t g, const uint32_t b) {
        using svgpp::factory::color::rgb8_policy;
        return rgb8_policy::preset_bits | (r << rgb8_policy::r_offset) | (g << rgb8_policy::g_offset) | (b << rgb8_policy::b_offset);
    }

    static uint32_t set_red(const uint32_t color, const uint32_t red) {
        return create(red, get_green(color), get_blue(color));
    }

    static uint32_t set_green(const uint32_t color, const uint32_t green) {
        return create(get_red(color), green, get_blue(color));
    }

    static uint32_t set_blue(const uint32_t color, const uint32_t blue) {
        return create(get_red(color), get_green(color), blue);
    }
};

} /* namespace basic */
} /* namespace laby */

#endif /* BASIC_COLOR_H_ */

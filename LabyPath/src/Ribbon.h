/*
 * Ribbon.h
 *
 *  Created on: Feb 26, 2018
 *      Author: florian
 */

#ifndef RIBBON_H_
#define RIBBON_H_

#include <cstdint>

#include <cstddef>
#include <vector>

#include "GeomData.h"
#include "Polyline.h"

namespace laby {
class CoverSpatialIndex;
} /* namespace laby */

namespace laby {

struct Ele {

    Ele(std::size_t imin, std::size_t imax) {
        min = imin;
        max = imax;
    }

    std::size_t min;
    std::size_t max;

};

class Ribbon {

public:
    std::vector<Ribbon> splitRibbon(const double thickness, const int octave);
    std::vector<Kernel::Segment_2> get_segments() const;
    std::vector<Point_2> get_Points() const;
    Arrangement_2 createArr() const;
    void reverse();
    static Arrangement_2 createArr(const Ribbon & r1, const Ribbon & r2);
    static Arrangement_2 createArr(const std::vector<Ribbon> & ribList);
    static void appendToArr(const Ribbon & r1, const Ribbon & r2, Arrangement_2& arr);

    static Ribbon createRibbonOfEdge(const Arrangement_2& arr, const double simplification);


    Ribbon(const int32_t fill_color = 0, const std::vector<Polyline>& lines = { }) :
            _lines { lines }, _fill_color { fill_color } {

    }

    int32_t fill_color() const {
        return _fill_color;
    }
    void set_fill_color(const int32_t direction) {
        _fill_color = direction;
    }

    std::vector<Polyline>& lines() {
        return _lines;
    }

    const std::vector<Polyline>& lines() const {
        return _lines;
    }
    void addToSegments(std::vector<Segment_info_2>& listSeg) const;

    void order_lines();

    Ribbon give_space(const double& space, const double& subdivision_factor, const double& minimal_length) const;

    Ribbon subdived(const double& thickness) const;

    Ribbon subRibbon(const double& space, const double& minimal_length) const;

    void print(std::ostream& os) const {

        os << " _fill_color " << _fill_color << " _lines " << _lines;

    }
    void simplify(const double dist);

    int32_t strokeColor() const {
        return _stroke_color;
    }

    void setStrokeColor(int32_t strokeColor) {
        _stroke_color = strokeColor;
    }

    double strokeWidth() const {
        return stroke_width;
    }

    void setStrokeWidth(double strokeWidth) {
        stroke_width = strokeWidth;
    }

private:
    std::vector<Polyline> _lines;
    int32_t _fill_color = 0;
    int32_t _stroke_color = 0;
    double stroke_width = 1.;

    std::vector<std::size_t> middleOrder(std::size_t min, std::size_t max) const;
    bool isLongEnough(const std::vector<Point_2>& coarse, double thickness) const;
}
;

} /* namespace laby */

#endif /* RIBBON_H_ */

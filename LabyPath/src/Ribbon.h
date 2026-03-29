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

struct IndexRange {
    std::size_t min;
    std::size_t max;
};

struct GiveSpaceConfig {
    double space;
    double subdivisionFactor;
    double minimalLength;
};

struct SubRibbonConfig {
    double space;
    double minimalLength;
};

struct SplitRibbonConfig {
    double thickness;
    int octave;
};

class Ribbon {

  public:
    [[nodiscard]] auto splitRibbon(SplitRibbonConfig config) const -> std::vector<Ribbon>;
    [[nodiscard]] auto getSegments() const -> std::vector<Kernel::Segment_2>;
    [[nodiscard]] auto getPoints() const -> std::vector<Point_2>;
    [[nodiscard]] auto createArr() const -> Arrangement_2;
    void reverse();
    static auto createArr(const Ribbon& firstRibbon, const Ribbon& secondRibbon) -> Arrangement_2;
    static auto createArr(const std::vector<Ribbon>& ribbonList) -> Arrangement_2;
    static void appendToArr(const Ribbon& firstRibbon, const Ribbon& secondRibbon,
                            Arrangement_2& arrangement);

    [[nodiscard]] static auto createRibbonOfEdge(const Arrangement_2& arrangement,
                                                 double simplification) -> Ribbon;

    explicit Ribbon(int32_t fillColor = 0, const std::vector<Polyline>& lines = {})
        : _lines{lines}, _fillColor{fillColor} {}

    [[nodiscard]] auto fillColor() const -> int32_t {
        return _fillColor;
    }
    void setFillColor(int32_t color) {
        _fillColor = color;
    }

    auto lines() -> std::vector<Polyline>& {
        return _lines;
    }

    [[nodiscard]] auto lines() const -> const std::vector<Polyline>& {
        return _lines;
    }
    void addToSegments(std::vector<Segment_info_2>& segmentList) const;

    void orderLines();

    [[nodiscard]] auto giveSpace(GiveSpaceConfig config) const -> Ribbon;

    [[nodiscard]] auto subdived(double thickness) const -> Ribbon;

    [[nodiscard]] auto subRibbon(SubRibbonConfig config) const -> Ribbon;

    void print(std::ostream& outputStream) const {
        outputStream << " _fill_color " << _fillColor << " _lines " << _lines;
    }
    void simplify(double dist);

    [[nodiscard]] auto strokeColor() const -> int32_t {
        return _strokeColor;
    }

    void setStrokeColor(int32_t strokeColor) {
        _strokeColor = strokeColor;
    }

    [[nodiscard]] auto strokeWidth() const -> double {
        return _strokeWidth;
    }

    void setStrokeWidth(double width) {
        _strokeWidth = width;
    }

  private:
    std::vector<Polyline> _lines{};
    int32_t _fillColor = 0;
    int32_t _strokeColor = 0;
    double _strokeWidth = 1.;

    [[nodiscard]] static auto middleOrder(std::size_t minIndex,
                                          std::size_t maxIndex) -> std::vector<std::size_t>;
    [[nodiscard]] static auto isLongEnough(const std::vector<Point_2>& coarsePoints,
                                           double thickness) -> bool;
};

} /* namespace laby */

#endif /* RIBBON_H_ */

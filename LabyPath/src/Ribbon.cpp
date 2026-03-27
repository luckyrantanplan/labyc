/*
 * Ribbon.cpp
 *
 *  Created on: Feb 26, 2018
 *      Author: florian
 */

#include "Ribbon.h"

#include "Rendering/GraphicRendering.h"
#include "basic/EasyProfilerCompat.h"
#include "basic/SimplifyLines.h"
#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Compact_container.h>
#include <CGAL/Delaunay_triangulation_2.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Point_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Triangulation_vertex_base_2.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>
#include <boost/geometry/geometries/point_xy.hpp>
#include <cmath>
#include <cstdint>
#include <map>
#include <queue>
#include <utility>

namespace laby {

namespace {

using VertexHandlePS = CGAL::Point_set_2<Kernel>::Vertex_handle;

constexpr double kInitialScale = 1.0;
constexpr double kScaleMultiplier = 2.0;

void resetEdgeVisits(const Arrangement_2& arrangement) {
    for (const Halfedge& edgeIterator :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        const Segment_info_2& curve = edgeIterator.curve();
        curve.data().setVisit(-1);
    }
}

void appendBranchPolyline(const Halfedge& startHalfedge, Ribbon& ribbon) {
    ribbon.lines().emplace_back();
    Polyline& polyline = ribbon.lines().back();

    for (const Halfedge& halfedge : RangeHelper::make(startHalfedge.twin()->ccb())) {
        polyline.points.emplace_back(halfedge.source()->point());

        if (halfedge.curve().data().getVisit() == 1) {
            break;
        }

        if (halfedge.target()->degree() != 2) {
            halfedge.curve().data().setVisit(1);
            polyline.points.emplace_back(halfedge.target()->point());
            break;
        }
        halfedge.curve().data().setVisit(1);
    }
}

void collectBranchPolylines(const Arrangement_2& arrangement, Ribbon& ribbon) {
    for (const Vertex& vertex :
         RangeHelper::make(arrangement.vertices_begin(), arrangement.vertices_end())) {
        if (!vertex.is_isolated() && vertex.degree() != 2) {
            for (const Halfedge& edgeIterator : RangeHelper::make(vertex.incident_halfedges())) {
                if (edgeIterator.curve().data().getVisit() == -1) {
                    appendBranchPolyline(edgeIterator, ribbon);
                }
            }
        }
    }
}

void appendLoopPolyline(const Halfedge& startHalfedge, Ribbon& ribbon) {
    ribbon.lines().emplace_back();
    Polyline& polyline = ribbon.lines().back();
    bool isClosed = true;

    for (const Halfedge& halfedge : RangeHelper::make(startHalfedge.twin()->ccb())) {
        polyline.points.emplace_back(halfedge.source()->point());
        if (halfedge.curve().data().getVisit() == 1) {
            isClosed = false;
            break;
        }
        halfedge.curve().data().setVisit(1);
    }

    if (isClosed) {
        polyline.points.emplace_back(startHalfedge.target()->point());
        polyline.closed = true;
    }
}

void collectLoopPolylines(const Arrangement_2& arrangement, Ribbon& ribbon) {
    for (const Halfedge& edgeIterator :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        if (edgeIterator.curve().data().getVisit() != 1) {
            appendLoopPolyline(edgeIterator, ribbon);
        }
    }
}

} // namespace

auto Ribbon::getPoints() const -> std::vector<Point_2> {
    std::vector<Point_2> pointsVector;
    for (const Polyline& polyline : _lines) {
        if (!polyline.points.empty()) {

            for (const Point_2& point : polyline.points) {
                pointsVector.emplace_back(point);
            }
        }
    }
    return pointsVector;
}

void Ribbon::addToSegments(std::vector<Segment_info_2>& segmentList) const {
    for (const Polyline& polyline : _lines) {
        if (!polyline.points.empty()) {

            Point_2 previousPoint = polyline.points.at(0);
            for (std::size_t pointIndex = 1; pointIndex < polyline.points.size(); ++pointIndex) {
                const Point_2& point = polyline.points.at(pointIndex);
                if (previousPoint != point) {
                    Segment_2 segment{previousPoint, point};
                    const auto coordinate = static_cast<std::size_t>(polyline.id);
                    segmentList.emplace_back(
                        segment, EdgeInfo{_fillColor, EdgeInfo::Coordinate{coordinate}});
                }
                previousPoint = point;
            }
        }
    }
}

auto Ribbon::createArr(const Ribbon& firstRibbon, const Ribbon& secondRibbon) -> Arrangement_2 {
    Arrangement_2 arrangement;
    appendToArr(firstRibbon, secondRibbon, arrangement);
    return arrangement;
}

auto Ribbon::createArr(const std::vector<Ribbon>& ribbonList) -> Arrangement_2 {
    Arrangement_2 arrangement;
    std::vector<Segment_info_2> segmentList;
    for (const Ribbon& ribbon : ribbonList) {
        ribbon.addToSegments(segmentList);
    }
    CGAL::insert(arrangement, segmentList.begin(), segmentList.end());
    return arrangement;
}

auto Ribbon::getSegments() const -> std::vector<Kernel::Segment_2> {
    std::vector<Kernel::Segment_2> segmentList;
    for (const Polyline& polyline : _lines) {
        if (!polyline.points.empty()) {

            Point_2 previousPoint = polyline.points.at(0);
            for (std::size_t pointIndex = 1; pointIndex < polyline.points.size(); ++pointIndex) {
                const Point_2& point = polyline.points.at(pointIndex);
                if (previousPoint == point) {
                    std::cout << "segment is degenerate !!\n";
                } else {

                    segmentList.emplace_back(previousPoint, point);
                }
                previousPoint = point;
            }
        }
    }
    return segmentList;
}

void Ribbon::appendToArr(const Ribbon& firstRibbon, const Ribbon& secondRibbon,
                         Arrangement_2& arrangement) {
    EASY_FUNCTION();
    std::vector<Segment_info_2> segmentList;
    firstRibbon.addToSegments(segmentList);
    secondRibbon.addToSegments(segmentList);
    CGAL::insert(arrangement, segmentList.begin(), segmentList.end());
}

void Ribbon::reverse() {
    for (Polyline& polyline : lines()) {
        polyline.reverse();
    }
}

auto Ribbon::createArr() const -> Arrangement_2 {
    EASY_FUNCTION();
    Arrangement_2 arrangement;
    std::vector<Segment_info_2> segmentList;
    addToSegments(segmentList);
    CGAL::insert(arrangement, segmentList.begin(), segmentList.end());
    return arrangement;
}

auto Ribbon::middleOrder(const std::size_t minIndex,
                         const std::size_t maxIndex) -> std::vector<std::size_t> {
    std::queue<IndexRange> queue;
    std::vector<std::size_t> result;
    result.reserve(maxIndex - minIndex);
    queue.push(IndexRange{minIndex, maxIndex});

    while (!queue.empty()) {

        IndexRange range = queue.front();
        queue.pop();
        const std::size_t middle = (range.min + range.max) / 2;
        result.push_back(middle);
        if (middle != range.min) {
            queue.push({range.min, middle - 1});
        }
        if (middle != range.max) {
            queue.push({middle + 1, range.max});
        }
    }
    return result;
}

auto Ribbon::isLongEnough(const std::vector<Point_2>& coarsePoints,
                          const double thickness) -> bool {
    // TODO : use polyline instead of vector<Point_2>

    if (coarsePoints.size() < 2) {
        return false;
    }
    Polyline polyline(0, coarsePoints);
    return polyline.totalLength() > thickness;
}

void Ribbon::orderLines() {
    for (Polyline& polyline : _lines) {
        polyline.computeMinLexi();
    }
    std::sort(std::begin(_lines), std::end(_lines),
              [](const Polyline& leftPolyline, const Polyline& rightPolyline) {
                  return leftPolyline.min_point < rightPolyline.min_point;
              });

    for (std::size_t i = 0; i < _lines.size(); ++i) {
        _lines.at(i).id = static_cast<int32_t>(i);
    }
}

auto Ribbon::giveSpace(const GiveSpaceConfig config) const -> Ribbon {

    Ribbon result = subdived(config.space / config.subdivisionFactor);

    result.orderLines();
    return result.subRibbon(SubRibbonConfig{config.space, config.minimalLength});
}

auto Ribbon::subdived(const double thickness) const -> Ribbon {
    Ribbon result(_fillColor);
    for (const Polyline& polyline : _lines) {
        result._lines.emplace_back(polyline.id);
        std::vector<Kernel::Point_2>& pointList = result._lines.back().points;
        pointList.emplace_back(polyline.points.front());
        for (std::size_t i = 1; i < polyline.points.size(); ++i) {
            const Point_2& sourcePoint = polyline.points.at(i - 1);
            const Point_2& targetPoint = polyline.points.at(i);
            const double length =
                sqrt(CGAL::to_double(CGAL::squared_distance(sourcePoint, targetPoint)));
            if (length > thickness) {
                const double weight = length / thickness;

                for (int subdivisionIndex = 1; subdivisionIndex < static_cast<int>(weight);
                     ++subdivisionIndex) {
                    const Kernel::Point_2 test = CGAL::barycenter(
                        targetPoint, static_cast<double>(subdivisionIndex) / weight, sourcePoint);
                    pointList.emplace_back(test);
                }
            }
            pointList.emplace_back(targetPoint);
        }
    }
    return result;
}

auto Ribbon::subRibbon(const SubRibbonConfig config) const -> Ribbon {
    EASY_FUNCTION();

    CGAL::Point_set_2<Kernel> pointSet;

    Ribbon coarseRibbon(_fillColor);

    std::map<int32_t, std::vector<Polyline>> flattenedRibbonMap;

    for (const Polyline& polyline : _lines) {
        std::vector<Polyline>& polylines = flattenedRibbonMap[polyline.id];
        polylines.push_back(polyline);
    }

    std::vector<std::vector<Polyline>> flattenedRibbon;
    flattenedRibbon.reserve(flattenedRibbonMap.size());

    for (const auto& pair : flattenedRibbonMap) {
        flattenedRibbon.emplace_back(pair.second);
    }

    std::vector<std::size_t> order = middleOrder(0U, flattenedRibbon.size() - 1);
    for (std::size_t index : order) {

        for (const Polyline& polyline : flattenedRibbon.at(index)) {
            std::vector<Point_2> coarse;

            for (const Point_2& point : polyline.points) {
                VertexHandlePS vertexHandle = pointSet.nearest_neighbor(point);

                if (vertexHandle != nullptr and //
                    CGAL::compare_squared_distance(vertexHandle->point(), point,
                                                   config.space * config.space) != CGAL::LARGER) {

                    if (isLongEnough(coarse, config.minimalLength)) {
                        coarseRibbon.lines().emplace_back(polyline.id, coarse);
                        pointSet.insert(coarse.begin(), coarse.end());
                    }
                    coarse.clear();
                } else {
                    coarse.emplace_back(point);
                }
            }
            if (isLongEnough(coarse, config.minimalLength)) {

                coarseRibbon.lines().emplace_back(polyline.id, coarse);
                pointSet.insert(coarse.begin(), coarse.end());
            }
        }
    }

    return coarseRibbon;
}

auto Ribbon::splitRibbon(const SplitRibbonConfig config) const -> std::vector<Ribbon> {
    std::vector<Ribbon> ribbonStack;
    ribbonStack.emplace_back(*this);
    double scale = kInitialScale;
    for (int octaveIndex = 0; octaveIndex < config.octave; ++octaveIndex) {
        ribbonStack.emplace_back(ribbonStack.back().subRibbon(
            SubRibbonConfig{scale * config.thickness, scale * config.thickness}));
        scale *= kScaleMultiplier;
    }
    return ribbonStack;
}

void Ribbon::simplify(const double dist) {
    for (Polyline& polyline : lines()) {
        polyline.simplify(dist);
    }
}

auto Ribbon::createRibbonOfEdge(const Arrangement_2& arrangement,
                                const double simplification) -> Ribbon {
    EASY_FUNCTION();

    resetEdgeVisits(arrangement);

    Ribbon ribbon;

    collectBranchPolylines(arrangement, ribbon);
    collectLoopPolylines(arrangement, ribbon);

    for (Polyline& polyline : ribbon.lines()) {
        polyline.removeConsecutiveDuplicatePoints();
    }
    if (simplification > 0) {
        ribbon.simplify(simplification);
    }
    return ribbon;
}

} /* namespace laby */

/*
 * streamLine.cpp
 *
 *  Created on: Oct 29, 2017
 *      Author: florian
 */

#include "StreamLine.h"

#include <CGAL/Compact_container.h>
#include <CGAL/Distance_2/Point_2_Point_2.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Segment_2.h>
#include <CGAL/enum.h>
#include <boost/multi_array/base.hpp>
#include <boost/multi_array/multi_array_ref.hpp>
#include <complex>
#include <cstddef>
#include <cstdint>
// #include <CGAL/Kernel/global_functions_3.h>
#include <CGAL/Point_2.h>
#include <CGAL/number_utils.h>
// #include <CGAL/Runge_kutta_integrator_2.h>
// #include <CGAL/Stream_lines_2.h>
#include "GeomData.h"
#include "Polyline.h"
#include "Ribbon.h"
#include "basic/EasyProfilerCompat.h"
#include <CGAL/Triangulation_ds_face_base_2.h>
#include <CGAL/Vector_2.h>
#include <algorithm>
#include <cmath>
#include <future>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
// #include <vector>
#include "../SegmentPS.h"
#include "../basic/RangeHelper.h"
#include "../basic/SimplifyLines.h"

namespace laby::generator {

namespace {

constexpr double kConnectionDistanceSquared = 1.0;
constexpr double kUnitMagnitude = 1.0;
constexpr double kTwo = 2.0;
constexpr double kHalf = 0.5;
constexpr double kPi = 3.14159265358979323846;
constexpr double kQuarterTurn = kPi / 4.0;
constexpr double kHalfTurn = kPi / 2.0;
constexpr double kFullTurn = 2.0 * kPi;

} // namespace

void StreamLine::postStreamCompute(const Strl_iterator_container& stream_lines,
                                   Ribbon& ribbon) const {
    double const div = _config.resolution;
    int32_t lineNumber = 0;

    for (const auto& sit : stream_lines) {
        ribbon.lines().emplace_back(lineNumber);
        ++lineNumber;
        Polyline& polyline = ribbon.lines().back();
        SimplifyLines::LineString lineString;
        for (auto pit = sit.first; pit != sit.second; ++pit) {
            std::complex<double> pointComplex{CGAL::to_double(pit->x()), CGAL::to_double(pit->y())};
            pointComplex = pointComplex / div;
            lineString.emplace_back(SimplifyLines::xy(pointComplex.real(), pointComplex.imag()));
        }
        SimplifyLines::LineString const simpleLine =
            SimplifyLines::decimate(lineString, _config.simplify_distance);
        for (SimplifyLines::xy const simplifiedPoint : simpleLine) {
            polyline.points().emplace_back(simplifiedPoint.x(), simplifiedPoint.y());
        }
    }
    connectExtremInPlace(ribbon);
}

StreamLine::StreamLine(const Config& config) : _config(config), _radialList(0), _circularList(1) {
    if (_config.old_RegularGrid) {
        _xSampleCount = static_cast<std::size_t>(config.size * _config.resolution);
        _ySampleCount = static_cast<std::size_t>(config.size * _config.resolution);
        using FieldIndex = boost::multi_array<std::complex<double>, 2>::index;
        _field.resize(boost::extents[static_cast<FieldIndex>(_xSampleCount)]
                                    [static_cast<FieldIndex>(_ySampleCount)]);
    }
}

void StreamLine::changeLine(const Point_2& midpoint,
                            std::unordered_map<PS::Vertex*, RibbonCoord>& ribbonCoordMap,
                            PS::Vertex* vertex) {
    RibbonCoord const& ribbonCoord = ribbonCoordMap.at(vertex);
    if (ribbonCoord.position() == 0) {
        ribbonCoord.polyline()->points().front() = midpoint;
    } else {
        ribbonCoord.polyline()->points().back() = midpoint;
    }
}

auto StreamLine::connectExtreme(Ribbon& ribbon) -> Ribbon {

    PS pointSet;

    std::vector<SegmentPS> candidateSegments;
    std::set<SegmentPS> existingSegments;
    std::unordered_set<PS::Vertex*> connectedVertices;
    std::unordered_map<PS::Vertex*, RibbonCoord> ribbonCoordMap;
    for (Polyline& poly : ribbon.lines()) {
        Point_2 const& startPoint = poly.points().front();
        Point_2 const& endPoint = poly.points().back();
        PS::Vertex* startVertex = &*pointSet.insert(startPoint);
        PS::Vertex* endVertex = &*pointSet.insert(endPoint);

        ribbonCoordMap.emplace(startVertex, RibbonCoord{&poly, 0});
        ribbonCoordMap.emplace(endVertex, RibbonCoord{&poly, poly.points().size() - 1});
        existingSegments.emplace(startVertex, endVertex);
    }

    for (auto& edge :
         RangeHelper::make(pointSet.finite_edges_begin(), pointSet.finite_edges_end())) {
        PS::Face_handle const& faceHandle = edge.first;
        int const edgeIndex = edge.second;
        SegmentPS const segment(&*faceHandle->vertex(PS::cw(edgeIndex)),
                                &*faceHandle->vertex(PS::ccw(edgeIndex)));
        if (existingSegments.count(segment) == 0) {
            candidateSegments.emplace_back(segment);
        }
    }
    std::sort(candidateSegments.begin(), candidateSegments.end());
    Ribbon result(ribbon.fillColor());
    for (SegmentPS const& segment : candidateSegments) {

        if (connectedVertices.count(segment.source()) == 0 &&
            connectedVertices.count(segment.target()) == 0) {

            if (CGAL::to_double(
                    CGAL::squared_distance(segment.source()->point(), segment.target()->point())) <
                kConnectionDistanceSquared) {

                Point_2 const midpoint =
                    CGAL::midpoint(segment.source()->point(), segment.target()->point());
                changeLine(midpoint, ribbonCoordMap, segment.source());
                changeLine(midpoint, ribbonCoordMap, segment.target());
            } else {

                result.lines().emplace_back();
                Polyline& polyline = result.lines().back();
                polyline.points().emplace_back(segment.source()->point());
                polyline.points().emplace_back(segment.target()->point());
            }
            connectedVertices.emplace(segment.source());
            connectedVertices.emplace(segment.target());
        }
    }

    return result;
}

void StreamLine::connectExtremInPlace(laby::Ribbon& ribbon) {
    Ribbon resultConnect = connectExtreme(ribbon);
    ribbon.lines().insert(ribbon.lines().end(), resultConnect.lines().begin(),
                          resultConnect.lines().end());
}

void StreamLine::render() {
    EASY_FUNCTION();
    int const xSampleCount = static_cast<int>(_xSampleCount);
    int const ySampleCount = static_cast<int>(_ySampleCount);
    Field radial(xSampleCount, ySampleCount, xSampleCount, ySampleCount);
    Field circular(xSampleCount, ySampleCount, xSampleCount, ySampleCount);

    for (int xIndex = 0; xIndex < xSampleCount; ++xIndex) {
        for (int yIndex = 0; yIndex < ySampleCount; ++yIndex) {

            std::complex<double>& fieldVector = _field[xIndex][yIndex];

            radial.set_field(xIndex, yIndex,
                             CGAL::Vector_2<K>(fieldVector.real(), fieldVector.imag()));
            fieldVector *= std::polar<double>(kUnitMagnitude, kHalfTurn);
            circular.set_field(xIndex, yIndex,
                               CGAL::Vector_2<K>(fieldVector.real(), fieldVector.imag()));
        }
    }

    /* the placement of streamlines */

    double const separationDistance = _config.resolution * _config.divisor;
    auto future = std::async(std::launch::async, [&]() {
        auto lines = streamPlacement(radial, separationDistance, _config.dRat);
        postStreamCompute(lines.iterator_container, _radialList);
    });
    auto lines = streamPlacement(circular, separationDistance, _config.dRat);
    postStreamCompute(lines.iterator_container, _circularList);

    future.get();
}

void StreamLine::drawSpiral(const SpiralParameters& parameters) {
    EASY_FUNCTION();

    std::complex<double> const center = parameters.origin * (kUnitMagnitude * _config.resolution);
    double const radiusSquared =
        parameters.radius * parameters.radius * _config.resolution * _config.resolution;

    for (std::size_t xIndex = 0; xIndex < _xSampleCount; ++xIndex) {
        for (std::size_t yIndex = 0; yIndex < _ySampleCount; ++yIndex) {
            std::complex<double> pixel(static_cast<double>(xIndex), static_cast<double>(yIndex));
            pixel -= center;
            std::complex<double> vect = pixel;

            vect /= std::abs(vect);
            vect *= std::polar<double>(kUnitMagnitude, parameters.angleRadians);

            if (std::norm(pixel) > radiusSquared) {

                double localAngle = std::arg(vect);
                localAngle += kQuarterTurn + kFullTurn;
                localAngle *= kTwo / kPi;
                int const angleInt = static_cast<int>(localAngle);

                vect = std::polar<double>(_config.epsilon * kHalf, angleInt * kHalfTurn);
                // vect=0;
            }
            _field[static_cast<long>(xIndex)][static_cast<long>(yIndex)] += vect;
        }
    }
}

void StreamLine::addToArrangement(Arrangement_2& arr) {
    EASY_FUNCTION();
    render();
    Ribbon::appendToArr(_radialList, _circularList, arr);
}

auto StreamLine::generateTriangularField(std::vector<CGAL::Point_2<K>> pointList,
                                         std::vector<CGAL::Vector_2<K>> vectorList,
                                         const Config& config) -> Ribbon {
    FieldTri const triangularField(pointList.begin(), pointList.end(), vectorList.begin());
    StreamLine streamLine(config);
    double const separationDistance = config.resolution * config.divisor;
    Ribbon ribbon;
    auto lines = streamLine.streamPlacement(triangularField, separationDistance, config.dRat);
    streamLine.postStreamCompute(lines.iterator_container, ribbon);
    return ribbon;
}

void StreamLine::VectorCompute::addSegPerp(std::vector<CGAL::Point_2<K>>& pointList,
                                           const CGAL::Segment_2<Kernel>& seg,
                                           std::vector<CGAL::Vector_2<K>>& vectorList) const {

    CGAL::Vector_2<K> vect(_kernelToK(seg));
    vect = vect.perpendicular(CGAL::COUNTERCLOCKWISE);
    //    if (vect.x() < 0) {
    //        vect *= -1;
    //    }

    vect = vect / sqrt(CGAL::to_double(vect.squared_length()));

    pointList.emplace_back(
        _kernelToK(CGAL::barycenter(seg.source(), _epsilon, seg.target())).transform(_scale));

    vectorList.emplace_back(vect);
    pointList.emplace_back(
        _kernelToK(CGAL::barycenter(seg.target(), _epsilon, seg.source())).transform(_scale));
    vectorList.emplace_back(vect);
}

void StreamLine::VectorCompute::addSegLong(std::vector<CGAL::Point_2<K>>& pointList,
                                           const CGAL::Segment_2<Kernel>& seg,
                                           std::vector<CGAL::Vector_2<K>>& vectorList) const {

    CGAL::Vector_2<K> vect(_kernelToK(seg));

    //    if (vect.x() < 0) {
    //        vect *= -1;
    //    }
    vect = vect / sqrt(CGAL::to_double(vect.squared_length()));

    pointList.emplace_back(
        _kernelToK(CGAL::barycenter(seg.source(), _epsilon, seg.target())).transform(_scale));

    vectorList.emplace_back(vect);
    pointList.emplace_back(
        _kernelToK(CGAL::barycenter(seg.target(), _epsilon, seg.source())).transform(_scale));
    vectorList.emplace_back(vect);
}

auto StreamLine::getRadial(const Config& config,
                           const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons)
    -> Ribbon {
    std::vector<CGAL::Point_2<K>> pointList;
    std::vector<CGAL::Vector_2<K>> vectorList;
    VectorCompute const vCompute(config.resolution);

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {
        for (const CGAL::Segment_2<Kernel>& seg : RangeHelper::make(
                 polygon.outer_boundary().edges_begin(), polygon.outer_boundary().edges_end())) {
            vCompute.addSegPerp(pointList, seg, vectorList);
        }
        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {

            for (const CGAL::Segment_2<Kernel>& seg :
                 RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
                vCompute.addSegPerp(pointList, seg.opposite(), vectorList);
            }
        }
    }
    return generateTriangularField(pointList, vectorList, config);
}

auto StreamLine::getLongitudinal(const Config& config,
                                 const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons)
    -> Ribbon {
    std::vector<CGAL::Point_2<K>> pointList;
    std::vector<CGAL::Vector_2<K>> vectorList;
    VectorCompute const vCompute(config.resolution);
    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {

        for (const CGAL::Segment_2<Kernel> seg : RangeHelper::make(
                 polygon.outer_boundary().edges_begin(), polygon.outer_boundary().edges_end())) {
            vCompute.addSegLong(pointList, seg, vectorList);
        }
        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {

            for (const CGAL::Segment_2<Kernel>& seg :
                 RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
                //    vCompute.addSegLong(pointList, seg.opposite(), vectorList);
            }
        }
    }
    return generateTriangularField(pointList, vectorList, config);
}

} // namespace laby::generator

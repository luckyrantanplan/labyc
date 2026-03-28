/*
 * streamLine.cpp
 *
 *  Created on: Oct 29, 2017
 *      Author: florian
 */

#include "StreamLine.h"

#include <complex>
#include <CGAL/Distance_2/Point_2_Point_2.h>
#include <cstddef>
#include <CGAL/Segment_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <cstdint>
#include <boost/multi_array/base.hpp>
#include <boost/multi_array/multi_array_ref.hpp>
#include <CGAL/Compact_container.h>
#include <CGAL/enum.h>
#include <CGAL/Kernel/global_functions_2.h>
// #include <CGAL/Kernel/global_functions_3.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
// #include <CGAL/Runge_kutta_integrator_2.h>
// #include <CGAL/Stream_lines_2.h>
#include <CGAL/Triangulation_ds_face_base_2.h>
#include <CGAL/Vector_2.h>
#include "Ribbon.h"
#include "Polyline.h"
#include "GeomData.h"
#include "basic/EasyProfilerCompat.h"
#include <algorithm>
#include <cmath>
#include <future>
#include <math.h>
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

void StreamLine::postStreamCompute(const Strl_iterator_container& stream_lines, Ribbon& ribbon) {
    double const div = _config.resolution;
    int32_t lineNumber = 0;

    for (const auto& sit : stream_lines) {
        ribbon.lines().emplace_back(lineNumber);
        ++lineNumber;
        Polyline& pl = ribbon.lines().back();
        SimplifyLines::LineString lineString;
        for (auto pit = sit.first; pit != sit.second; ++pit) {
            std::complex<double> p{CGAL::to_double(pit->x()), CGAL::to_double(pit->y())};
            p = p / div;
            lineString.emplace_back(SimplifyLines::xy(p.real(), p.imag()));
        }
        SimplifyLines::LineString const simpleLine = SimplifyLines::decimate(lineString, _config.simplify_distance);
        for (SimplifyLines::xy const p : simpleLine) {
            pl.points.emplace_back(p.x(), p.y());
        }
    }
    connectExtremInPlace(ribbon);
}

StreamLine::StreamLine(const Config& config) : _config(config), _radialList(0), _circularList(1) {
    if (_config.old_RegularGrid) {
        auto xSamples = static_cast<uint32_t>(config.size * _config.resolution);
        auto ySamples = static_cast<uint32_t>(config.size * _config.resolution);
        _field.resize(boost::extents[xSamples][ySamples]);
    }
}

void StreamLine::changeLine(const Point_2& mid, std::unordered_map<PS::Vertex*, RibbonCoord>& map, PS::Vertex* v1) {
    RibbonCoord const& r1 = map.at(v1);
    if (r1._pos == 0) {
        r1._pl->points.front() = mid;
    }
    else {
        r1._pl->points.back() = mid;
    }
}

auto StreamLine::connectExtreme(Ribbon& ribbon) -> Ribbon {

    PS pointSet;

    std::vector<SegmentPS> vectSeg;
    std::set<SegmentPS> isAlreadyInside;
    std::unordered_set<PS::Vertex*> set;
    std::unordered_map<PS::Vertex*, RibbonCoord> map;
    for (Polyline& poly : ribbon.lines()) {
        Point_2 const& p1 = poly.points.front();
        Point_2 const& p2 = poly.points.back();
        PS::Vertex* v1 = &*pointSet.insert(p1);
        PS::Vertex* v2 = &*pointSet.insert(p2);

        map.emplace(v1, RibbonCoord{&poly, 0});
        map.emplace(v2, RibbonCoord{&poly, poly.points.size() - 1});
        isAlreadyInside.emplace(v1, v2);
    }

    for (auto& edge : RangeHelper::make(pointSet.finite_edges_begin(), pointSet.finite_edges_end())) {
        PS::Face_handle const& fh = edge.first;
        int const i = edge.second;
        SegmentPS const seg(&*fh->vertex(PS::cw(i)), &*fh->vertex(PS::ccw(i)));
        if (isAlreadyInside.count(seg) == 0) {
            vectSeg.emplace_back(seg);
        }
    }
    std::sort(vectSeg.begin(), vectSeg.end());
    Ribbon result(ribbon.fillColor());
    for (SegmentPS const& seg : vectSeg) {

        if (set.count(seg.source()) == 0 and set.count(seg.target()) == 0) {

            if (CGAL::to_double(CGAL::squared_distance(seg.source()->point(), seg.target()->point())) < 1) {

                Point_2 const mid = CGAL::midpoint(seg.source()->point(), seg.target()->point());
                changeLine(mid, map, seg.source());
                changeLine(mid, map, seg.target());
            }
            else {

                result.lines().emplace_back();
                Polyline& poly = result.lines().back();
                poly.points.emplace_back(seg.source()->point());
                poly.points.emplace_back(seg.target()->point());
            }
            set.emplace(seg.source());
            set.emplace(seg.target());
        }
    }

    return result;
}

void StreamLine::connectExtremInPlace(laby::Ribbon& ribbon) {
    Ribbon resultConnext = connectExtreme(ribbon);
    ribbon.lines().insert(ribbon.lines().end(), resultConnext.lines().begin(), resultConnext.lines().end());
}

void StreamLine::render() {
    EASY_FUNCTION();
    auto xSamples = static_cast<int>(_field.size());
    auto ySamples = static_cast<int>(_field.shape()[1]);
    Field radial(xSamples, ySamples, xSamples, ySamples);
    Field circular(xSamples, ySamples, xSamples, ySamples);

    for (int i = 0; i < xSamples; ++i) {
        for (int j = 0; j < ySamples; ++j) {

            std::complex<double>& vect = _field[i][j];

            radial.set_field(i, j, CGAL::Vector_2<K>(vect.real(), vect.imag()));
            vect *= std::polar<double>(1., M_PI / 2.);
            circular.set_field(i, j, CGAL::Vector_2<K>(vect.real(), vect.imag()));
        }
}

    /* the placement of streamlines */

    double dSep = _config.resolution * _config.divisor; // 3.5;
    ;
    auto future = std::async(std::launch::async, [&]() {
        auto lines = streamPlacement(radial, dSep, _config.dRat);
        postStreamCompute(lines.iterator_container, _radialList);
    });
    auto lines = streamPlacement(circular, dSep, _config.dRat);
    postStreamCompute(lines.iterator_container, _circularList);

    future.get();
}

void StreamLine::drawSpiral(const std::complex<double>& o, const double& r, const double& angle) {
    EASY_FUNCTION();
    std::complex<double> const center = o * (1. * _config.resolution);
    double const rSqr = r * r * _config.resolution * _config.resolution;

    for (std::size_t i = 0; i < _field.size(); ++i) {
        for (std::size_t j = 0; j < _field.shape()[1]; ++j) {
            std::complex<double> pixel(static_cast<double>(i), static_cast<double>(j));
            pixel -= center;
            std::complex<double> vect = pixel;

            vect /= std::abs(vect);
            vect *= std::polar<double>(1., angle);

            if (std::norm(pixel) > rSqr) {

                double localAngle = std::arg(vect);
                localAngle += M_PI / 4 + 2 * M_PI;
                localAngle *= 2 / M_PI;
                int const angleInt = static_cast<int>(localAngle);

                vect = std::polar<double>(_config.epsilon * 0.5, angleInt * M_PI * 0.5);
                // vect=0;
            }
            _field[static_cast<long>(i)][static_cast<long>(j)] += vect;
        }
    }
}

void StreamLine::addToArrangement(Arrangement_2& arr) {
    EASY_FUNCTION();
    render();
    Ribbon::appendToArr(_radialList, _circularList, arr);
}

auto StreamLine::generateTriangularField(std::vector<CGAL::Point_2<K>> pointList, std::vector<CGAL::Vector_2<K>> vectorList, const Config& config) -> Ribbon {
    FieldTri const triangularField(pointList.begin(), pointList.end(), vectorList.begin());
    StreamLine st(config);
    double const dSep = config.resolution * config.divisor; // 3.5;
    Ribbon ribbon;
    auto lines = st.streamPlacement(triangularField, dSep, config.dRat);
    st.postStreamCompute(lines.iterator_container, ribbon);
    return ribbon;
}

void StreamLine::VectorCompute::addSegPerp(std::vector<CGAL::Point_2<K>>& pointList, const CGAL::Segment_2<Kernel>& seg,
                                           std::vector<CGAL::Vector_2<K>>& vectorList) const {

    CGAL::Vector_2<K> vect(kernel_to_k(seg));
    vect = vect.perpendicular(CGAL::COUNTERCLOCKWISE);
    //    if (vect.x() < 0) {
    //        vect *= -1;
    //    }

    vect = vect / sqrt(CGAL::to_double(vect.squared_length()));

    pointList.emplace_back(kernel_to_k(CGAL::barycenter(seg.source(), epsilon, seg.target())).transform(scale));

    vectorList.emplace_back(vect);
    pointList.emplace_back(kernel_to_k(CGAL::barycenter(seg.target(), epsilon, seg.source())).transform(scale));
    vectorList.emplace_back(vect);
}

void StreamLine::VectorCompute::addSegLong(std::vector<CGAL::Point_2<K>>& pointList, const CGAL::Segment_2<Kernel>& seg,
                                           std::vector<CGAL::Vector_2<K>>& vectorList) const {

    CGAL::Vector_2<K> vect(kernel_to_k(seg));

    //    if (vect.x() < 0) {
    //        vect *= -1;
    //    }
    vect = vect / sqrt(CGAL::to_double(vect.squared_length()));

    pointList.emplace_back(kernel_to_k(CGAL::barycenter(seg.source(), epsilon, seg.target())).transform(scale));

    vectorList.emplace_back(vect);
    pointList.emplace_back(kernel_to_k(CGAL::barycenter(seg.target(), epsilon, seg.source())).transform(scale));
    vectorList.emplace_back(vect);
}

auto StreamLine::getRadial(const Config& config, const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons) -> Ribbon {
    std::vector<CGAL::Point_2<K>> pointList;
    std::vector<CGAL::Vector_2<K>> vectorList;
    VectorCompute const vCompute(config.resolution);

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {
        for (const CGAL::Segment_2<Kernel>& seg : RangeHelper::make(polygon.outer_boundary().edges_begin(), polygon.outer_boundary().edges_end())) {
            vCompute.addSegPerp(pointList, seg, vectorList);
        }
        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {

            for (const CGAL::Segment_2<Kernel>& seg : RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
                vCompute.addSegPerp(pointList, seg.opposite(), vectorList);
            }
        }
    }
    return generateTriangularField(pointList, vectorList, config);
}

auto StreamLine::getLongitudinal(const Config& config, const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons) -> Ribbon {
    std::vector<CGAL::Point_2<K>> pointList;
    std::vector<CGAL::Vector_2<K>> vectorList;
    VectorCompute const vCompute(config.resolution);
    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {

        for (const CGAL::Segment_2<Kernel> seg : RangeHelper::make(polygon.outer_boundary().edges_begin(), polygon.outer_boundary().edges_end())) {
            vCompute.addSegLong(pointList, seg, vectorList);
        }
        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {

            for (const CGAL::Segment_2<Kernel>& seg : RangeHelper::make(hole.edges_begin(), hole.edges_end())) {
                //    vCompute.addSegLong(pointList, seg.opposite(), vectorList);
            }
        }
    }
    return generateTriangularField(pointList, vectorList, config);
}

} // namespace laby::generator


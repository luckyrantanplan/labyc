/*
 * streamLine.cpp
 *
 *  Created on: Oct 29, 2017
 *      Author: florian
 */

#include "StreamLine.h"

#include <cstdint>
#include <boost/geometry/geometries/point_xy.hpp>
#include <boost/multi_array/base.hpp>
#include <boost/multi_array/multi_array_ref.hpp>
#include <CGAL/aff_transformation_tags.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Compact_container.h>
#include <CGAL/enum.h>
#include <CGAL/Kernel/global_functions_2.h>
//#include <CGAL/Kernel/global_functions_3.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
//#include <CGAL/Runge_kutta_integrator_2.h>
//#include <CGAL/Stream_lines_2.h>
#include <CGAL/Triangulation_ds_face_base_2.h>
#include <CGAL/Triangulation_utils_2.h>
#include <CGAL/Vector_2.h>
#include "basic/EasyProfilerCompat.h"
#include <algorithm>
#include <cmath>
#include <future>
#include <iostream>
#include <list>
#include <set>
#include <unordered_set>
#include <utility>
//#include <vector>
#include "../SegmentPS.h"
#include "../basic/RangeHelper.h"
#include "../basic/SimplifyLines.h"

namespace laby {
namespace generator {

void StreamLine::postStreamCompute(const Strl_iterator_container& stream_lines, Ribbon& ribbon) {
    double div = _config.resolution;
    int32_t lineNumber = 0;

    for (auto& sit : stream_lines) {
        ribbon.lines().emplace_back(lineNumber);
        ++lineNumber;
        Polyline& pl = ribbon.lines().back();
        SimplifyLines::LineString lineString;
        for (Strl_polyline::const_iterator pit = sit.first; pit != sit.second; ++pit) {
            std::complex<double> p { CGAL::to_double(pit->x()), CGAL::to_double(pit->y()) };
            p = p / div;
            lineString.emplace_back(SimplifyLines::xy(p.real(), p.imag()));
        }
        SimplifyLines::LineString simpleLine = SimplifyLines::decimate(lineString, _config.simplify_distance);
        for (SimplifyLines::xy p : simpleLine) {
            pl.points.emplace_back(p.x(), p.y());
        }
    }
    connectExtremInPlace(ribbon);
}

StreamLine::StreamLine(const Config& config) :
        _config(config), _radialList(0), _circularList(1) {
    if (_config.old_RegularGrid) {
        uint32_t x_samples = static_cast<uint32_t>(config.size * _config.resolution);
        uint32_t y_samples = static_cast<uint32_t>(config.size * _config.resolution);
        _field.resize(boost::extents[x_samples][y_samples]);
    }
}

void StreamLine::changeLine(const Point_2& mid, std::unordered_map<PS::Vertex*, RibbonCoord>& map, PS::Vertex* v1) {
    RibbonCoord& r1 = map.at(v1);
    if (r1._pos == 0) {
        r1._pl->points.front() = mid;
    } else {
        r1._pl->points.back() = mid;
    }
}

Ribbon StreamLine::connectExtreme(Ribbon& ribbon) {

    PS pointSet;

    std::vector<SegmentPS> vectSeg;
    std::set<SegmentPS> isAlreadyInside;
    std::unordered_set<PS::Vertex*> set;
    std::unordered_map<PS::Vertex*, RibbonCoord> map;
    for (Polyline& poly : ribbon.lines()) {
        Point_2& p1 = poly.points.front();
        Point_2& p2 = poly.points.back();
        PS::Vertex* v1 = &*pointSet.insert(p1);
        PS::Vertex* v2 = &*pointSet.insert(p2);

        map.emplace(v1, RibbonCoord { &poly, 0 });
        map.emplace(v2, RibbonCoord { &poly, poly.points.size() - 1 });
        isAlreadyInside.emplace(v1, v2);
    }

    for (auto& edge : RangeHelper::make(pointSet.finite_edges_begin(), pointSet.finite_edges_end())) {
        PS::Face_handle & fh = edge.first;
        int i = edge.second;
        SegmentPS seg(&*fh->vertex(PS::cw(i)), &*fh->vertex(PS::ccw(i)));
        if (isAlreadyInside.count(seg) == 0) {
            vectSeg.emplace_back(seg);
        }

    }
    std::sort(vectSeg.begin(), vectSeg.end());
    Ribbon result(ribbon.fill_color());
    for (SegmentPS& seg : vectSeg) {

        if (set.count(seg.source()) == 0 and set.count(seg.target()) == 0) {

            if (CGAL::to_double(CGAL::squared_distance(seg.source()->point(), seg.target()->point())) < 1) {

                Point_2 mid = CGAL::midpoint(seg.source()->point(), seg.target()->point());
                changeLine(mid, map, seg.source());
                changeLine(mid, map, seg.target());
            } else {

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
    auto x_samples = static_cast<int>(_field.size());
    auto y_samples = static_cast<int>(_field.shape()[1]);
    Field radial(x_samples, y_samples, x_samples, y_samples);
    Field circular(x_samples, y_samples, x_samples, y_samples);

    for (int i = 0; i < x_samples; ++i)
        for (int j = 0; j < y_samples; ++j) {

            std::complex<double> &vect = _field[i][j];

            radial.set_field(i, j, CGAL::Vector_2<K>(vect.real(), vect.imag()));
            vect *= std::polar<double>(1., M_PI / 2.);
            circular.set_field(i, j, CGAL::Vector_2<K>(vect.real(), vect.imag()));
        }

    /* the placement of streamlines */

    double dSep = _config.resolution * _config.divisor; //3.5;
    ;
    auto future = std::async(std::launch::async, [&]() {
        auto lines=streamPlacement(radial, dSep, _config.dRat);
        postStreamCompute(lines.iterator_container , _radialList);});
    auto lines = streamPlacement(circular, dSep, _config.dRat);
    postStreamCompute(lines.iterator_container, _circularList);

    future.get();

}

void StreamLine::drawSpiral(const std::complex<double>& o, const double& r, const double& angle) {
    EASY_FUNCTION();
    std::complex<double> center = o * (1. * _config.resolution);
    double rSqr = r * r * _config.resolution * _config.resolution;

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
                int angleInt = static_cast<int>(localAngle);

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

Ribbon StreamLine::generateTriangularField(std::vector<CGAL::Point_2<K> > pointList, std::vector<CGAL::Vector_2<K> > vectorList, const Config& config) {
    FieldTri triangular_field(pointList.begin(), pointList.end(), vectorList.begin());
    StreamLine st(config);
    double dSep = config.resolution * config.divisor; //3.5;
    Ribbon ribbon;
    auto lines = st.streamPlacement(triangular_field, dSep, config.dRat);
    st.postStreamCompute(lines.iterator_container, ribbon);
    return ribbon;
}

void StreamLine::VectorCompute::addSegPerp(std::vector<CGAL::Point_2<K> >& pointList, const CGAL::Segment_2<Kernel>& seg, std::vector<CGAL::Vector_2<K> >& vectorList) {

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

void StreamLine::VectorCompute::addSegLong(std::vector<CGAL::Point_2<K> >& pointList, const CGAL::Segment_2<Kernel>& seg, std::vector<CGAL::Vector_2<K> >& vectorList) {

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

Ribbon StreamLine::getRadial(const Config& config, const std::vector<CGAL::Polygon_with_holes_2<Kernel> >& polygons) {
    std::vector<CGAL::Point_2<K>> pointList;
    std::vector<CGAL::Vector_2<K>> vectorList;
    VectorCompute vCompute(config.resolution);

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

Ribbon StreamLine::getLongitudinal(const Config& config, const std::vector<CGAL::Polygon_with_holes_2<Kernel> >& polygons) {
    std::vector<CGAL::Point_2<K>> pointList;
    std::vector<CGAL::Vector_2<K>> vectorList;
    VectorCompute vCompute(config.resolution);
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

} /* namespace generator */
}/* namespace laby */

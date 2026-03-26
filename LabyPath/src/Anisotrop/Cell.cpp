/*
 * Cell.cpp
 *
 *  Created on: Feb 5, 2018
 *      Author: florian
 */

#include "Cell.h"

#include <cstdint>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arrangement_2/Arrangement_2_iterators.h>
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Point_2.h>
#include <optional>

#include "../basic/NumericRange.h"
#include "../basic/RandomUniDist.h"
#include "../generator/PoissonGenerator.h"

#include "../basic/RangeHelper.h"

namespace laby {
namespace aniso {

Cell::Cell(const proto::Cell& config, Arrangement_2& arr, const Ribbon& limit) : _config(config), _arr{arr}, _random(0, 100.0, _config.seed()) {
    std::vector<Point_2> points = limit.get_Points();

    std::cout << " points.size() " << points.size() << std::endl;

    for (const Polyline& pl : limit.lines()) {
        for (const Point_2& pt : subdivide(pl)) {

            selectNearestPoint(pt);
        }
    }

    _random.shuffle(randomVertex);

    startNetWithRandomPin();

    createRandomPinOnExistingVerticesOnly();
}

double Cell::resolution() const {
    if (_config.resolution() > 0.) {
        return _config.resolution();
    }
    std::cout << "cell resolution <= 0, fallback to 1.0" << std::endl;
    return 1.0;
}

std::vector<Point_2> Cell::subdivide(const Polyline& pl) {
    std::vector<Point_2> result;
    const double subdivision_resolution = resolution();
    for (std::size_t i = 1; i < pl.points.size(); ++i) {

        CGAL::Vector_2<Kernel> vec = pl.points.at(i) - pl.points.at(i - 1);

        double length = sqrt(CGAL::to_double(vec.squared_length()));
        if (length > 0) {
            CGAL::Vector_2<Kernel> u = vec / length;

            u = u * subdivision_resolution;

            result.emplace_back(pl.points.at(i - 1));
            for (int32_t vi = 1; vi < length / subdivision_resolution; ++vi) {

                result.emplace_back(pl.points.at(i - 1) + vi * u);
            }
            if (i + 1 == pl.points.size()) {

                if (pl.closed or pl.points.front() == pl.points.back()) {}
                else {
                    result.emplace_back(pl.points.at(i));
                }
            }
        }
    }
    return result;
}

void Cell::createOutlinedNet(std::size_t begin, double thickness) {

    for (std::size_t i = begin + 1; i < listVertex.size(); ++i) {
        _nets.emplace_back(Pin{*listVertex.at(i), thickness}, Pin{*listVertex.at(i - 1), thickness}, _nets.size());
    }
    _nets.emplace_back(Pin{*listVertex.at(begin), thickness}, Pin{*listVertex.back(), thickness}, _nets.size());
}

void Cell::createRandomPin(const CGAL::Bbox_2& bbox, const std::size_t maxPin) {

    std::vector<std::complex<double>> points = generator::PoissonPoints::generateRectangle(bbox, maxPin + 1);

    for (const std::complex<double> pt : points) {
        Point_2 point2(pt.real(), pt.imag());
        selectNearestPoint(point2);
    }

    std::cout << "number of random vertices : " << randomVertex.size() << std::endl;
}

void Cell::createRandomPinOnExistingVerticesOnly() {
    const std::size_t maxPin = _config.maxpin();
    for (Vertex& v : RangeHelper::make(_arr.vertices_begin(), _arr.vertices_end())) {
        if (v.degree() > 2) {
            randomVertex.emplace_back(&v);
        }
    }
    _random.shuffle(randomVertex);
    randomVertex.resize(std::min(randomVertex.size(), maxPin));
    std::cout << "createRandomPinOnExistingVerticesOnly number of random vertices : " << randomVertex.size() << std::endl;
}

void Cell::startNetWithRandomPin() {
    const std::size_t numberOfStartNet = _config.startnet();
    for (std::size_t i = 0; i < std::min(numberOfStartNet, randomVertex.size()); ++i) {
        listVertex.emplace_back(randomVertex.at(i));
    }
    _random.shuffle(listVertex);
}

void Cell::selectNearestPoint(const Point_2& point2) {

    // TODO change insert_point by locate

    Vertex_handle handle = CGAL::insert_point(_arr, point2);

    if (handle->is_isolated()) {
        Face_handle fh = handle->face();
        Vertex& v_nearest = const_cast<Vertex&>(GeomHelper::getNearestVertex(*fh, *handle));

        if (CGAL::compare_squared_distance(v_nearest.point(), point2, 2.) == CGAL::SMALLER) {
            v_nearest.data().setType(VertexInfo::PIN);
            randomVertex.emplace_back(&v_nearest);
        }
    }
    else {
        std::cout << "not isolated " << std::endl;
    }
}

void Cell::insertPointAndConnect(const Point_2& point2) {
    Vertex_handle handle = CGAL::insert_point(_arr, point2);
    handle->data().setType(VertexInfo::PIN);
    randomVertex.push_back(handle.ptr());
    if (handle->is_isolated()) {
        Face_handle fh = handle->face();
        const Point_2& point = GeomHelper::getNearestVertex(*fh, *handle).point();
        Segment_2 s{handle->point(), point};
        Segment_info_2 segInfo(s, EdgeInfo(EdgeInfo::Type::CELL));
        CGAL::insert(_arr, segInfo);
    }
}

void Cell::shuffleVertices() {

    _random.shuffle(listVertex);
}

void Cell::drawRectOutline(const CGAL::Bbox_2& bbox, const double quantity, const double thickness, const double raylength) {

    std::vector<Point_2> allvertices;
    double resolution = 3;
    { allvertices.emplace_back(bbox.xmin(), bbox.ymin()); }
    double step = 1 / resolution;
    for (double x : NumericRange<double>(bbox.xmin() + step, bbox.xmax() - step, step)) {
        allvertices.emplace_back(x, bbox.ymin());
    }
    for (double y : NumericRange<double>(bbox.ymin(), bbox.ymax() - step, step)) {
        allvertices.emplace_back(bbox.xmax(), y);
    }
    for (double x : NumericRange<double>(bbox.xmax(), bbox.xmin() + step, -step)) {
        allvertices.emplace_back(x, bbox.ymax());
    }
    for (double y : NumericRange<double>(bbox.ymax(), bbox.ymin() + step, -step)) {
        allvertices.emplace_back(bbox.xmin(), y);
    }

    std::size_t begin = listVertex.size();
    for (std::size_t i = 0; i < allvertices.size(); ++i) {
        if (NumericHelper::reduce(static_cast<int32_t>(i), static_cast<int32_t>(allvertices.size()), static_cast<int32_t>(quantity)).has_value()) {
            Vertex_handle handle = CGAL::insert_point(_arr, allvertices.at(i));
            handle->data().setType(VertexInfo::PIN);

            listVertex.emplace_back(handle.ptr());
        }
    }

    std::complex<double> corner1{bbox.xmin(), bbox.ymin()};
    std::complex<double> corner2{bbox.xmax(), bbox.ymax()};
    std::complex<double> o = (corner1 + corner2) / 2.;
    std::vector<Segment_info_2> listSeg;

    for (std::size_t i = begin; i < listVertex.size(); ++i) {
        Vertex* v = listVertex.at(i);
        std::complex<double> c{CGAL::to_double(v->point().x()), CGAL::to_double(v->point().y())};

        std::complex<double> vect = c - o;
        vect *= raylength / std::abs(vect);
        vect += o;

        listSeg.emplace_back(Segment_2(v->point(), Point_2{vect.real(), vect.imag()}), EdgeInfo{EdgeInfo::Type::CELL});
    }

    for (std::size_t i = begin + 1; i < listVertex.size(); ++i) {
        Segment_2 s{listVertex.at(i - 1)->point(), listVertex.at(i)->point()};
        listSeg.emplace_back(s, EdgeInfo{EdgeInfo::Type::CELL});
    }

    Segment_2 s{listVertex.back()->point(), listVertex.at(begin)->point()};
    listSeg.emplace_back(s, EdgeInfo{EdgeInfo::Type::CELL});
    CGAL::insert(_arr, listSeg.begin(), listSeg.end());
    createOutlinedNet(begin, thickness);
}

} /* namespace aniso */
} /* namespace laby */

/*
 * Cell.cpp
 *
 *  Created on: Feb 5, 2018
 *      Author: florian
 */

#include "Cell.h"

#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <utility>

#include <CGAL/Bbox_2.h>

#include "../basic/NumericRange.h"
#include "../generator/PoissonGenerator.h"

#include "GeomData.h"
#include "Polyline.h"
#include "Ribbon.h"
#include "protoc/AllConfig.pb.h"

namespace laby::aniso {

namespace {

constexpr double kFallbackResolution = 1.0;
constexpr double kSnapDistanceSquared = 2.0;
constexpr double kOutlineResolution = 3.0;
constexpr double kMidpointDivisor = 2.0;

} // namespace

Cell::Cell(proto::Cell config, Arrangement_2& arr, const Ribbon& limit)
    : _config(std::move(config)), _arr(&arr), _random(0, 100.0, _config.seed()) {
    std::vector<Point_2> const points = limit.getPoints();

    std::cout << " points.size() " << points.size() << '\n';

    for (const Polyline& polyline : limit.lines()) {
        for (const Point_2& point : subdivide(polyline)) {

            selectNearestPoint(point);
        }
    }

    _random.shuffle(_randomVertices);

    startNetWithRandomPin();

    createRandomPinOnExistingVerticesOnly();
}

auto Cell::resolution() const -> double {
    if (_config.resolution() > 0.) {
        return _config.resolution();
    }
    std::cout << "cell resolution <= 0, fallback to 1.0" << '\n';
    return kFallbackResolution;
}

auto Cell::subdivide(const Polyline& polyline) const -> std::vector<Point_2> {
    std::vector<Point_2> result;
    const double subdivisionResolution = resolution();
    for (std::size_t i = 1; i < polyline.points().size(); ++i) {

        CGAL::Vector_2<Kernel> const vec = polyline.points().at(i) - polyline.points().at(i - 1);

        double const length = std::sqrt(CGAL::to_double(vec.squared_length()));
        if (length > 0) {
            CGAL::Vector_2<Kernel> unitVector = vec / length;

            unitVector = unitVector * subdivisionResolution;

            result.emplace_back(polyline.points().at(i - 1));
            for (int32_t vertexIndex = 1; vertexIndex < length / subdivisionResolution;
                 ++vertexIndex) {

                result.emplace_back(polyline.points().at(i - 1) + vertexIndex * unitVector);
            }
            if (i + 1 == polyline.points().size()) {

                if (polyline.isClosed() or polyline.points().front() == polyline.points().back()) {
                } else {
                    result.emplace_back(polyline.points().at(i));
                }
            }
        }
    }
    return result;
}

void Cell::createOutlinedNet(std::size_t begin, double thickness) {

    for (std::size_t i = begin + 1; i < _listVertex.size(); ++i) {
        _nets.emplace_back(Net::SourcePin{Pin{*_listVertex.at(i), thickness}},
                           Net::TargetPin{Pin{*_listVertex.at(i - 1), thickness}},
                           static_cast<int32_t>(_nets.size()));
    }
    _nets.emplace_back(Net::SourcePin{Pin{*_listVertex.at(begin), thickness}},
                       Net::TargetPin{Pin{*_listVertex.back(), thickness}},
                       static_cast<int32_t>(_nets.size()));
}

void Cell::createRandomPin(const CGAL::Bbox_2& bbox, std::size_t maxPin) {

    std::vector<std::complex<double>> const points =
        generator::PoissonPoints::generateRectangle(bbox, maxPin + 1);

    for (const std::complex<double> point : points) {
        Point_2 const point2(point.real(), point.imag());
        selectNearestPoint(point2);
    }

    std::cout << "number of random vertices : " << _randomVertices.size() << '\n';
}

void Cell::createRandomPinOnExistingVerticesOnly() {
    const std::size_t maxPin = _config.maxpin();
    for (Vertex& vertex : RangeHelper::make(_arr->vertices_begin(), _arr->vertices_end())) {
        if (vertex.degree() > 2) {
            _randomVertices.emplace_back(&vertex);
        }
    }
    _random.shuffle(_randomVertices);
    _randomVertices.resize(std::min(_randomVertices.size(), maxPin));
    std::cout << "createRandomPinOnExistingVerticesOnly number of random vertices : "
              << _randomVertices.size() << '\n';
}

void Cell::startNetWithRandomPin() {
    const std::size_t numberOfStartNet = _config.startnet();
    for (std::size_t i = 0; i < std::min(numberOfStartNet, _randomVertices.size()); ++i) {
        _listVertex.emplace_back(_randomVertices.at(i));
    }
    _random.shuffle(_listVertex);
}

void Cell::selectNearestPoint(const Point_2& point2) {

    // TODO change insert_point by locate

    Vertex_handle const handle = CGAL::insert_point(*_arr, point2);

    if (handle->is_isolated()) {
        Face_handle const faceHandle = handle->face();
        auto& nearestVertex = GeomHelper::getNearestVertex(*faceHandle, *handle);

        if (CGAL::compare_squared_distance(nearestVertex.point(), point2, kSnapDistanceSquared) ==
            CGAL::SMALLER) {
            nearestVertex.data().setType(VertexInfo::PIN);
            _randomVertices.emplace_back(&nearestVertex);
        }
    } else {
        std::cout << "not isolated " << '\n';
    }
}

void Cell::insertPointAndConnect(const Point_2& point2) {
    Vertex_handle const handle = CGAL::insert_point(*_arr, point2);
    handle->data().setType(VertexInfo::PIN);
    _randomVertices.push_back(handle.ptr());
    if (handle->is_isolated()) {
        Face_handle const faceHandle = handle->face();
        const Point_2& point = GeomHelper::getNearestVertex(*faceHandle, *handle).point();
        Segment_2 const segment{handle->point(), point};
        Segment_info_2 const segInfo(segment, EdgeInfo(EdgeInfo::Type::CELL));
        CGAL::insert(*_arr, segInfo);
    }
}

void Cell::shuffleVertices() {

    _random.shuffle(_listVertex);
}

void Cell::drawRectOutline(const CGAL::Bbox_2& bbox, RectOutlineConfig config) {

    std::vector<Point_2> allvertices;
    double const outlineResolution = kOutlineResolution;
    { allvertices.emplace_back(bbox.xmin(), bbox.ymin()); }
    double const step = 1 / outlineResolution;
    for (double const xCoord : NumericRange<double>(bbox.xmin() + step, bbox.xmax() - step, step)) {
        allvertices.emplace_back(xCoord, bbox.ymin());
    }
    for (double const yCoord : NumericRange<double>(bbox.ymin(), bbox.ymax() - step, step)) {
        allvertices.emplace_back(bbox.xmax(), yCoord);
    }
    for (double const xCoord : NumericRange<double>(bbox.xmax(), bbox.xmin() + step, -step)) {
        allvertices.emplace_back(xCoord, bbox.ymax());
    }
    for (double const yCoord : NumericRange<double>(bbox.ymax(), bbox.ymin() + step, -step)) {
        allvertices.emplace_back(bbox.xmin(), yCoord);
    }

    std::size_t const begin = _listVertex.size();
    for (std::size_t i = 0; i < allvertices.size(); ++i) {
        if (NumericHelper::reduce(static_cast<int32_t>(i), static_cast<int32_t>(allvertices.size()),
                                  static_cast<int32_t>(config.quantity))
                .has_value()) {
            Vertex_handle const handle = CGAL::insert_point(*_arr, allvertices.at(i));
            handle->data().setType(VertexInfo::PIN);

            _listVertex.emplace_back(handle.ptr());
        }
    }

    std::complex<double> const corner1{bbox.xmin(), bbox.ymin()};
    std::complex<double> const corner2{bbox.xmax(), bbox.ymax()};
    std::complex<double> const center = (corner1 + corner2) / kMidpointDivisor;
    std::vector<Segment_info_2> listSeg;

    for (std::size_t i = begin; i < _listVertex.size(); ++i) {
        Vertex* vertex = _listVertex.at(i);
        std::complex<double> const vertexPoint{CGAL::to_double(vertex->point().x()),
                                               CGAL::to_double(vertex->point().y())};

        std::complex<double> vect = vertexPoint - center;
        vect *= config.rayLength / std::abs(vect);
        vect += center;

        listSeg.emplace_back(Segment_2(vertex->point(), Point_2{vect.real(), vect.imag()}),
                             EdgeInfo{EdgeInfo::Type::CELL});
    }

    for (std::size_t i = begin + 1; i < _listVertex.size(); ++i) {
        Segment_2 const outlineSegment{_listVertex.at(i - 1)->point(), _listVertex.at(i)->point()};
        listSeg.emplace_back(outlineSegment, EdgeInfo{EdgeInfo::Type::CELL});
    }

    Segment_2 const outlineSegment{_listVertex.back()->point(), _listVertex.at(begin)->point()};
    listSeg.emplace_back(outlineSegment, EdgeInfo{EdgeInfo::Type::CELL});
    CGAL::insert(*_arr, listSeg.begin(), listSeg.end());
    createOutlinedNet(begin, config.thickness);
}

} // namespace laby::aniso

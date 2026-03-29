/*
 * GeomData.h
 *
 *  Created on: Feb 1, 2018
 *      Author: florian
 */

#ifndef GEOMDATA_H_
#define GEOMDATA_H_

#include "basic/RangeHelper.h"
#include <CGAL/Arr_curve_data_traits_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_trapezoid_ric_point_location.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arrangement_2/Arrangement_2_iterators.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Iterator_transform.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Polygon_2.h>
#include <algorithm>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>

namespace laby {

// handy for printing datas
template <class T>
auto operator<<(std::ostream& outputStream, const T& value) -> decltype(value.print(outputStream),
                                                                        outputStream) {
    value.print(outputStream);
    return outputStream;
}

template <typename T>
auto operator<<(std::ostream& outputStream, const std::vector<T>& values) -> std::ostream& {
    if (!values.empty()) {
        outputStream << '[';
        std::copy(values.begin(), values.end(), std::ostream_iterator<T>(outputStream, ", "));
        outputStream << "END]";
    } else {
        outputStream << "[EMPTY]";
    }
    return outputStream;
}

class EdgeInfo;

class VertexInfo;

class Dummy {};
// //typedef CGAL::Cartesian<CGAL::Exact_rational> Kernel;
using Kernel = CGAL::Exact_predicates_exact_constructions_kernel;

using Segment_traits_2 = CGAL::Arr_segment_traits_2<Kernel>;
using Segment_2 = Segment_traits_2::Curve_2;
using Traits_2 = CGAL::Arr_curve_data_traits_2<Segment_traits_2, EdgeInfo>;
using Point_2 = Traits_2::Point_2;
using Segment_info_2 = Traits_2::X_monotone_curve_2;

using Dcel = CGAL::Arr_extended_dcel<Traits_2, VertexInfo, Dummy, Dummy>;

using Arrangement_2 = CGAL::Arrangement_2<Traits_2, Dcel>;

using Vertex_handle = Arrangement_2::Vertex_handle;
using Vertex = Vertex_handle::value_type;

using Vertex_const_handle = Arrangement_2::Vertex_const_handle;
using Halfedge_handle = Arrangement_2::Halfedge_handle;

using Halfedge = Halfedge_handle::value_type;

using Halfedge_const_handle = Arrangement_2::Halfedge_const_handle;
using Face_handle = Arrangement_2::Face_handle;
using Face_const_handle = Arrangement_2::Face_const_handle;
using Face = Face_handle::value_type;

using Edge_iterator = Arrangement_2::Edge_iterator;
using Edge_const_iterator = Arrangement_2::Edge_const_iterator;

using Edge_const = Arrangement_2::Edge_const_iterator::value_type;
using Walk_pl = CGAL::Arr_trapezoid_ric_point_location<Arrangement_2>;
using PolySegment = std::vector<Halfedge*>;

class EdgeInfo {
  public:
    enum Type : int8_t {
        UNDEFINED = 1,
        HORIZONTAL = 2,
        VERTICAL = 3,
        DIAGONAL = 4,
        SPECIAL = 5,
        CELL = 6
    };

    struct Coordinate {
        std::size_t value;
    };

    EdgeInfo(int32_t direction, Coordinate coordinate)
        : _direction{direction}, _coordinate{coordinate.value} {}

    explicit EdgeInfo(Type edgeType) : _direction{edgeType} {}

    EdgeInfo() = default;

    auto direction() const -> int32_t {
        return _direction;
    }

    auto congestion() const -> int32_t {
        return static_cast<int32_t>(_path.size());
    }

    auto hasNet(const int32_t serialId) const -> bool {
        return _path.count(serialId) != 0;
    }

    static void addPath(const int32_t serialId) {
        auto insertResult = _path.emplace(serialId, 1);
        if (!insertResult.second) {
            ++insertResult.first->second;
        }
    }

    void clearAllPath() {
        _path.clear();
    }

    auto getVisit() const -> int32_t {
        return _visit;
    }

    void setVisit(int32_t value = -1) const {
        _visit = value;
    }

    auto operator==(const EdgeInfo& other) const -> bool {
        return _direction == other._direction;
    }

    auto thickness() const -> double {
        return _thickness;
    }

    void setThickness(double thickness = 1) {
        _thickness = thickness;
    }

    auto getNextHalfedge(int32_t visited, const Vertex& vertex) const -> const Halfedge*;

    static void print(std::ostream& outputStream) {
        outputStream << " path ";
        for (const auto& [net_id, count] : _path) {
            outputStream << " " << net_id << "->" << count;
        }
    }

    auto getNet() const -> int32_t {
        return _path.begin()->first;
    }

    auto coordinate() const -> std::size_t {
        return _coordinate;
    }

  private:
    // TODO : replace unordered_map by just an int
    std::unordered_map<int32_t, int32_t> _path{};
    double _thickness = 1;
    mutable int32_t _visit = -1;
    int32_t _direction = -1;
    std::size_t _coordinate = 0;
};

class VertexInfo {
  public:
    enum Type : int8_t { UNDEFINED = 0, STEINER = 1, PIN = 2 };

    VertexInfo() = default;

    auto getType() const -> Type {
        return _type;
    }

    void setType(Type type) {
        _type = type;
    }

    auto getVisit() const -> int32_t {
        return _visit;
    }

    void setVisit(int32_t visitValue, Halfedge* halfedge) {
        _visit = visitValue;
        _halfedge = halfedge;
    }

    void setGlobalCoordinate(const std::complex<int32_t>& global) {
        _global = {global};
    }

    auto getGlobalCoordinate() const -> const std::optional<std::complex<int32_t>>& {
        return _global;
    }

    auto getDetail() const -> const std::optional<std::complex<int32_t>>& {
        return _detail;
    }

    void setDetail(const std::optional<std::complex<int32_t>>& detail) {
        _detail = detail;
    }

    void print(std::ostream& outputStream) const {
        const std::complex<double> defaultCoord{-1., -1.};
        outputStream << " global " << getGlobalCoordinate().value_or(defaultCoord) << " detail "
                     << getDetail().value_or(defaultCoord);
    }

    auto id() const -> int32_t {
        return _id;
    }

    void setId(const int32_t vertexId) {
        _id = vertexId;
    }

  private:
    Halfedge* _halfedge = nullptr;
    mutable int32_t _visit = -1;

    Type _type = UNDEFINED;
    std::optional<std::complex<int32_t>> _global;
    std::optional<std::complex<int32_t>> _detail;

    int32_t _id = 0;
};

class GeomHelper {
  public:
    template <typename T>
    static void getNearest(const Vertex& handle, const Vertex*& nearestVertex,
                           RangeIterator<T> range) {
        for (auto ccb : range) {
            for (auto halfedge : RangeHelper::make(ccb)) {
                const Vertex& targetVertex = *halfedge.target();

                if (nearestVertex == nullptr ||
                    CGAL::has_smaller_distance_to_point(handle.point(), targetVertex.point(),
                                                        nearestVertex->point())) {
                    nearestVertex = &targetVertex;
                }
            }
        }
    }

    static auto getNearestVertex(const Face& face, const Vertex& handle) -> const Vertex& {
        const Vertex* nearestVertex = nullptr;

        getNearest(handle, nearestVertex,
                   RangeHelper::make(face.inner_ccbs_begin(), face.inner_ccbs_end()));

        getNearest(handle, nearestVertex,
                   RangeHelper::make(face.outer_ccbs_begin(), face.outer_ccbs_end()));
        return *nearestVertex;
    }

    template <typename T>
    static void getNearest(const Vertex& handle, Vertex*& nearestVertex, RangeIterator<T> range) {
        for (auto ccb : range) {
            for (auto halfedge : RangeHelper::make(ccb)) {
                Vertex& targetVertex = *halfedge.target();

                if (nearestVertex == nullptr ||
                    CGAL::has_smaller_distance_to_point(handle.point(), targetVertex.point(),
                                                        nearestVertex->point())) {
                    nearestVertex = &targetVertex;
                }
            }
        }
    }

    static auto getNearestVertex(Face& face, const Vertex& handle) -> Vertex& {
        Vertex* nearestVertex = nullptr;

        getNearest(handle, nearestVertex,
                   RangeHelper::make(face.inner_ccbs_begin(), face.inner_ccbs_end()));

        getNearest(handle, nearestVertex,
                   RangeHelper::make(face.outer_ccbs_begin(), face.outer_ccbs_end()));
        return *nearestVertex;
    }
};

class GlobalEdge {
  public:
    struct Endpoints {
        std::complex<int32_t> start;
        std::complex<int32_t> end;
    };

    explicit GlobalEdge(Endpoints endpoints) : _a{endpoints.start}, _b{endpoints.end} {}

    void print(std::ostream& outputStream) const {
        outputStream << " a " << _a << " b " << _b;
    }

    [[nodiscard]] auto a() const -> const std::complex<int32_t>& {
        return _a;
    }

    [[nodiscard]] auto b() const -> const std::complex<int32_t>& {
        return _b;
    }

  private:
    std::complex<int32_t> _a;
    std::complex<int32_t> _b;
};

} /* namespace laby */

#endif /* GEOMDATA_H_ */

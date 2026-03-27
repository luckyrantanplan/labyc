/*
 * GeomData.h
 *
 *  Created on: Feb 1, 2018
 *      Author: florian
 */

#ifndef GEOMDATA_H_
#define GEOMDATA_H_

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <CGAL/Arr_curve_data_traits_2.h>
#include <CGAL/Arr_extended_dcel.h>
#include <CGAL/Arr_segment_traits_2.h>
#include <CGAL/Arr_trapezoid_ric_point_location.h>
#include <CGAL/Arrangement_2/Arrangement_2_iterators.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/Exact_predicates_exact_constructions_kernel.h>
#include <CGAL/Iterator_transform.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Polygon_2.h>
#include <complex>
#include <iostream>
#include <iterator>
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
#include "basic/RangeHelper.h"

namespace laby {

// handy for printing datas
template <class T> auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
    t.print(os);
    return os;
}

template <typename T> std::ostream& operator<<(std::ostream& out, const std::vector<T>& v) {
    if (!v.empty()) {
        out << '[';
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
        out << "END]";
    }
    else {
        out << "[EMPTY]";
    }
    return out;
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
    enum Type : int8_t { UNDEFINED = 1, HORIZONTAL = 2, VERTICAL = 3, DIAGONAL = 4, SPECIAL = 5, CELL = 6 };

    struct Coordinate {
        std::size_t value;
    };

    EdgeInfo(int32_t direction, Coordinate coordinate) : _direction{direction}, _coordinate{coordinate.value} {}

    explicit EdgeInfo(Type itype) : _direction{itype} {}

    EdgeInfo() = default;

    int32_t direction() const { return _direction; }

    int32_t congestion() const { return static_cast<int32_t>(_path.size()); }

    bool hasNet(const int32_t serialId) const { return _path.count(serialId) != 0; }

    void addPath(const int32_t serialId) {
        auto it = _path.emplace(serialId, 1);
        if (!it.second) {
            ++it.first->second;
        }
    }

    void clearAllPath() { _path.clear(); }

    int32_t getVisit() const { return _visit; }

    void setVisit(int32_t value = -1) const { _visit = value; }

    bool operator==(const EdgeInfo& it) const { return _direction == it._direction; }

    double thickness() const { return _thickness; }

    void setThickness(double thickness = 1) { _thickness = thickness; }

    const Halfedge* getNextHalfedge(int32_t visited, const Vertex& v) const;

    void print(std::ostream& os) const {
        os << " path ";
        for (const auto& [net_id, count] : _path) {
            os << " " << net_id << "->" << count;
        }
    }

    int32_t getNet() const { return _path.begin()->first; }

    std::size_t coordinate() const { return _coordinate; }

private:
    // TODO : replace unordered_map by just an int
    std::unordered_map<int32_t, int32_t> _path;
    double _thickness = 1;
    mutable int32_t _visit = -1;
    int32_t _direction = -1;
    std::size_t _coordinate = 0;
};

class VertexInfo {
public:
    enum Type : int8_t { UNDEFINED = 0, STEINER = 1, PIN = 2 };

    VertexInfo() = default;

    Type getType() const { return _type; }

    void setType(Type type) { _type = type; }

    int32_t getVisit() const { return _visit; }

    void setVisit(int32_t ivisit, Halfedge* he) {
        _visit = ivisit;
        _halfedge = he;
    }

    void setGlobalCoordinate(const std::complex<int32_t>& global) { _global = {global}; }

    const std::optional<std::complex<int32_t>>& getGlobalCoordinate() const { return _global; }

    const std::optional<std::complex<int32_t>>& getDetail() const { return _detail; }

    void setDetail(const std::optional<std::complex<int32_t>>& detail) { _detail = detail; }

    void print(std::ostream& os) const {
        const std::complex<double> defaultCoord{-1., -1.};
        os << " global " << getGlobalCoordinate().value_or(defaultCoord) << " detail " << getDetail().value_or(defaultCoord);
    }

    int32_t id() const { return _id; }

    void setId(const int32_t id) { _id = id; }

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
    template <typename T> static void getNearest(const Vertex& handle, const Vertex*& vertMin, RangeIterator<T> range) {

        for (auto ccb : range) {

            for (auto he : RangeHelper::make(ccb)) {

                const Vertex& v = *he.target();

                if (vertMin == nullptr || CGAL::has_smaller_distance_to_point(handle.point(), v.point(), vertMin->point())) {
                    vertMin = &v;
                }
            }
        }
    }

    static const Vertex& getNearestVertex(const Face& fh, const Vertex& handle) {
        const Vertex* vertMin = nullptr;

        getNearest(handle, vertMin, RangeHelper::make(fh.inner_ccbs_begin(), fh.inner_ccbs_end()));

        getNearest(handle, vertMin, RangeHelper::make(fh.outer_ccbs_begin(), fh.outer_ccbs_end()));
        return *vertMin;
    }

    template <typename T> static void getNearest(const Vertex& handle, Vertex*& vertMin, RangeIterator<T> range) {

        for (auto ccb : range) {

            for (auto he : RangeHelper::make(ccb)) {

                Vertex& v = *he.target();

                if (vertMin == nullptr || CGAL::has_smaller_distance_to_point(handle.point(), v.point(), vertMin->point())) {
                    vertMin = &v;
                }
            }
        }
    }

    static Vertex& getNearestVertex(Face& fh, const Vertex& handle) {
        Vertex* vertMin = nullptr;

        getNearest(handle, vertMin, RangeHelper::make(fh.inner_ccbs_begin(), fh.inner_ccbs_end()));

        getNearest(handle, vertMin, RangeHelper::make(fh.outer_ccbs_begin(), fh.outer_ccbs_end()));
        return *vertMin;
    }
};

class GlobalEdge {
public:
    struct Endpoints {
        std::complex<int32_t> start;
        std::complex<int32_t> end;
    };

    explicit GlobalEdge(Endpoints endpoints) : _a{endpoints.start}, _b{endpoints.end} {}

    void print(std::ostream& os) const { os << " a " << _a << " b " << _b; }

    [[nodiscard]] const std::complex<int32_t>& a() const { return _a; }

    [[nodiscard]] const std::complex<int32_t>& b() const { return _b; }

private:
    std::complex<int32_t> _a;
    std::complex<int32_t> _b;
};

} /* namespace laby */

#endif /* GEOMDATA_H_ */

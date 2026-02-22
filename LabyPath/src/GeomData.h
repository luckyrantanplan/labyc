/*
 * GeomData.h
 *
 *  Created on: Feb 1, 2018
 *      Author: florian
 */

#ifndef GEOMDATA_H_
#define GEOMDATA_H_

#include <bits/stdint-intn.h>
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
#include <optional>
#include <unordered_map>
#include <utility>
#include <vector>
#include "basic/RangeHelper.h"

namespace laby {

//handy for printing datas
template<class T>
auto operator<<(std::ostream& os, const T& t) -> decltype(t.print(os), os) {
    t.print(os);
    return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& out, const std::vector<T>& v) {
    if (!v.empty()) {
        out << '[';
        std::copy(v.begin(), v.end(), std::ostream_iterator<T>(out, ", "));
        out << "END]";
    } else {
        out << "[EMPTY]";
    }
    return out;
}

class EdgeInfo;

class VertexInfo;

class Dummy {

};
// //typedef CGAL::Cartesian<CGAL::Exact_rational> Kernel;
typedef CGAL::Exact_predicates_exact_constructions_kernel Kernel;

typedef CGAL::Arr_segment_traits_2<Kernel> Segment_traits_2;
typedef Segment_traits_2::Curve_2 Segment_2;
typedef CGAL::Arr_curve_data_traits_2<Segment_traits_2, EdgeInfo> Traits_2;
typedef Traits_2::Point_2 Point_2;
typedef Traits_2::X_monotone_curve_2 Segment_info_2;

typedef CGAL::Arr_extended_dcel<Traits_2, VertexInfo, Dummy, Dummy> Dcel;

typedef CGAL::Arrangement_2<Traits_2, Dcel> Arrangement_2;

typedef Arrangement_2::Vertex_handle Vertex_handle;
typedef Vertex_handle::value_type Vertex;

typedef Arrangement_2::Vertex_const_handle Vertex_const_handle;
typedef Arrangement_2::Halfedge_handle Halfedge_handle;

typedef Halfedge_handle::value_type Halfedge;

typedef Arrangement_2::Halfedge_handle Halfedge_handle;
typedef Arrangement_2::Halfedge_handle Halfedge_const_handle;
typedef Arrangement_2::Face_handle Face_handle;
typedef Arrangement_2::Face_const_handle Face_const_handle;
typedef Face_handle::value_type Face;

typedef Arrangement_2::Edge_iterator Edge_iterator;
typedef Arrangement_2::Edge_const_iterator Edge_const_iterator;

typedef Arrangement_2::Edge_const_iterator::value_type Edge_const;
typedef CGAL::Arr_trapezoid_ric_point_location<Arrangement_2> Walk_pl;
typedef std::vector<Halfedge*> PolySegment;

class EdgeInfo {
public:
    enum Type
        : int8_t {
            UNDEFINED = 1, HORIZONTAL = 2, VERTICAL = 3, DIAGONAL = 4, SPECIAL = 5, CELL = 6
    };
    EdgeInfo(const int32_t& idirection, const std::size_t& icoordinate) :
            _direction { idirection }, _coordinate { icoordinate } {
    }

    explicit EdgeInfo(const Type& itype) :
            _direction { itype } {
    }

    EdgeInfo() {
    }

    int32_t direction() const {
        return _direction;
    }

    int32_t congestion() const {
        return path.size();
    }

    bool hasNet(const int serialId) const {
        return path.count(serialId) != 0;
    }

    void addPath(const int serialId) {
        std::pair<std::unordered_map<int, int>::iterator, bool> it = path.emplace(serialId, 1);
        if (!it.second) {
            ++it.first->second;
        }
    }

    void clearAllPath() {
        path.clear();
    }

    int32_t getVisit() const {
        return visit;
    }

    void setVisit(int32_t visit = -1) const {
        this->visit = visit;
    }

    bool operator ==(const EdgeInfo& it) const {
        return _direction == it._direction;
    }

    double thickness() const {
        return _thickness;
    }

    void setThickness(double thickness = 1) {
        _thickness = thickness;
    }

    const Halfedge* getNextHalfedge(int32_t visited, const Vertex& v) const;

    void print(std::ostream& os) const {
        os << " path ";
        for (auto& pair : path) {
            os << " " << pair.first << "->" << pair.second;
        }
    }

    int32_t get_net() const {
        return path.begin()->first;
    }

    std::size_t coordinate() const {
        return _coordinate;
    }

private:

    //TODO : replace unordered_map by just an int
    std::unordered_map<int, int> path;
    double _thickness = 1;
    mutable int32_t visit = -1;
    int32_t _direction = -1;
    std::size_t _coordinate = 0;

};

class VertexInfo {
public:
    enum Type
        : int8_t {
            UNDEFINED = 0, STEINER = 1, PIN = 2
    };

    VertexInfo() :
            _heh { nullptr }, visit(-1), type { UNDEFINED }, _global { }, _detail { } {
    }

    Type getType() const {
        return type;
    }

    void setType(const Type& iType) {
        type = iType;
    }

    int getVisit() const {
        return visit;
    }

    void setVisit(int ivisit, Halfedge* he) {
        visit = ivisit;
        _heh = he;
    }

    Halfedge* _heh;

    void setGlobalCoordinate(const std::complex<int32_t> & global) {
        _global = {global};
    }

    const std::optional<std::complex<int32_t>>& getGlobalCoordinate() const {
        return _global;
    }

    const std::optional<std::complex<int32_t> >& getDetail() const {
        return _detail;
    }

    void setDetail(const std::optional<std::complex<int32_t> >& detail) {
        _detail = detail;
    }

    void print (std::ostream& os ) const {
        std::complex<double> nunu {-1.,-1.};
        os << " global " << getGlobalCoordinate().value_or(nunu) << " detail " << getDetail().value_or(nunu );

    }

    int32_t id() const {
        return _id;
    }

    void setId(const int32_t id) {
        _id = id;
    }

private:
    mutable int visit;

    Type type;
    std::optional<std::complex<int32_t>> _global;
    std::optional<std::complex<int32_t>> _detail;

    int32_t _id=0;
};

class GeomHelper {
public:

    template<typename T>
    static void getNearest(const Vertex& handle, const Vertex*& vertMin, RangeIterator<T> && range) {

        for (auto ccb : range) {

            for (auto he : RangeHelper::make(ccb)) {

                const Vertex & v = *he.target();

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

};

struct GlobalEdge {

    GlobalEdge(int32_t ax, int32_t ay, int32_t bx, int32_t by) :
            _a { ax, ay }, _b { bx, by } {
    }

    void print(std::ostream& os) const {
        os << " a " << _a << " b " << _b;

    }

    std::complex<int32_t> _a;
    std::complex<int32_t> _b;
};

} /* namespace laby */

#endif /* GEOMDATA_H_ */

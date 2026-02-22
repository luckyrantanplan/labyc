/*
 * PolyConvex.h
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#ifndef POLYCONVEX_H_
#define POLYCONVEX_H_

#include <bits/move.h>
#include <bits/stdint-intn.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/Union_find.h>
#include <algorithm>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>

#include "basic/PolygonTools.h"
#include "flatteningOverlap/Node.h"
#include "GeomData.h"

namespace laby {
namespace basic {
class LinearGradient;
} /* namespace basic */
} /* namespace laby */

namespace laby {

class PolyConvex {
public:
    Linear_polygon _geometry;
    Linear_polygon _originalTrapeze;
    std::vector<std::size_t> _adjacents;
    mutable std::vector<Node*> _nodes;
    Halfedge* _supportHe = nullptr;
    std::size_t _id = 0;
    mutable int32_t _visited = 0;
    mutable CGAL::Union_find<std::size_t>::handle handle;
    double _average_thickness = 0;

    void set_average_thickness(double thickness) {
        _average_thickness = thickness;
    }

    double thickness() const {
        return _average_thickness;
    }
    PolyConvex() {

    }

    void remove_adjacence(std::size_t i) {
        auto ite = std::find(_adjacents.begin(), _adjacents.end(), i);
        std::swap(*ite, _adjacents.back());
        _adjacents.resize(_adjacents.size() - 1u);
    }

    void resetMutable() const {
        handle = CGAL::Union_find<std::size_t>::handle();

        _visited = 0;
        _nodes.clear();
    }

    void clear() {
        _supportHe = nullptr;
        _geometry.clear();
        _id = 0;
        _average_thickness = 0;
    }

    bool empty() {
        return _geometry.is_empty();
    }

    PolyConvex(Halfedge& he, std::size_t id, basic::LinearGradient& lgrad);
    PolyConvex(const Point_2& ps, const Point_2& pt, std::size_t id, basic::LinearGradient& lgrad);
    PolyConvex(const Point_2& ps, const Point_2& pt, std::size_t id, const Linear_polygon& geometry);

    void print(std::ostream& os) const {

        os << " _geometry " << _geometry;
        os << " _adjacents " << _adjacents;
        os << " ps " << _ps;
        os << " pt " << _pt;
        os << " id " << _id;
    }

    bool contains(const Node& n) const {
        return (std::find(_nodes.begin(), _nodes.end(), &n) != _nodes.end());
    }

    static bool testConvexPolyIntersect(const Linear_polygon & a, const Linear_polygon& b);
    static void connect(std::size_t first, std::size_t second, std::vector<PolyConvex>& polyConvexList, const Point_2& center);
    static void connect(std::size_t begin, std::vector<PolyConvex>& polyConvexList);
    static void connect(std::size_t first, std::size_t second, std::vector<PolyConvex>& polyConvexList);

    const CGAL::Point_2<Kernel>& getSourcePoint() const {
        if (_supportHe != nullptr) {
            return _supportHe->source()->point();
        }
        return _ps;
    }

    const CGAL::Point_2<Kernel>& getTargetPoint() const {
        if (_supportHe != nullptr) {
            return _supportHe->target()->point();
        }
        return _pt;
    }

    const bool has_points() const {
        if (_supportHe != nullptr) {
            return true;
        }
        return _has_points;
    }

private:
    void init(const Point_2& ps, const Point_2& pt, basic::LinearGradient& lgrad);

    // needed for connect_maze (since we do not have the original arrangement for support : perharps we should split PolyConvex Class ?

    CGAL::Point_2<Kernel> _ps;
    CGAL::Point_2<Kernel> _pt;
    bool _has_points = false;
}
;

} /* namespace laby */

#endif /* POLYCONVEX_H_ */

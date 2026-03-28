/*
 * PolyConvex.h
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#ifndef POLYCONVEX_H_
#define POLYCONVEX_H_

#include <utility>
#include <cstdint>
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

namespace laby::basic {
class LinearGradient;
} /* namespace laby::basic */

namespace laby {

/**
 * @brief A convex polygon tile in the routing decomposition.
 *
 * Each PolyConvex represents a trapezoid (or triangle) tile created by
 * the routing stage.  Key fields:
 *
 * - **_geometry**: The actual convex polygon boundary (Linear_polygon).
 * - **_originalTrapeze**: Copy of the original trapeze before any extension.
 * - **_adjacents**: Indices of adjacent PolyConvex objects in the same vector.
 * - **_nodes**: Pointers to Node objects (from flatteningOverlap) that cover
 *   this polygon. Declared `mutable` because the overlap resolution traversal
 *   writes to it on const objects passed through CGAL callbacks.
 * - **_visited**: Mutable BFS traversal flag (0=unvisited, -1=marked).
 * - **handle**: CGAL Union_find handle for connectivity grouping.
 * - **_supportHe**: Optional pointer to the arrangement halfedge that
 *   generated this polygon (nullptr for maze-connected polygons).
 */
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

    void setAverageThickness(double thickness) { 
        _average_thickness = thickness;
    }

    auto thickness() const -> double { return _average_thickness; }
    PolyConvex() = default;

    void removeAdjacence(std::size_t i) { 
        auto ite = std::find(_adjacents.begin(), _adjacents.end(), i);
        std::swap(*ite, _adjacents.back());
        _adjacents.resize(_adjacents.size() - 1U);
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

    auto empty() const -> bool { return _geometry.is_empty(); }

    PolyConvex(Halfedge& he, std::size_t id, basic::LinearGradient& lgrad);
    PolyConvex(const Point_2& ps, const Point_2& pt, std::size_t id, basic::LinearGradient& lgrad);
    PolyConvex(const Point_2& ps, const Point_2& pt, std::size_t id, Linear_polygon  geometry);

    void print(std::ostream& os) const {

        os << " _geometry " << _geometry;
        os << " _adjacents " << _adjacents;
        os << " ps " << _ps;
        os << " pt " << _pt;
        os << " id " << _id;
    }

    auto contains(const Node& n) const -> bool { return (std::find(_nodes.begin(), _nodes.end(), &n) != _nodes.end()); }

    static auto testConvexPolyIntersect(const Linear_polygon& a, const Linear_polygon& b) -> bool;
    static void connect(std::size_t first, std::size_t second, std::vector<PolyConvex>& polyConvexList, const Point_2& center);
    static void connect(std::size_t begin, std::vector<PolyConvex>& polyConvexList);
    static void connect(std::size_t first, std::size_t second, std::vector<PolyConvex>& polyConvexList);

    auto getSourcePoint() const -> const CGAL::Point_2<Kernel>& {
        if (_supportHe != nullptr) {
            return _supportHe->source()->point();
        }
        return _ps;
    }

    auto getTargetPoint() const -> const CGAL::Point_2<Kernel>& {
        if (_supportHe != nullptr) {
            return _supportHe->target()->point();
        }
        return _pt;
    }

    auto hasPoints() const -> bool { 
        if (_supportHe != nullptr) {
            return true;
        }
        return _has_points;
    }

private:
    void init(const Point_2& ps, const Point_2& pt, basic::LinearGradient& lgrad);

    // Needed for connect_maze (since we do not have the original arrangement for support; perhaps we should split PolyConvex class)

    CGAL::Point_2<Kernel> _ps;
    CGAL::Point_2<Kernel> _pt;
    bool _has_points = false;
};

} /* namespace laby */

#endif /* POLYCONVEX_H_ */

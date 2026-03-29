/*
 * PolyConvex.h
 *
 *  Created on: Mar 15, 2018
 *      Author: florian
 */

#ifndef POLYCONVEX_H_
#define POLYCONVEX_H_

#include <CGAL/Polygon_2.h>
#include <CGAL/Union_find.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

#include "GeomData.h"
#include "basic/PolygonTools.h"
#include "flatteningOverlap/Node.h"

namespace laby::basic {
class LinearGradient;
} /* namespace laby::basic */

namespace laby {

struct PolyConvexEndpoints {
    Point_2 sourcePoint;
    Point_2 targetPoint;
};

struct PolyConvexConnection {
    std::size_t firstPolyConvexId;
    std::size_t secondPolyConvexId;
};

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
struct PolyConvex {
    Linear_polygon _geometry;
    Linear_polygon _originalTrapeze;
    std::vector<std::size_t> _adjacents;
    mutable std::vector<Node*> _nodes;
    Halfedge* _supportHe = nullptr;
    std::size_t _id = 0;
    mutable int32_t _visited = 0;
    mutable CGAL::Union_find<std::size_t>::handle handle;
    double _average_thickness = 0.0;

    auto setAverageThickness(double thickness) -> void {
        _average_thickness = thickness;
    }

    [[nodiscard]] auto thickness() const -> double {
        return _average_thickness;
    }
    PolyConvex() = default;

    auto removeAdjacence(std::size_t adjacencyId) -> void {
        auto iterator = std::find(_adjacents.begin(), _adjacents.end(), adjacencyId);
        if (iterator == _adjacents.end()) {
            return;
        }
        std::swap(*iterator, _adjacents.back());
        _adjacents.resize(_adjacents.size() - 1U);
    }

    auto resetMutable() const -> void {
        handle = CGAL::Union_find<std::size_t>::handle();

        _visited = 0;
        _nodes.clear();
    }

    auto clear() -> void {
        _supportHe = nullptr;
        _geometry.clear();
        _id = 0;
        _average_thickness = 0.0;
    }

    [[nodiscard]] auto empty() const -> bool {
        return _geometry.is_empty();
    }

    PolyConvex(Halfedge& halfedge, std::size_t polygonId, basic::LinearGradient& gradient);
    PolyConvex(const PolyConvexEndpoints& endpoints, std::size_t polygonId,
               basic::LinearGradient& gradient);
    PolyConvex(PolyConvexEndpoints endpoints, std::size_t polygonId, Linear_polygon geometry);

    auto print(std::ostream& outputStream) const -> void {
        outputStream << " _geometry " << _geometry;
        outputStream << " _adjacents " << _adjacents;
        outputStream << " source " << _sourcePoint;
        outputStream << " target " << _targetPoint;
        outputStream << " id " << _id;
    }

    [[nodiscard]] auto contains(const Node& node) const -> bool {
        return std::find(_nodes.begin(), _nodes.end(), &node) != _nodes.end();
    }

    static auto testConvexPolyIntersect(const Linear_polygon& polygon,
                                        const Linear_polygon& otherPolygon) -> bool;
    static auto connect(const PolyConvexConnection& connection,
                        std::vector<PolyConvex>& polyConvexList, const Point_2& center) -> void;
    static auto connect(std::size_t beginIndex, std::vector<PolyConvex>& polyConvexList) -> void;
    static auto connect(const PolyConvexConnection& connection,
                        std::vector<PolyConvex>& polyConvexList) -> void;

    [[nodiscard]] auto getSourcePoint() const -> const CGAL::Point_2<Kernel>& {
        if (_supportHe != nullptr) {
            return _supportHe->source()->point();
        }
        return _sourcePoint;
    }

    [[nodiscard]] auto getTargetPoint() const -> const CGAL::Point_2<Kernel>& {
        if (_supportHe != nullptr) {
            return _supportHe->target()->point();
        }
        return _targetPoint;
    }

    [[nodiscard]] auto hasPoints() const -> bool {
        if (_supportHe != nullptr) {
            return true;
        }
        return _has_points;
    }

  private:
    auto init(const PolyConvexEndpoints& endpoints, basic::LinearGradient& gradient) -> void;

    // Needed for connect_maze (since we do not have the original arrangement for support; perhaps
    // we should split PolyConvex class)

    CGAL::Point_2<Kernel> _sourcePoint;
    CGAL::Point_2<Kernel> _targetPoint;
    bool _has_points = false;
};

} /* namespace laby */

#endif /* POLYCONVEX_H_ */

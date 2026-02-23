/*
 * PathRendering.h
 *
 *  Created on: Mar 8, 2018
 *      Author: florian
 */

#ifndef PATHRENDERING_H_
#define PATHRENDERING_H_

#include <CGAL/Box_intersection_d/Box_d.h>
#include <CGAL/Box_intersection_d/Box_with_handle_d.h>
#include <CGAL/Union_find.h>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "../basic/AugmentedPolygonSet.h"
#include "../GeomData.h"
#include "../PolyConvex.h"
#include "../OrientedRibbon.h"
#include "Family.h"
#include "Node.h"

namespace laby {

class Node;

struct QueueNodePolyConvex {

    QueueNodePolyConvex(Node& n, const PolyConvex& pc) : _n{n}, _pc{pc} {}

    Node& _n;
    const PolyConvex& _pc;
};

class PathRendering {
public:
    static void pathRender(const std::vector<PolyConvex>& polyConvexList, OrientedRibbon& oRibbon);

private:
    void createIntersect(OrientedRibbon& oribbon, const std::vector<PolyConvex>& polyConvexList);
    void createUnion(OrientedRibbon& ribs, const std::vector<PolyConvex>& polyConvexList);
    void reCutAllGeometry(const std::vector<const Family*>& families, const std::vector<PolyConvex>& polyConvexList,
                          const std::unordered_map<std::size_t, Node*>& map);

    void createPolygonSet(const std::vector<PolyConvex>& polyConvexList, const std::vector<std::size_t>& cover, basic::Polygon_set_2Node& setPolygons);

    typedef CGAL::Box_intersection_d::Box_with_handle_d<double, 2, std::size_t, CGAL::Box_intersection_d::ID_EXPLICIT> BoxIntersection;
    void unify(std::size_t second, const std::vector<std::size_t>& aAdjs, std::unordered_set<Intersection>& intersections, CGAL::Union_find<size_t>& uf,
               const Intersection& i);
    std::vector<Node> processFamilies(std::unordered_map<size_t, std::size_t>& families, const std::vector<PolyConvex>& polyConvexList,
                                      std::vector<Family>& familyVector, std::vector<Intersection>& intersections,
                                      std::vector<BoxIntersection>& box_intersection, CGAL::Union_find<std::size_t>& uf);
    void mergeFamilies(const std::vector<const Family*>& families, std::unordered_set<Intersection>& intersectOnSinglePiece,
                       const std::vector<PolyConvex>& polyConvexList, std::vector<Node>& nodes);
    void nodeAdjacence(std::vector<Node>& nodes, const std::vector<PolyConvex>& polyConvexList);
    void chooseNodeState(std::vector<Node>& nodes);

    std::unordered_map<std::size_t, std::vector<const Family*>>
    locateFamilies(const std::unordered_map<std::size_t, std::size_t>& families, std::vector<Family>& familyVector, std::vector<Intersection>& intersections,
                   std::vector<BoxIntersection>& box_intersection, const CGAL::Union_find<std::size_t>& uf, const std::vector<PolyConvex>& polyConvexList);
    std::vector<Node> createNode(const std::unordered_map<std::size_t, std::vector<const Family*>>& map,
                                 std::unordered_set<Intersection> intersectOnSinglePiece, const std::vector<PolyConvex>& polyConvexList);

    bool do_intersect(Intersection& ai, Intersection& bi, const std::vector<PolyConvex>& polyConvexList);
};

} /* namespace laby */

#endif /* PATHRENDERING_H_ */

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

#include "../GeomData.h"
#include "../OrientedRibbon.h"
#include "../PolyConvex.h"
#include "../basic/AugmentedPolygonSet.h"
#include "Family.h"
#include "Node.h"

namespace laby {

class Node;

struct QueueNodePolyConvex {

    QueueNodePolyConvex(Node& node, const PolyConvex& polyConvex)
        : _node{&node}, _polyConvex{&polyConvex} {}

    [[nodiscard]] auto node() const -> Node& {
        return *_node;
    }
    [[nodiscard]] auto polyConvex() const -> const PolyConvex& {
        return *_polyConvex;
    }

  private:
    Node* _node;
    const PolyConvex* _polyConvex;
};

class PathRendering {
  public:
    static void pathRender(const std::vector<PolyConvex>& polyConvexList, OrientedRibbon& oRibbon);

  private:
    struct PolygonSetData {
        const std::vector<PolyConvex>& polyConvexList;
        const std::vector<std::size_t>& cover;
    };

    void createIntersect(OrientedRibbon& oribbon, const std::vector<PolyConvex>& polyConvexList);
    static void createUnion(OrientedRibbon& ribs, const std::vector<PolyConvex>& polyConvexList);
    struct ReCutGeometryData {
        const std::vector<PolyConvex>& polyConvexList;
        const std::unordered_map<std::size_t, Node*>& map;
    };

    static void reCutAllGeometry(const std::vector<const Family*>& families,
                                 const ReCutGeometryData& reCutGeometryData);

    static void createPolygonSet(const PolygonSetData& polygonSetData,
                                 basic::Polygon_set_2Node& setPolygons);

    using BoxIntersection =
        CGAL::Box_intersection_d::Box_with_handle_d<double, 2, std::size_t,
                                                    CGAL::Box_intersection_d::ID_EXPLICIT>;
    struct UnifyData {
        std::size_t secondIndex;
        const std::vector<std::size_t>& adjacentIndices;
    };

    static void unify(const UnifyData& unifyData, std::unordered_set<Intersection>& intersections,
                      CGAL::Union_find<size_t>& unionFind, const Intersection& intersection);
    struct FamilyProcessingData {
        const std::vector<PolyConvex>& polyConvexList;
        std::vector<Family>& familyVector;
        std::vector<Intersection>& intersections;
        std::vector<BoxIntersection>& boxIntersectionList;
        CGAL::Union_find<std::size_t>& unionFind;
    };

    auto processFamilies(std::unordered_map<size_t, std::size_t>& families,
                         FamilyProcessingData& familyProcessingData) -> std::vector<Node>;
    void mergeFamilies(const std::vector<const Family*>& families,
                       std::unordered_set<Intersection>& intersectOnSinglePiece,
                       const std::vector<PolyConvex>& polyConvexList, std::vector<Node>& nodes);
    static void nodeAdjacence(std::vector<Node>& nodes,
                              const std::vector<PolyConvex>& polyConvexList);
    static void chooseNodeState(std::vector<Node>& nodes);

    struct FamilyLocationData {
        std::vector<Family>& familyVector;
        std::vector<Intersection>& intersections;
        std::vector<BoxIntersection>& boxIntersectionList;
        const CGAL::Union_find<std::size_t>& unionFind;
        const std::vector<PolyConvex>& polyConvexList;
    };

    static auto locateFamilies(const std::unordered_map<std::size_t, std::size_t>& families,
                               FamilyLocationData& familyLocationData)
        -> std::unordered_map<std::size_t, std::vector<const Family*>>;
    struct NodeCreationData {
        const std::unordered_map<std::size_t, std::vector<const Family*>>& familyMap;
        std::unordered_set<Intersection>& intersectOnSinglePiece;
        const std::vector<PolyConvex>& polyConvexList;
    };

    auto createNode(const NodeCreationData& nodeCreationData) -> std::vector<Node>;

    static auto doIntersect(Intersection& firstIntersection, Intersection& secondIntersection,
                            const std::vector<PolyConvex>& polyConvexList) -> bool;

    int32_t _nextNodeId =
        0; ///< Counter for assigning unique node IDs (replaces former static variable).
};

} /* namespace laby */

#endif /* PATHRENDERING_H_ */

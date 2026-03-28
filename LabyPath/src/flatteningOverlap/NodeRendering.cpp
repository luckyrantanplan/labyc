/*
 * NodeRendering.cpp
 *
 *  Created on: Mar 13, 2018
 *      Author: florian
 */

#include "NodeRendering.h"

#include <CGAL/Arr_overlay_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Iterator_transform.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include <vector>

#include "OrientedRibbon.h"
#include "GeomData.h"
#include "PolyConvex.h"
#include "basic/AugmentedPolygonSet.h"
#include "basic/EasyProfilerCompat.h"
#include "basic/RangeHelper.h"
#include "flatteningOverlap/Node.h"

namespace laby {

namespace {

auto edgeMatchesPolygonId(const int32_t polygonIndex, const basic::HalfedgeNode& halfedge) -> bool {
    return basic::edgeHasPolygonId(halfedge, polygonIndex);
}

auto arrangementHasSharedFace(basic::Arrangement_2Node& arrangement) -> bool {
    using namespace basic;

    for (FaceNode& face : RangeHelper::make(arrangement.faces_begin(), arrangement.faces_end())) {
        if (face.data().size() > 1) {
            return true;
        }
    }
    return false;
}

auto buildOverlayArrangement(const std::vector<Node*>& nodes) -> basic::Arrangement_2Node {
    basic::Arrangement_2Node overlayArrangement = nodes.front()->_setPolygons.arrangement();
    basic::Overlay_traitsNode overlayTraits;

    for (std::size_t nodeIndex = 1; nodeIndex < nodes.size(); ++nodeIndex) {
        const basic::Arrangement_2Node previousArrangement = overlayArrangement;
        CGAL::overlay(previousArrangement, nodes.at(nodeIndex)->_setPolygons.arrangement(),
                      overlayArrangement, overlayTraits);
    }

    return overlayArrangement;
}

auto renderSharedEdges(OrientedRibbon& orientedRibbon,
                       basic::Arrangement_2Node& arrangement) -> void {
    using namespace basic;

    for (HalfedgeNode& halfedge :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        const auto& faceData = halfedge.face()->data();
        const auto& twinFaceData = halfedge.twin()->face()->data();
        if (faceData.size() + twinFaceData.size() > 1) {
            orientedRibbon.addCCW(
                Kernel::Segment_2(halfedge.source()->point(), halfedge.target()->point()));
        }
    }
}

auto addOuterBoundarySegments(OrientedRibbon& orientedRibbon, const int32_t polygonIndex,
                              basic::FaceNode& face) -> void {
    using namespace basic;

    for (HalfedgeNode& halfedge : RangeHelper::make(face.outer_ccb())) {
        if (edgeMatchesPolygonId(polygonIndex, halfedge)) {
            orientedRibbon.addCCW(
                Kernel::Segment_2(halfedge.source()->point(), halfedge.target()->point()));
        }
    }
}

auto addHoleBoundarySegments(OrientedRibbon& orientedRibbon, const int32_t polygonIndex,
                             basic::FaceNode& face) -> void {
    using namespace basic;

    for (Arrangement_2Node::Inner_ccb_iterator innerBoundary = face.holes_begin();
         innerBoundary != face.holes_end(); ++innerBoundary) {
        for (const HalfedgeNode& halfedge : RangeHelper::make(*innerBoundary)) {
            if (edgeMatchesPolygonId(polygonIndex, halfedge)) {
                orientedRibbon.addCW(
                    Kernel::Segment_2(halfedge.source()->point(), halfedge.target()->point()));
            }
        }
    }
}

auto renderOverlappingFaces(OrientedRibbon& orientedRibbon,
                            basic::Arrangement_2Node& arrangement) -> void {
    using namespace basic;

    for (FaceNode& face : RangeHelper::make(arrangement.faces_begin(), arrangement.faces_end())) {
        std::unordered_set<int32_t>& polygonIds = face.data();
        if (polygonIds.size() <= 1 || face.is_unbounded()) {
            continue;
        }

        const auto minPolygonIt = std::min_element(polygonIds.begin(), polygonIds.end());
        if (minPolygonIt == polygonIds.end()) {
            continue;
        }

        const int32_t polygonIndex = *minPolygonIt;
        addOuterBoundarySegments(orientedRibbon, polygonIndex, face);
        addHoleBoundarySegments(orientedRibbon, polygonIndex, face);
    }
}

} // namespace

auto NodeOverlap::addIdToPolygon(const std::vector<PolyConvex>& /*polyConvexList*/) -> void {
    EASY_FUNCTION();
    using namespace basic;
    int32_t polygonIndex = 0;

    for (Node* node : _nodes) {
        Arrangement_2Node& arrangement = node->_setPolygons.arrangement();
        for (FaceNode& face :
             RangeHelper::make(arrangement.faces_begin(), arrangement.faces_end())) {
            face.setPolygonId(polygonIndex);
        }
        polygonIndex++;
    }
}

auto NodeOverlap::render(OrientedRibbon& orientedRibbon,
                         const std::vector<PolyConvex>& polyConvexList) -> void {
    EASY_FUNCTION();

    sortNode();

    addIdToPolygon(polyConvexList);

    if (_nodes.empty()) {
        return;
    }

    basic::Arrangement_2Node overlayArrangement = buildOverlayArrangement(_nodes);
    if (!arrangementHasSharedFace(overlayArrangement)) {
        renderSharedEdges(orientedRibbon, overlayArrangement);
        return;
    }

    renderOverlappingFaces(orientedRibbon, overlayArrangement);
}

auto NodeOverlap::sortNode() -> void {
    std::sort(_nodes.begin(), _nodes.end(), [](const Node* leftNode, const Node* rightNode) {
        return leftNode->_state < rightNode->_state;
    });
}

auto NodeRendering::render(OrientedRibbon& orientedRibbon, std::vector<Node>& nodes,
                           const std::vector<PolyConvex>& polyConvexList) -> void {
    EASY_FUNCTION();

    for (Node& node : nodes) {
        if (node._visited != 1) {

            node._visited = 1;
            NodeOverlap nodeOverlap;
            nodeOverlap._nodes.push_back(&node);
            for (Node* oppositeNode : node._opposite) {
                oppositeNode->_visited = 1;
                nodeOverlap._nodes.push_back(oppositeNode);
            }

            nodeOverlap.render(orientedRibbon, polyConvexList);
        }
    }
}

} /* namespace laby */

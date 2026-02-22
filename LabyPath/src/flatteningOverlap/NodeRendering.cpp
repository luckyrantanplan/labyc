/*
 * NodeRendering.cpp
 *
 *  Created on: Mar 13, 2018
 *      Author: florian
 */

#include "NodeRendering.h"

#include <CGAL/Arr_overlay_2.h>
#include <CGAL/Arrangement_2.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <CGAL/General_polygon_set_2.h>
#include <CGAL/Iterator_transform.h>
#include "basic/EasyProfilerCompat.h"
#include <algorithm>
#include <cstddef>
#include <list>
#include <unordered_map>
#include <unordered_set>

namespace laby {

void NodeOverlap::addIdToPolygon(const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    using namespace basic;
    int32_t polygonIndex = 0;

    for (Node* n : _nodes) {
        Arrangement_2Node& arr = n->_setPolygons.arrangement();
        for (FaceNode& face : RangeHelper::make(arr.faces_begin(), arr.faces_end())) {
            face.setPolygonId(polygonIndex);
        }
        polygonIndex++;
    }
}

bool NodeOverlap::testSeg(int32_t index, const basic::HalfedgeNode& he) {
    using namespace basic;
    return edgeHasPolygonId(he, index);
}



bool NodeOverlap::has_face(basic::Arrangement_2Node& res) {
    using namespace basic;
    for (FaceNode& face : RangeHelper::make(res.faces_begin(), res.faces_end())) {
        std::unordered_set<int32_t>& polygonsId = face.data();
        if (polygonsId.size() > 1)
            return true;
    }
    return false;
}

void NodeOverlap::render(OrientedRibbon& oribbon, const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();
    using namespace basic;
    sortNode();

    addIdToPolygon(polyConvexList);

    Overlay_traitsNode overlay_traits;

    if (!_nodes.empty()) {
        std::vector<Arrangement_2Node> arrResult;
        arrResult.reserve(_nodes.size());
        arrResult.emplace_back(_nodes.at(0)->_setPolygons.arrangement());

        for (std::size_t i = 1; i < _nodes.size(); ++i) {
            Arrangement_2Node& last = arrResult.back();
            arrResult.emplace_back();
            CGAL::overlay(last, _nodes.at(i)->_setPolygons.arrangement(), arrResult.back(), overlay_traits);

        }

        Arrangement_2Node& res = arrResult.back();

        if (!has_face(res)) {

            for (HalfedgeNode& he : RangeHelper::make(res.edges_begin(), res.edges_end())) {
                // Check if edge is shared between multiple polygons
                const auto& faceData = he.face()->data();
                const auto& twinData = he.twin()->face()->data();
                if (faceData.size() + twinData.size() > 1) {
                    oribbon.addCCW(Kernel::Segment_2(he.source()->point(), he.target()->point()));

                }

            }
        } else {

            for (FaceNode& face : RangeHelper::make(res.faces_begin(), res.faces_end())) {

                std::unordered_set<int32_t>& polygonsId = face.data();
                if (polygonsId.size() > 1) {
                    int32_t index = *std::min_element(polygonsId.begin(), polygonsId.end());
                    for (HalfedgeNode& he : RangeHelper::make(face.outer_ccb())) {
                        if (testSeg(index, he)) {
                            oribbon.addCCW(Kernel::Segment_2(he.source()->point(), he.target()->point()));

                        }
                    }

                    for (Arrangement_2Node::Inner_ccb_iterator ite = face.holes_begin(); ite != face.holes_end(); ++ite) {
                        for (const HalfedgeNode& he : RangeHelper::make(*ite)) {
                            if (testSeg(index, he)) {
                                oribbon.addCW(Kernel::Segment_2(he.source()->point(), he.target()->point()));
                            }
                        }
                    }

                }
            }
        }
    }

}
void NodeOverlap::sortNode() {
    std::sort(_nodes.begin(), _nodes.end(), [](const Node* a,const Node* b) {
        return a->_state< b->_state;
    });
}

void NodeRendering::render(OrientedRibbon& oribbon, std::vector<Node>& nodes, const std::vector<PolyConvex>& polyConvexList) {
    EASY_FUNCTION();

    for (std::size_t i = 0; i < nodes.size(); ++i) {
        Node& n = nodes.at(i);
        if (n._visited != 1) {

            n._visited = 1;
            NodeOverlap no;
            no._nodes.push_back(&n);
            for (Node* opp : n._opposite) {
                opp->_visited = 1;
                no._nodes.push_back(opp);
            }

            no.render(oribbon, polyConvexList);

        }
    }

}

} /* namespace laby */

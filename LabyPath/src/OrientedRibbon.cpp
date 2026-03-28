/*
 * OrientedRibbon.cpp
 *
 *  Created on: Sep 11, 2018
 *      Author: florian
 */

#include "OrientedRibbon.h"
#include "GeomData.h"
#include "Polyline.h"
#include "Ribbon.h"
#include "basic/RangeHelper.h"
#include <algorithm>
#include <vector>
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>

namespace laby {

namespace {

auto isRight(const Kernel::Point_2& firstPoint, const Kernel::Point_2& secondPoint) -> bool {
    if (firstPoint.y() < secondPoint.y()) {
        return true;
    }
    if (firstPoint.y() > secondPoint.y()) {
        return false;
    }
    if (firstPoint.x() < secondPoint.x()) {
        return true;
    }
    return false;
}

auto setPolylineOrientation(const Halfedge& halfedge, Polyline& polyline) -> void {
    if (halfedge.curve().data().direction() == 1) {
        polyline.setId(isRight(halfedge.source()->point(), halfedge.target()->point()) ? +1 :
                                                                                         -1);
        return;
    }

    polyline.setId(isRight(halfedge.source()->point(), halfedge.target()->point()) ? -1 : +1);
}

void resetEdgeVisits(const Arrangement_2& arrangement) {
    for (const Halfedge& edgeIterator :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        const Segment_info_2& curve = edgeIterator.curve();
        curve.data().setVisit(-1);
    }
}

void appendBranchPolyline(const Halfedge& startHalfedge, Ribbon& ribbon) {
    ribbon.lines().emplace_back();
    Polyline& polyline = ribbon.lines().back();

    for (const Halfedge& halfedge : RangeHelper::make(startHalfedge.twin()->ccb())) {
        polyline.points().emplace_back(halfedge.source()->point());
        setPolylineOrientation(halfedge, polyline);

        if (halfedge.curve().data().getVisit() == 1) {
            break;
        }

        if (halfedge.target()->degree() != 2) {
            halfedge.curve().data().setVisit(1);
            polyline.points().emplace_back(halfedge.target()->point());
            break;
        }
        halfedge.curve().data().setVisit(1);
    }
}

void collectBranchPolylines(const Arrangement_2& arrangement, Ribbon& ribbon) {
    for (const Vertex& vertex :
         RangeHelper::make(arrangement.vertices_begin(), arrangement.vertices_end())) {
        if (!vertex.is_isolated() && vertex.degree() != 2) {
            for (const Halfedge& edgeIterator : RangeHelper::make(vertex.incident_halfedges())) {
                if (edgeIterator.curve().data().getVisit() == -1) {
                    appendBranchPolyline(edgeIterator, ribbon);
                }
            }
        }
    }
}

void appendLoopPolyline(const Halfedge& startHalfedge, Ribbon& ribbon) {
    ribbon.lines().emplace_back();
    Polyline& polyline = ribbon.lines().back();
    bool isClosed = true;

    for (const Halfedge& halfedge : RangeHelper::make(startHalfedge.twin()->ccb())) {
        setPolylineOrientation(halfedge, polyline);
        polyline.points().emplace_back(halfedge.source()->point());
        if (halfedge.curve().data().getVisit() == 1) {
            isClosed = false;
            break;
        }
        halfedge.curve().data().setVisit(1);
    }

    if (isClosed) {
        polyline.points().emplace_back(startHalfedge.target()->point());
        polyline.setClosed(true);
    }
}

void collectLoopPolylines(const Arrangement_2& arrangement, Ribbon& ribbon) {
    for (const Halfedge& edgeIterator :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        if (edgeIterator.curve().data().getVisit() != 1) {
            appendLoopPolyline(edgeIterator, ribbon);
        }
    }
}

void normalizeRibbonOrientation(Ribbon& ribbon) {
    for (Polyline& polyline : ribbon.lines()) {
        polyline.removeConsecutiveDuplicatePoints();

        if (polyline.id() == 1) {
            std::reverse(polyline.points().begin(), polyline.points().end());
            polyline.setId(-1);
        }
    }
}

auto createRibbonOfEdge(const Arrangement_2& arrangement) -> Ribbon {
    resetEdgeVisits(arrangement);

    Ribbon ribbon;
    collectBranchPolylines(arrangement, ribbon);
    collectLoopPolylines(arrangement, ribbon);
    return ribbon;
}

} // namespace

void OrientedRibbon::addCW(const Kernel::Segment_2& segment) {
    if (isRight(segment.source(), segment.target())) {
        _right.emplace_back(segment);
        return;
    }

    _left.emplace_back(segment);
}

void OrientedRibbon::addCCW(const Kernel::Segment_2& segment) {
    // CCW segments have inverted left/right classification compared to CW
    if (isRight(segment.source(), segment.target())) {
        _left.emplace_back(segment);
        return;
    }

    _right.emplace_back(segment);
}

auto OrientedRibbon::createOrientedRibbon() const -> Ribbon {

    Arrangement_2 arrangement;
    std::vector<Segment_info_2> segmentList;
    segmentList.reserve(_left.size() + _right.size());
    for (const Kernel::Segment_2& segment : _left) {
        segmentList.emplace_back(segment, EdgeInfo{1, EdgeInfo::Coordinate{0}});
    }
    for (const Kernel::Segment_2& segment : _right) {
        segmentList.emplace_back(segment, EdgeInfo{3, EdgeInfo::Coordinate{0}});
    }
    CGAL::insert(arrangement, segmentList.begin(), segmentList.end());
    Ribbon result = createRibbonOfEdge(arrangement);
    normalizeRibbonOrientation(result);

    return result;
}

auto OrientedRibbon::getResult() const -> std::vector<Ribbon> {
    std::vector<Ribbon> result;
    result.emplace_back(createOrientedRibbon());
    return result;
}

} /* namespace laby */

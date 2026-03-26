/*
 * OrientedRibbon.cpp
 *
 *  Created on: Sep 11, 2018
 *      Author: florian
 */

#include "OrientedRibbon.h"
#include "basic/Color.h"

namespace laby {

bool OrientedRibbon::isRight(const Kernel::Point_2& p1, const Kernel::Point_2& p2) {
    if (p1.y() < p2.y()) {
        return true;
    }
    if (p1.y() > p2.y()) {
        return false;
    }
    if (p1.x() < p2.x()) {
        return true;
    }
    return false;
}

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

Ribbon OrientedRibbon::createOrientedRibbon() const {

    Arrangement_2 arr;
    std::vector<Segment_info_2> listSeg;
    for (const Kernel::Segment_2& seg : _left) {
        listSeg.push_back(Segment_info_2(seg, EdgeInfo { 1, 0 }));
    }
    for (const Kernel::Segment_2& seg : _right) {
        listSeg.push_back(Segment_info_2(seg, EdgeInfo { 3, 0 }));
    }
    CGAL::insert(arr, listSeg.begin(), listSeg.end());
    Ribbon result = createRibbonOfEdge(arr);

    for (Polyline& pl : result.lines()) {

        if (pl.id == 1) {
            std::reverse(pl.points.begin(), pl.points.end());
            pl.id = -1;
        }
    }

    return result;

}
std::vector<Ribbon> OrientedRibbon::getResult() const {
    std::vector<Ribbon> result;
    result.emplace_back(createOrientedRibbon());
    return result;
}
void OrientedRibbon::setPolylineOrientation(const Halfedge& he, Polyline& poly) {

    if (he.curve().data().direction() == 1) {

        if (isRight(he.source()->point(), he.target()->point())) {
            poly.id = +1;
        } else {
            poly.id = -1;
        }
    } else {

        if (isRight(he.source()->point(), he.target()->point())) {
            poly.id = -1;
        } else {
            poly.id = +1;
        }
    }
}

// copy from Ribbon perhaps we can merge it with the other function
Ribbon OrientedRibbon::createRibbonOfEdge(const Arrangement_2& arr) {

//init
    for (const Halfedge& eit : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        const Segment_info_2& curve = eit.curve();
        curve.data().setVisit(-1);
    }

    Ribbon ribbon;

    for (const Vertex& v : RangeHelper::make(arr.vertices_begin(), arr.vertices_end())) {
        if (!v.is_isolated() and v.degree() != 2) {
            for (const Halfedge& eit : RangeHelper::make(v.incident_halfedges())) {
                if (eit.curve().data().getVisit() == -1) {
                    ribbon.lines().emplace_back();

                    Polyline& poly = ribbon.lines().back();

                    for (const Halfedge& he : RangeHelper::make(eit.twin()->ccb())) {
                        poly.points.emplace_back(he.source()->point());
                        setPolylineOrientation(he, poly);

                        if (he.curve().data().getVisit() == 1) {
                            break;
                        }

                        if (he.target()->degree() != 2) {
                            he.curve().data().setVisit(1);
                            poly.points.emplace_back(he.target()->point());
                            break;
                        }
                        he.curve().data().setVisit(1);
                    }

                }
            }
        }

    }
//loop (only degree 2 )
    for (const Halfedge& eit : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {

        if (eit.curve().data().getVisit() != 1) {

            ribbon.lines().emplace_back();
            Polyline& poly = ribbon.lines().back();
            bool closed = true;

            for (const Halfedge& he : RangeHelper::make(eit.twin()->ccb())) {
                setPolylineOrientation(he, poly);

                poly.points.emplace_back(he.source()->point());
                if (he.curve().data().getVisit() == 1) {
                    closed = false;
                    break;
                }
                he.curve().data().setVisit(1);
            }

            if (closed) {
                poly.points.emplace_back(eit.target()->point());
                poly.closed = true;
            }
        }
    }
    return ribbon;

}

} /* namespace laby */

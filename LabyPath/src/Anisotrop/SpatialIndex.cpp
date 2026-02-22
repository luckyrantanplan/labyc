/*
 * SpatialIndex.cpp
 *
 *  Created on: Apr 3, 2018
 *      Author: florian
 */

#include "SpatialIndex.h"

#include <boost/geometry/index/predicates.hpp>
#include <boost/geometry/index/rtree.hpp>
#include <boost/tuple/detail/tuple_basic.hpp>
//#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
#include <CGAL/Polygon_2.h>
#include <CGAL/squared_distance_2.h>
#include <CGAL/Vector_2.h>
#include "basic/EasyProfilerCompat.h"
#include <cmath>

#include "Net.h"
#include "QueueCost.h"

namespace laby {
namespace aniso {

SpatialIndex::SpatialIndex(std::vector<PolyConvex>& polyConvexList) :
        _polyConvexList { polyConvexList } {
    EASY_FUNCTION();
}

void SpatialIndex::insert(const PolyConvex& pc) {
    EASY_FUNCTION();
    CGAL::Bbox_2 bbox = pc._geometry.bbox();
    _rtree.insert(Value { getBox(bbox), pc._id });
}

SpatialIndex::Box SpatialIndex::getBox(const CGAL::Bbox_2& bbox) const {

    return Box { Point { bbox.xmin(), bbox.ymin() }, Point { bbox.xmax(), bbox.ymax() } };
}

CGAL::Segment_2<SpatialIndex::KApprox> SpatialIndex::getApproxSeg(const PolyConvex& pc) {
    const CGAL::Segment_2<Kernel>& c1 = pc._supportHe->curve();
    CGAL::Point_2<KApprox> p1(CGAL::to_double(c1.source().x()), CGAL::to_double(c1.source().y()));
    CGAL::Point_2<KApprox> p2(CGAL::to_double(c1.target().x()), CGAL::to_double(c1.target().y()));
    CGAL::Segment_2<KApprox> seg1(p1, p2);
    return seg1;
}

bool SpatialIndex::is_PointInside(const PolyConvex & pc1, const Vertex& vertex2, uint32_t& index) const {
    EASY_FUNCTION();

    CGAL::Bbox_2 bbox = vertex2.point().bbox();

    return _rtree.qbegin(boost::geometry::index::intersects(getBox(bbox)) && //
            boost::geometry::index::satisfies([&pc1,&vertex2,& index, this](Value const& v) {
                EASY_BLOCK("is_PointInside_Callback");

                const PolyConvex & pc2 = _polyConvexList.at(v.second);

                const std::vector<std::size_t>& w=pc1._adjacents;
                if (pc2._id==pc1._id or std::find(w.begin(), w.end(), pc2._id) != w.end()) {
                    return false;
                }

                if (pc2._geometry.has_on_bounded_side(vertex2.point()) or //
                    pc2._geometry.has_on_boundary(vertex2.point())) {
                index=v.second;
                return true;
            }
            return false;
        })) != _rtree.qend();

}

bool SpatialIndex::areSegmentsCloseEnough(const CGAL::Segment_2<KApprox>& seg, const CGAL::Segment_2<KApprox>& seg2, const double& thickness) {

    return (CGAL::squared_distance(seg, seg2) < (thickness * thickness));
}

double SpatialIndex::cosinus(const CGAL::Segment_2<KApprox>& seg, const CGAL::Segment_2<KApprox>& seg2) const {
    CGAL::Vector_2<KApprox> v1 = seg.to_vector();
    CGAL::Vector_2<KApprox> v2 = seg2.to_vector();
    v1 = v1 / sqrt(v1.squared_length());
    v2 = v2 / sqrt(v2.squared_length());
    return v1 * v2;
}

bool SpatialIndex::test_is_same_pin(const Halfedge& he, const Net& net) const {
    const Vertex& vsource = *he.source();
    const Vertex& vtarget = *he.target();
    const laby::Vertex& v0 = net.source().vertex();
    const laby::Vertex& v1 = net.target().vertex();

    if (vsource.data().getType() == VertexInfo::PIN) {
        if (&vsource == &v0 or &vsource == &v1) {
            return true;
        }
    }
    if (vtarget.data().getType() == VertexInfo::PIN) {
        if (&vtarget == &v0 or &vtarget == &v1) {
            return true;
        }
    }
    return false;
}

bool SpatialIndex::test_pin_proximity(const CGAL::Segment_2<KApprox> seg, const Vertex& vertex, const Net& net, const double& thickness) const {
    if (vertex.data().getType() == VertexInfo::PIN) {

        CGAL::Point_2<KApprox> p1(CGAL::to_double(vertex.point().x()), CGAL::to_double(vertex.point().y()));
        if (CGAL::to_double(CGAL::squared_distance(p1, seg)) <= thickness * thickness) {
            return true;
        }

    }
    return false;
}

bool SpatialIndex::test_intersection_test(const PolyConvex & pc, const PolyConvex & pc2) {
    const CGAL::Segment_2<KApprox> seg2 = getApproxSeg(pc2);
    double thick2 = pc2.thickness();
    const CGAL::Segment_2<KApprox> seg = getApproxSeg(pc);
    double thick1 = pc.thickness();

    return areSegmentsCloseEnough(seg, seg2, (thick2 + thick1) / 2);

}

bool SpatialIndex::intersection_test(QueueCost& cost, const PolyConvex & pc, const Net& net, const std::unordered_set<int32_t>& nets_target, const CGAL::Segment_2<KApprox>& seg, double thick1,
        Value const& v) const {
    EASY_FUNCTION();
    const PolyConvex & pc2 = _polyConvexList.at(v.second);
//    if (pc2._supportHe->curve().data().direction() == EdgeInfo::CELL && pc._supportHe->curve().data().direction() == EdgeInfo::CELL) {
//        return false;
//    }
    const CGAL::Segment_2<KApprox> seg2 = getApproxSeg(pc2);
    double thick2 = pc2.thickness();
    double min_separation = (thick2 + thick1) / 2;
    if (areSegmentsCloseEnough(seg, seg2, min_separation)) {

        const Vertex& vsource = *pc2._supportHe->source();
        const Vertex& vtarget = *pc2._supportHe->target();

        if (test_is_same_pin(*pc2._supportHe, net)) {

            return false;
        }

        if (test_pin_proximity(seg, vsource, net, min_separation) or //
                (test_pin_proximity(seg, vtarget, net, min_separation))) {
            ++cost.congestion;

            return true;
        }

        double sqPd = cosinus(seg, seg2);
        // <45 degree : true if we should block
        if (sqPd * sqPd > 0.5) {
            int32_t net_id = pc2._supportHe->curve().data().get_net();
            if (cost.memory_source.count(net_id)) {
                cost.future_memory_source.emplace(net_id);
            } else if (nets_target.count(net_id)) {
                cost.future_memory_target.emplace(net_id);
            } else {

                ++cost.congestion;
            }

            return true;
        }
    }

    return false;
}

void SpatialIndex::update_cost_if_intersect(const PolyConvex & pc, const Net& net, const std::unordered_set<int32_t>& nets_target, QueueCost& cost) const {
    EASY_FUNCTION();

    CGAL::Bbox_2 bbox = pc._geometry.bbox();
    const CGAL::Segment_2<KApprox> seg = getApproxSeg(pc);
    double thick1 = pc.thickness();

    _rtree.qbegin(boost::geometry::index::intersects(getBox(bbox)) && //
            boost::geometry::index::satisfies([&cost,&pc,&net,&nets_target,&seg,&thick1,this](Value const& v) {
                return intersection_test(cost, pc,net,nets_target, seg, thick1, v);
            }));

}

} /* namespace aniso */
} /* namespace laby */

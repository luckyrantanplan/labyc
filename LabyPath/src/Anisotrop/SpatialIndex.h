/*
 * SpatialIndex.h
 *
 *  Created on: Apr 3, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_SPATIALINDEX_H_
#define ANISOTROP_SPATIALINDEX_H_

#include <bits/stdint-uintn.h>
#include <boost/geometry.hpp>
#include <CGAL/Segment_2.h>
#include <CGAL/Simple_cartesian.h>
#include <utility>
#include <vector>

#include "../GeomData.h"
#include "../PolyConvex.h"

namespace laby {
namespace aniso {

class QueueCost;
class Net;

class SpatialIndex {

public:
    typedef boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian> Point;
    typedef boost::geometry::model::box<Point> Box;
    typedef std::pair<Box, uint32_t> Value;

    typedef CGAL::Simple_cartesian<double> KApprox;

    // create the rtree using default constructor
    typedef boost::geometry::index::rtree<Value, boost::geometry::index::quadratic<16> > RTree;

    explicit SpatialIndex(std::vector<PolyConvex>& polyConvexList);

    void insert(const PolyConvex& pc);

    void update_cost_if_intersect(const PolyConvex & pc, const Net& net, const std::unordered_set<int32_t>& nets_target, QueueCost& cost) const;
    bool is_PointInside(const PolyConvex & pc1, const Vertex& vertex2, uint32_t& index) const;
    double cosinus(const CGAL::Segment_2<KApprox>& seg, const CGAL::Segment_2<KApprox>& seg2) const;

    static bool test_intersection_test(const PolyConvex & pc, const PolyConvex & pc2);

private:
    bool test_pin_proximity(const CGAL::Segment_2<KApprox> seg, const Vertex& vsource, const Net& net, const double& thickness) const;
    bool intersection_test(QueueCost& cost, const PolyConvex & pc, const Net& net, const std::unordered_set<int32_t>& nets_target, const CGAL::Segment_2<KApprox>& seg, double thick1,
            Value const& v) const;
    bool test_is_same_pin(const Halfedge& he, const Net& net) const;
    static bool areSegmentsCloseEnough(const CGAL::Segment_2<KApprox>& seg, const CGAL::Segment_2<KApprox>& seg2, const double& thickness);
    Box getBox(const CGAL::Bbox_2& bbox) const;
    static CGAL::Segment_2<KApprox> getApproxSeg(const PolyConvex& pc);
    std::vector<PolyConvex>& _polyConvexList;
    RTree _rtree;
}
;

} /* namespace aniso */
} /* namespace laby */

#endif /* ANISOTROP_SPATIALINDEX_H_ */

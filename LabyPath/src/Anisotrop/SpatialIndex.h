/*
 * SpatialIndex.h
 *
 *  Created on: Apr 3, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_SPATIALINDEX_H_
#define ANISOTROP_SPATIALINDEX_H_

#include <CGAL/Segment_2.h>
#include <CGAL/Simple_cartesian.h>
#include <boost/geometry.hpp>
#include <cstdint>
#include <utility>
#include <vector>

#include "../GeomData.h"
#include "../PolyConvex.h"

namespace laby::aniso {

class QueueCost;
class Net;

class SpatialIndex {

  public:
    static constexpr int kRTreeQuadraticNodes = 16;

    using Point = boost::geometry::model::point<double, 2, boost::geometry::cs::cartesian>;
    using Box = boost::geometry::model::box<Point>;
    using Value = std::pair<Box, uint32_t>;

    using KApprox = CGAL::Simple_cartesian<double>;

    // create the rtree using default constructor
    using RTree =
        boost::geometry::index::rtree<Value,
                                      boost::geometry::index::quadratic<kRTreeQuadraticNodes>>;

    explicit SpatialIndex(std::vector<PolyConvex>& polyConvexList);

    void insert(const PolyConvex& polyConvex);

    void updateCostIfIntersect(const PolyConvex& polyConvex, const Net& net,
                               const std::unordered_set<int32_t>& targetNets,
                               QueueCost& cost) const;
    auto isPointInside(const PolyConvex& polyConvex, const Vertex& vertex,
                       uint32_t& index) const -> bool;
    [[nodiscard]] static auto cosinus(const CGAL::Segment_2<KApprox>& segment,
                                      const CGAL::Segment_2<KApprox>& otherSegment) -> double;

    static auto testIntersectionTest(const PolyConvex& polyConvex,
                                     const PolyConvex& otherPolyConvex) -> bool;

  private:
    [[nodiscard]] static auto testPinProximity(const CGAL::Segment_2<KApprox>& segment,
                                               const Vertex& sourceVertex, const Net& net,
                                               double thickness) -> bool;
    auto intersectionTest(QueueCost& cost, const PolyConvex& polyConvex, const Net& net,
                          const std::unordered_set<int32_t>& targetNets,
                          const CGAL::Segment_2<KApprox>& segment, double thickness,
                          const Value& value) const -> bool;
    [[nodiscard]] static auto testIsSamePin(const Halfedge& halfedge, const Net& net) -> bool;
    static auto areSegmentsCloseEnough(const CGAL::Segment_2<KApprox>& segment,
                                       const CGAL::Segment_2<KApprox>& otherSegment,
                                       double thickness) -> bool;
    [[nodiscard]] static auto getBox(const CGAL::Bbox_2& bbox) -> Box;
    static auto getApproxSeg(const PolyConvex& polyConvex) -> CGAL::Segment_2<KApprox>;
    std::vector<PolyConvex>* _polyConvexList = nullptr;
    RTree _rtree;
};

} // namespace laby::aniso

#endif /* ANISOTROP_SPATIALINDEX_H_ */

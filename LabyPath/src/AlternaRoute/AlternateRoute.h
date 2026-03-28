/*
 * AlternateRoute.h
 *
 *  Created on: Aug 21, 2018
 *      Author: florian
 */

#ifndef WORKSPACE_LABYPATH_SRC_ALTERNAROUTE_ALTERNATEROUTE_H
#define WORKSPACE_LABYPATH_SRC_ALTERNAROUTE_ALTERNATEROUTE_H

#include "../GeomData.h"
#include "../GridIndex.h"
#include "../Ribbon.h"
#include "../protoc/AllConfig.pb.h"
#include "StrokeArrangement.h"

#include <ostream>
#include <unordered_map>
#include <vector>

namespace laby {

class PolyConvex;

class AlternateRoute {

    struct OffsetPair {
      public:
        [[nodiscard]] auto origin() const -> const Kernel::Point_2& {
            return _origin;
        }

        [[nodiscard]] auto offset() const -> const Kernel::Point_2& {
            return _offset;
        }

        void setOrigin(const Kernel::Point_2& origin) {
            _origin = origin;
        }

        void setOffset(const Kernel::Point_2& offset) {
            _offset = offset;
        }

        void print(std::ostream& outputStream) const {
            outputStream << " origin " << origin();
            outputStream << " offset " << offset();
        }
        static auto simplify(std::vector<OffsetPair>& list,
                             const double& distance) -> std::vector<OffsetPair>;

      private:
        Kernel::Point_2 _origin;
        Kernel::Point_2 _offset;
    };

    struct OffsetEndpoints {
        Kernel::Point_2 originPoint;
        Kernel::Point_2 targetPoint;
    };

  public:
    AlternateRoute(proto::AlternateRouting configfilepaths, const proto::Filepaths& filepaths);

  private:
    proto::AlternateRouting _config;
    double _sqMaxThickness = 0.0;
    double _sqMinThickness = 0.0;

    [[nodiscard]] auto pruneArrangement(const Arrangement_2& arrangement) const -> Arrangement_2;
    [[nodiscard]] static auto removeAntenna(const Arrangement_2& arrangement) -> Arrangement_2;
    void addPoint(std::vector<OffsetPair>& offsets, const OffsetEndpoints& endpoints);
    [[nodiscard]] auto coupleList(const Halfedge& halfedge,
                                  int32_t direction) -> std::vector<OffsetPair>;
    static void addTriplet(alter::OffsetTriplet& triplet, const OffsetPair& offsetPair,
                           const Kernel::Point_2& lineStart, const Kernel::Point_2& lineEnd);

    [[nodiscard]] auto voronoiArr(const Arrangement_2& arrangement, int32_t direction,
                                  const CGAL::Bbox_2& viewBox,
                                  const Ribbon& ribLimit) -> Arrangement_2;
    [[nodiscard]] auto createTripletList(const Halfedge& halfedge,
                                         int32_t direction) -> std::vector<alter::OffsetTriplet>;
    void populateTrapeze(const GridIndex& gridIndex, const std::vector<Ribbon>& ribList,
                         const CGAL::Bbox_2& viewBox,
                         std::vector<alter::SegmentTrapezeInfo2>& trapezeVect);
    void ribToTrapeze(const Ribbon& rib, std::vector<alter::SegmentTrapezeInfo2>& trapezeVect,
                      const Arrangement_2& arrangement, const CGAL::Bbox_2& viewBox,
                      const Ribbon& ribLimit);
    static void copyStrokeColorToFillColor(std::vector<Ribbon>& ribList);
    [[nodiscard]] auto
    buildTrapezeVector(const std::unordered_map<uint32_t, GridIndex>& mapOfGrids,
                       const std::vector<Ribbon>& ribList,
                       const CGAL::Bbox_2& viewBox) -> std::vector<alter::SegmentTrapezeInfo2>;
    [[nodiscard]] static auto buildPolyConvexVector(
        const alter::ArrTrapeze& arrTrapeze,
        std::unordered_map<const alter::SegmentTrapezeInfo2*, std::size_t>& curvePlcMap)
        -> std::vector<PolyConvex>;
    static void connectPolyConvexes(
        const alter::ArrTrapeze& arrTrapeze,
        const std::unordered_map<const alter::SegmentTrapezeInfo2*, std::size_t>& curvePlcMap,
        std::vector<PolyConvex>& polyConvexVect);
};

} /* namespace laby */

#endif /* WORKSPACE_LABYPATH_SRC_ALTERNAROUTE_ALTERNATEROUTE_H */

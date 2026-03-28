/*
 * SkeletonRadial.h
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#ifndef SKELETONRADIAL_H_
#define SKELETONRADIAL_H_
#include "GeomData.h"
#include "basic/AugmentedPolygonSet.h"
#include "basic/RandomUniDist.h"
#include <CGAL/Point_set_2.h>

#include <cstdint>

#include "SkeletonGrid.h"
#include <utility>

namespace laby {

class SkeletonRadial {

    struct FaceHelper {
        enum class Type : std::uint8_t { Unknown, Lateral, Corner };
        Type _type = Type::Unknown;
        Kernel::Vector_2 _perp;
        Kernel::Point_2 _o;
    };

    struct Incidence {

        Incidence() = default;

        Incidence(Point_2 testPoint, const basic::HalfedgeNode& halfedge)
            : isEmpty(false), _test(std::move(testPoint)), _hedge(&halfedge) {}
        bool isEmpty = true;
        Point_2 _test;
        const basic::HalfedgeNode* _hedge = nullptr;

        [[nodiscard]] auto getTwin() const -> Incidence {
            Incidence twinIncidence(*this);
            if (twinIncidence._hedge != nullptr) {
                twinIncidence._hedge = &*twinIncidence._hedge->twin();
            }
            return twinIncidence;
        }
    };

  public:
    struct Config {
        double epsilon = 1e-50;
        double cosTheta = 0.93;
        double filtered_distance = 0.2;
        double sep = 1.5;
        double sep_subdivision = 100;
        double displacement = 1e-6;
        int32_t max_polyline_element = 100;
        double min_length_polyline = 0.75; // sep/2
        std::uint32_t seed = 0;
    };

    explicit SkeletonRadial(const Config& config)
        : _config(config), _random(0, 100.0, _config.seed) {}

    auto createRadial(const basic::Arrangement_2Node& arrangement,
                      const CGAL::Polygon_with_holes_2<Kernel>& polygonWithHoles) -> void;

    auto getPointIntersect(const CGAL::Polygon_with_holes_2<Kernel>& polygonWithHoles) const
        -> CGAL::Point_set_2<Kernel>;
    auto radialList() const -> std::vector<Kernel::Segment_2>;

    auto getRibbon() const -> const Ribbon& {
        return _radial_list;
    }

  private:
    auto
    registerFace(const basic::HalfedgeNode& halfedge,
                 std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) const -> void;
    auto traverseFace(const Incidence& incidence,
                      const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
                      std::vector<Kernel::Point_2>& result) const -> SkeletonRadial::Incidence;
    auto traverseCorner(const Incidence& incidence, const FaceHelper& faceHelper,
                        std::vector<Kernel::Point_2>& result) const -> Incidence;
    auto
    iterateEdge(const basic::HalfedgeNode& halfedge, const double& spacing,
                CGAL::Point_set_2<Kernel>& pointSet,
                const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void;
    auto
    iterateCorner(const basic::HalfedgeNode& halfedge, const double& spacing,
                  CGAL::Point_set_2<Kernel>& pointSet,
                  const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void;

    auto cropLine(Kernel::Vector_2 vector, const Kernel::Line_2& line,
                  Kernel::Iso_rectangle_2 boundingBox,
                  std::vector<Kernel::Segment_2>& resultSegments) const -> void;
    auto traverseRay(const basic::HalfedgeNode& halfedge, const Kernel::Point_2& testPoint,
                     const Kernel::Vector_2& direction,
                     std::vector<Kernel::Point_2>& result) const -> Incidence;

    auto filterNewVertex(const std::vector<Kernel::Point_2>& result,
                         const Kernel::Point_2& vertex) const -> bool;
    auto polylineLength(const std::vector<Kernel::Point_2>& result) const -> double;
    auto polygonContour(const basic::Arrangement_2Node& arrangement,
                        const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
                        CGAL::Point_set_2<Kernel>& pointSet) -> void;

    auto
    fillCorner(const basic::Arrangement_2Node& arrangement,
               const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache) -> void;
    auto fillRadialList(std::vector<Kernel::Point_2>& result,
                        CGAL::Point_set_2<Kernel>& pointSet) -> void;
    auto startTraverseEdge(const Kernel::Point_2& testPoint, const double& spacing,
                           const basic::HalfedgeNode& halfedge,
                           const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
                           CGAL::Point_set_2<Kernel>& pointSet) -> void;
    auto
    startTraverseCorner(const Kernel::Point_2& testPoint, const double& spacing,
                        const basic::HalfedgeNode& halfedge,
                        const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
                        CGAL::Point_set_2<Kernel>& pointSet) -> std::vector<Kernel::Point_2>;

    Config _config;
    mutable basic::RandomUniDist _random;
    Ribbon _radial_list;
};

} /* namespace laby */

#endif /* SKELETONRADIAL_H_ */

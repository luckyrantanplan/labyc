/*
 * SkeletonRadial.h
 *
 *  Created on: Jul 9, 2018
 *      Author: florian
 */

#ifndef SKELETONRADIAL_H_
#define SKELETONRADIAL_H_
#include "basic/AugmentedPolygonSet.h"
#include "basic/RandomUniDist.h"
#include "GeomData.h"
#include <CGAL/Point_set_2.h>

#include <utility>
#include "SkeletonGrid.h"

namespace laby {

class SkeletonRadial {

    struct FaceHelper {
        enum Type {
            UNKNOW, LATERAL, CORNER
        };
        Type _type = UNKNOW;
        Kernel::Vector_2 _perp;
//        Kernel::Segment_2 _bissectrice;
//        Kernel::Line_2 _l1;
//        Kernel::Line_2 _l2;

        Kernel::Point_2 _o;

    };

    struct Incidence {

        Incidence() = default;

        Incidence(Point_2  test, const basic::HalfedgeNode& hedge) :
                isEmpty(false), _test(std::move(test)), _hedge(&hedge) {
        }
        bool isEmpty = true;
        Point_2 _test;
        const basic::HalfedgeNode* _hedge = nullptr;

        auto getTwin() -> Incidence {
            Incidence i(*this);
            if (i._hedge != nullptr) {
                i._hedge = &*i._hedge->twin();
            }
            return i;
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
        uint32_t seed = 0;
    };

    explicit SkeletonRadial(const Config& config) :
            _config(config), _random(0, 100.0, _config.seed) {
    }

    void createRadial(const basic::Arrangement_2Node& arr3, const CGAL::Polygon_with_holes_2<Kernel>& poly_hole);

    auto getPointIntersect(const CGAL::Polygon_with_holes_2<Kernel>& poly_hole) const -> CGAL::Point_set_2<Kernel>;
    auto radialList() const -> const std::vector<Kernel::Segment_2>;

    auto getRibbon() const -> const Ribbon& {
        return _radial_list;
    }

private:
    void registerFace(const basic::HalfedgeNode& he, std::unordered_map<const basic::FaceNode*, FaceHelper>& vertices_cache) const;
    auto traverseFace(const Incidence& incidence, const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache, //
            std::vector<Kernel::Point_2>& result) const -> SkeletonRadial::Incidence;
    auto traverseCorner(const Incidence& incidence, const FaceHelper& faceHelper, std::vector<Kernel::Point_2>& result) const -> Incidence;
    void iterateEdge(const basic::HalfedgeNode& hedge, const double& sep, CGAL::Point_set_2<Kernel>& pt_set, const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache);
    void iterateCorner(const basic::HalfedgeNode& hedge, const double& sep, CGAL::Point_set_2<Kernel>& pt_set, const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache);

    void cropLine(Kernel::Vector_2 vect, const Kernel::Line_2& line, Kernel::Iso_rectangle_2 bbox, std::vector<Kernel::Segment_2>& result2) const;
    auto traverseRay(const basic::HalfedgeNode& hedge, const Kernel::Point_2& test, //
            const Kernel::Vector_2& dir, std::vector<Kernel::Point_2>& result) const -> Incidence;

    auto filterNewVertex(const std::vector<Kernel::Point_2>& result, const Kernel::Point_2& vertex) const -> bool;
    auto polylineLength(const std::vector<Kernel::Point_2>& result) const -> double;
    void polygonContour(const basic::Arrangement_2Node& arr3, //
            const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,   //
            CGAL::Point_set_2<Kernel>& pt_set);

    void fillCorner(const basic::Arrangement_2Node& arr3, const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache);
    void fillRadialList(std::vector<Kernel::Point_2>& result, CGAL::Point_set_2<Kernel>& pt_set);
    void startTraverseEdge(const Kernel::Point_2& test, const double& sep, const basic::HalfedgeNode& hedge2, const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache,
            CGAL::Point_set_2<Kernel>& pt_set);
    auto startTraverseCorner(const Kernel::Point_2& test, const double& sep, const basic::HalfedgeNode& hedge2,
            const std::unordered_map<const basic::FaceNode*, FaceHelper>& faceCache, CGAL::Point_set_2<Kernel>& pt_set) -> std::vector<Kernel::Point_2>;

    const Config _config;
    mutable basic::RandomUniDist _random;
    Ribbon _radial_list;
}
;

} /* namespace laby */

#endif /* SKELETONRADIAL_H_ */

/*
 * PenStroke.cpp
 *
 *  Created on: Jun 11, 2018
 *      Author: florian
 */

#include "PenStroke.h"

#include "../agg/agg_arc.h"
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/Point_2.h>
#include <CGAL/Polygon_set_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace laby {

namespace {
constexpr double kHalfWidthScale = 2.0;
constexpr double kSimplifyTolerance = 0.0001;
} // namespace

auto PenStroke::LineConstruct::getMedianList() const -> const Polyline& {
    return _anti;
}

void PenStroke::LineConstruct::setClosed() {
    _anti.setClosed(true);
    _anti.points().emplace_back(_anti.points().front());
    _sym.emplace_back(_sym.front());
}
auto PenStroke::LineConstruct::isClosed() const -> bool {
    return _anti.isClosed();
}

auto PenStroke::LineConstruct::getMedian(const std::size_t pointIndex) const -> const Point_2& {
    return _anti.points().at(pointIndex);
}

auto PenStroke::LineConstruct::getBorder(const std::size_t pointIndex) const -> const double& {
    return _sym.at(pointIndex);
}

auto PenStroke::smoothstep(const double edgeStart, const double edgeEnd, double value) -> double {
    // Scale, bias and saturate x to 0..1 range
    value = std::clamp((value - edgeStart) / (edgeEnd - edgeStart), 0.0, 1.0);
    // Evaluate polynomial
    return value * value * (3 - 2 * value);
}

auto PenStroke::barycentre(const Point_2& origin, const int32_t vertexIndex,
                           const CGAL::Vector_2<Kernel>& unitVector) -> Point_2 {
    return origin + vertexIndex * unitVector;
}

auto PenStroke::smooth(const Point_2& point, const Point_2& origin) const -> double {
    if (point == origin) {
        return 0;
    }
    return smoothstep(0, _config.antisymmetric_amplitude(),
                      sqrt(CGAL::to_double((point - origin).squared_length())));
}

void PenStroke::LineConstruct::addPoint(const Point_2& point, const double borderValue) {
    _anti.points().emplace_back(point);
    _sym.emplace_back(borderValue);
}

void PenStroke::addPoint(LineConstruct& lineConstruct, const Point_2& point, const Point_2& origin,
                         const Point_2& destination, const int32_t increment,
                         const CGAL::Vector_2<Kernel>& normalVector) {

    const double symmetricNoise =
        _hqNoise2DSym.get(CGAL::to_double(point.x()) + 10., CGAL::to_double(point.y()) + 10.);
    const double antisymmetricNoise =
        _hqNoise2DAnti.get(CGAL::to_double(point.x()) + 10., CGAL::to_double(point.y()) + 10.);

    const double antisymmetricClamp =
        antisymmetricNoise * smooth(point, origin) * smooth(point, destination);

    lineConstruct.addPoint(point + normalVector * (increment * antisymmetricClamp),
                           symmetricNoise + _config.thickness());
}

void PenStroke::lineTo(svg::Path& path, const Point_2& point) {
    path << point;
}

void PenStroke::moveTo(svg::Path& path, const Point_2& point) {
    path.startNewSubPath();
    path << point;
}

auto PenStroke::createPenStroke(const proto::PenStroke& config,
                                const CGAL::Bbox_2& bbox) -> PenStroke {
    NoiseConfigs noiseConfigs{};
    noiseConfigs.symmetric.maxN = static_cast<uint32_t>(std::max(bbox.xmax(), bbox.ymax()));
    noiseConfigs.symmetric.seed = config.symmetric_seed();
    noiseConfigs.symmetric.amplitude = config.symmetric_amplitude();
    noiseConfigs.symmetric.accuracy = static_cast<uint32_t>(std::ceil(1. / config.resolution()));
    noiseConfigs.symmetric.gaussian.frequency = noiseConfigs.symmetric.maxN * 100.;
    noiseConfigs.symmetric.powerlaw.frequency =
        noiseConfigs.symmetric.maxN / config.symmetric_freq();
    noiseConfigs.symmetric.powerlaw.power = 2;
    noiseConfigs.symmetric.complex = false;

    noiseConfigs.antisymmetric.maxN = static_cast<uint32_t>(std::max(bbox.xmax(), bbox.ymax()));
    noiseConfigs.antisymmetric.seed = config.antisymmetric_seed();
    noiseConfigs.antisymmetric.amplitude = config.antisymmetric_amplitude();
    noiseConfigs.antisymmetric.accuracy =
        static_cast<uint32_t>(std::ceil(1. / config.resolution()));
    noiseConfigs.antisymmetric.gaussian.frequency = noiseConfigs.symmetric.maxN * 100.;
    noiseConfigs.antisymmetric.powerlaw.frequency =
        noiseConfigs.symmetric.maxN / config.antisymmetric_freq();
    noiseConfigs.antisymmetric.powerlaw.power = 2;
    noiseConfigs.antisymmetric.complex = false;

    return PenStroke(config, noiseConfigs);
}

void PenStroke::drawRibbonStroke(svg::Path& path, const Ribbon& ribbon) {
    for (const Polyline& polyline : ribbon.lines()) {
        if (polyline.points().size() > 1UL) {

            moveTo(path, polyline.points().at(0));

            for (std::size_t pointIndex = 1; pointIndex < polyline.points().size(); ++pointIndex) {
                lineTo(path, polyline.points().at(pointIndex));
            }
            if (polyline.isClosed()) {
                path.closeSubPath();
            }
        }
    }
}

auto PenStroke::getSegmentFromMedian(const std::unordered_set<std::size_t>& referenceMedianLineSet)
    -> std::vector<Segment_info_2> {
    std::vector<Segment_info_2> segmentList;
    for (const std::size_t medianLineIndex : referenceMedianLineSet) {
        const Polyline& medianPolyline = _medrib.at(medianLineIndex).getMedianList();
        for (std::size_t pointIndex = 1; pointIndex < medianPolyline.points().size(); ++pointIndex) {
            segmentList.emplace_back(Kernel::Segment_2(medianPolyline.points().at(pointIndex - 1),
                                                       medianPolyline.points().at(pointIndex)),
                                     EdgeInfo{1, EdgeInfo::Coordinate{0}});
        }
    }
    return segmentList;
}

auto PenStroke::fillFace(const std::vector<Segment_info_2>& segmentList) -> Ribbon {
    Ribbon ribbon;

    Arrangement_2 arrangement;
    CGAL::insert(arrangement, segmentList.begin(), segmentList.end());
    for (const Face& face : RangeHelper::make(arrangement.unbounded_faces_begin(),
                                              arrangement.unbounded_faces_end())) {
        for (Arrangement_2::Inner_ccb_const_iterator holeIterator = face.holes_begin();
             holeIterator != face.holes_end(); ++holeIterator) {
            ribbon.lines().emplace_back();
            Polyline& polyline = ribbon.lines().back();

            polyline.points().emplace_back((*holeIterator)->source()->point());

            for (const Halfedge& halfedge : RangeHelper::make(*holeIterator)) {
                polyline.points().emplace_back(halfedge.target()->point());
            }
            polyline.setClosed(true);
        }
    }

    return ribbon;
}

auto PenStroke::faceWithoutAntenna(Arrangement_2& arrangement, const Face& face) -> const Face& {
    std::vector<Segment_info_2> segmentList;
    for (const Halfedge& halfedge : RangeHelper::make(face.outer_ccb())) {
        if (&*halfedge.face() != &*halfedge.twin()->face()) { // not an antenna
            segmentList.push_back(halfedge.curve());
        }
    }

    for (Arrangement_2::Inner_ccb_const_iterator holeIterator = face.holes_begin();
         holeIterator != face.holes_end(); ++holeIterator) {
        for (const Halfedge& halfedge : RangeHelper::make(*holeIterator)) {
            if (&*halfedge.face() != &*halfedge.twin()->face()) { // not an antenna
                segmentList.push_back(halfedge.curve());
            }
        }
    }

    CGAL::insert(arrangement, segmentList.begin(), segmentList.end());

    const Face& unboundedFace = *arrangement.unbounded_face();
    Arrangement_2::Inner_ccb_const_iterator holeIterator = unboundedFace.holes_begin();
    return *((*holeIterator)->twin()->face());
}

void PenStroke::drawFace(svg::Path& path, const Face& face) {

    if (face.has_outer_ccb()) {
        Arrangement_2 arrangementWithoutAntenna;
        const Face& simplifiedFace = faceWithoutAntenna(arrangementWithoutAntenna, face);
        {
            std::unordered_set<std::size_t> referenceMedianLineSet;
            for (const Halfedge& halfedge : RangeHelper::make(simplifiedFace.outer_ccb())) {
                referenceMedianLineSet.emplace(halfedge.curve().data().coordinate());
            }
            Ribbon ribbon = fillFace(getSegmentFromMedian(referenceMedianLineSet));
            ribbon.reverse();
            drawRibbonStroke(path, ribbon);
        }

        for (Arrangement_2::Inner_ccb_const_iterator holeIterator = simplifiedFace.holes_begin();
             holeIterator != simplifiedFace.holes_end(); ++holeIterator) {
            std::unordered_set<std::size_t> referenceMedianLineSet;
            for (const Halfedge& halfedge : RangeHelper::make(*holeIterator)) {
                referenceMedianLineSet.emplace(halfedge.curve().data().coordinate());
            }
            drawRibbonStroke(path, fillFace(getSegmentFromMedian(referenceMedianLineSet)));
        }
    }
}

void PenStroke::createUnion(Ribbon& ribbon, const std::vector<PolyConvex>& polyConvexList) {

    CGAL::Polygon_set_2<Kernel> set;

    std::vector<Linear_polygon> polygonList;
    polygonList.reserve(polyConvexList.size());
    for (const PolyConvex& polyConvex : polyConvexList) {
        polygonList.emplace_back(polyConvex._geometry);
    }
    set.join(polygonList.begin(), polygonList.end());

    std::vector<CGAL::Polygon_with_holes_2<Kernel>> polygons;

    set.polygons_with_holes(std::back_inserter(polygons));

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {

        ribbon.lines().emplace_back(polygon.outer_boundary());

        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
            ribbon.lines().emplace_back(hole);
        }
    }
}

void PenStroke::drawOutline(svg::Path& path) const {
    for (const LineConstruct& medianLine : _medrib) {

        std::vector<PolyConvex> polyConvexVector;
        // we populate polyConvex
        const std::size_t pointCount = medianLine.getMedianList().points().size();
        const std::size_t beginIndex = polyConvexVector.size();
        for (std::size_t pointIndex = 1; pointIndex < pointCount; ++pointIndex) {
            const std::size_t polyConvexIndex = polyConvexVector.size();
            const Point_2& sourcePoint = medianLine.getMedian(pointIndex - 1);
            const Point_2& targetPoint = medianLine.getMedian(pointIndex);
            polyConvexVector.emplace_back(
                sourcePoint, targetPoint, polyConvexIndex,
                PolygonTools::makeTrapeze(sourcePoint, targetPoint,
                                          medianLine.getBorder(pointIndex - 1),
                                          medianLine.getBorder(pointIndex)));
        }
        const std::size_t lastIndex = polyConvexVector.size() - 1;

        PolyConvex::connect(beginIndex, polyConvexVector);
        {
            const PolyConvex& firstPolyConvex = polyConvexVector.front();
            agg::Arc arc(medianLine.getMedian(0), medianLine.getBorder(0) / kHalfWidthScale,
                         firstPolyConvex._geometry.vertex(0), firstPolyConvex._geometry.vertex(1));
            polyConvexVector.emplace_back();
            polyConvexVector.back()._geometry.insert(
                polyConvexVector.back()._geometry.vertices_end(), arc.getPoints().begin(),
                arc.getPoints().end());
        }
        {
            const PolyConvex& lastPolyConvex = polyConvexVector.at(lastIndex);
            agg::Arc arc(medianLine.getMedian(pointCount - 1),
                         medianLine.getBorder(pointCount - 1) / kHalfWidthScale,
                         lastPolyConvex._geometry.vertex(2), lastPolyConvex._geometry.vertex(3));
            polyConvexVector.emplace_back();
            polyConvexVector.back()._geometry.insert(
                polyConvexVector.back()._geometry.vertices_end(), arc.getPoints().begin(),
                arc.getPoints().end());
        }

        Ribbon ribbon;
        createUnion(ribbon, polyConvexVector);
        ribbon.simplify(kSimplifyTolerance);
        drawRibbonStroke(path, ribbon);
    }
}

void PenStroke::createStroke(const Polyline& polyline) {

    if (polyline.points().size() <= 1UL) {
        std::cout << "warning : polyline with size: " << polyline.points().size() << '\n';
        return;
    }
    const Point_2& origin = polyline.points().front();
    const Point_2& destination = polyline.points().back();

    _medrib.emplace_back();
    LineConstruct& medianLine = _medrib.back();
    for (std::size_t pointIndex = 1; pointIndex < polyline.points().size(); ++pointIndex) {

        CGAL::Vector_2<Kernel> segmentVector =
            polyline.points().at(pointIndex) - polyline.points().at(pointIndex - 1);

        const double length = sqrt(CGAL::to_double(segmentVector.squared_length()));
        if (length > 0) {
            CGAL::Vector_2<Kernel> unitVector = segmentVector / length;
            const CGAL::Vector_2<Kernel> normalVector = unitVector.perpendicular(CGAL::LEFT_TURN);
            unitVector = unitVector * _config.resolution();

            addPoint(medianLine, polyline.points().at(pointIndex - 1), origin, destination, +1,
                     normalVector);
            for (int32_t vertexIndex = 1; vertexIndex < length / _config.resolution();
                 ++vertexIndex) {
                addPoint(medianLine,
                         barycentre(polyline.points().at(pointIndex - 1), vertexIndex,
                                    unitVector),
                         origin, destination, +1, normalVector);
            }
            if (pointIndex + 1 == polyline.points().size()) {

                if (polyline.isClosed() or
                    polyline.points().front() == polyline.points().back()) {

                    medianLine.setClosed();
                } else {
                    addPoint(medianLine, polyline.points().at(pointIndex), origin, destination,
                             +1, normalVector);
                }
            }
        }
    }
}

} /* namespace laby */

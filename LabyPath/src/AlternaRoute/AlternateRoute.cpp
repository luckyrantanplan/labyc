/*
 * AlternateRoute.cpp
 *
 *  Created on: Aug 21, 2018
 *      Author: florian
 */

#include "AlternateRoute.h"
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Bbox_2.h>
#include <CGAL/Intersections_2/Line_2_Line_2.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/enum.h>
#include <CGAL/number_utils.h>
#include <algorithm>
#include <boost/variant/get.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../Anisotrop/Routing.h"
#include "../GridIndex.h"
#include "../OrientedRibbon.h"
#include "../PolyConvex.h"
#include "../Rendering/GraphicRendering.h"
#include "../SVGParser/Loader.h"
#include "../VoronoiMedialSkeleton.h"
#include "../basic/SimplifyLines.h"
#include "../flatteningOverlap/PathRendering.h"
#include "AlternaRoute/StrokeArrangement.h"
#include "GeomData.h"
#include "Polyline.h"
#include "Ribbon.h"
#include "basic/RangeHelper.h"
#include "protoc/AllConfig.pb.h"

namespace laby {

namespace {

constexpr double kRibbonThickness = 0.3;

using ColorPolyConvexMap = std::unordered_map<int32_t, std::vector<std::size_t>>;

auto buildColorPolyConvexMap(
    const alter::ArrTrapeze::Vertex& vertex,
    const std::unordered_map<const alter::SegmentTrapezeInfo2*, std::size_t>& curvePlcMap)
    -> ColorPolyConvexMap {
    ColorPolyConvexMap colorPlcMap;
    for (const alter::ArrTrapeze::Halfedge& halfedge :
         RangeHelper::make(vertex.incident_halfedges())) {
        const alter::SegmentTrapezeInfo2& curve = halfedge.curve();
        std::vector<std::size_t>& plcVect =
            colorPlcMap.try_emplace(curve.data().direction()).first->second;
        plcVect.emplace_back(curvePlcMap.at(&curve));
    }
    return colorPlcMap;
}

auto collectOrphanPolyConvexes(const ColorPolyConvexMap& colorPlcMap)
    -> std::pair<std::vector<std::size_t>, bool> {
    std::vector<std::size_t> orphan;
    bool onlyOrphan = true;
    for (const auto& item : colorPlcMap) {
        const std::vector<std::size_t>& vect = item.second;
        if (vect.size() == 1) {
            orphan.emplace_back(vect.front());
        } else {
            onlyOrphan = false;
        }
    }
    return {orphan, onlyOrphan};
}

void connectSharedColorPolyConvexes(const ColorPolyConvexMap& colorPlcMap,
                                    const std::vector<std::size_t>& orphan,
                                    std::vector<PolyConvex>& polyConvexVect,
                                    const Kernel::Point_2& connectionPoint) {
    for (const auto& item : colorPlcMap) {
        const std::vector<std::size_t>& vect = item.second;
        if (vect.size() > 1) {
            for (std::size_t i = 1; i < vect.size(); ++i) {
                PolyConvex::connect(vect.at(i - 1), vect.at(i), polyConvexVect, connectionPoint);
            }
            for (const std::size_t orphanIndex : orphan) {
                PolyConvex::connect(vect.at(0), orphanIndex, polyConvexVect, connectionPoint);
            }
        }
    }
}

void connectOrphanPolyConvexes(const std::vector<std::size_t>& orphan,
                               std::vector<PolyConvex>& polyConvexVect,
                               const Kernel::Point_2& connectionPoint) {
    for (std::size_t i = 1; i < orphan.size(); ++i) {
        PolyConvex::connect(orphan.at(0), orphan.at(i), polyConvexVect, connectionPoint);
    }
}

} // namespace

auto AlternateRoute::OffsetPair::simplify(std::vector<AlternateRoute::OffsetPair>& list,
                                          const double& distance)
    -> std::vector<AlternateRoute::OffsetPair> {

    if (list.size() > 2) {
        SimplifyLines::LineStringIndexed lineString;
        for (std::size_t i = 0; i < list.size(); ++i) {
            const OffsetPair& pair = list.at(i);
            lineString.emplace_back(IndexedPoint::fromCoordinates(
                {CGAL::to_double(pair.offset().x()), CGAL::to_double(pair.offset().y())}, i));
        }
        SimplifyLines::LineStringIndexed const simpleLine =
            SimplifyLines::decimateIndex(lineString, distance);

        std::vector<AlternateRoute::OffsetPair> result;
        result.reserve(simpleLine.size());
        for (const IndexedPoint& offset : simpleLine) {

            result.emplace_back(list.at(offset.index()));
        }
        return result;
    }
    return list;
}

auto AlternateRoute::pruneArrangement(const laby::Arrangement_2& arrangement) const
    -> laby::Arrangement_2 {
    // remove inner  antenna
    std::vector<Segment_info_2> result2;
    uint32_t counter = 0;
    for (Edge_const_iterator eit = arrangement.edges_begin(); eit != arrangement.edges_end();
         ++eit) {
        if (counter < _config.pruning()) {
            result2.emplace_back(eit->curve());
        } else {
            counter = 0;
        }
        ++counter;
    }
    std::cout << "start random remove edges\n";
    laby::Arrangement_2 arr2;
    CGAL::insert(arr2, result2.begin(), result2.end());
    std::cout << "random edges are remove\n";
    return arr2;
}

auto AlternateRoute::removeAntenna(const laby::Arrangement_2& arrangement) -> laby::Arrangement_2 {
    // remove inner  antenna
    std::vector<Segment_info_2> result2;
    for (Edge_const_iterator eit = arrangement.edges_begin(); eit != arrangement.edges_end();
         ++eit) {
        if (&*eit->face() != &*eit->twin()->face()) {
            result2.emplace_back(eit->curve());
        }
    }
    std::cout << "start remove antenna edges\n";
    laby::Arrangement_2 arr2;
    CGAL::insert(arr2, result2.begin(), result2.end());
    std::cout << "  antenna edges are remove\n";
    return arr2;
}

void AlternateRoute::addPoint(std::vector<OffsetPair>& offsets, const OffsetEndpoints& endpoints) {
    offsets.emplace_back();
    OffsetPair& offsetPair = offsets.back();

    offsetPair.setOrigin(endpoints.originPoint);
    offsetPair.setOffset(
        CGAL::barycenter(offsetPair.origin(), _config.thicknesspercent(), endpoints.targetPoint));
    auto vec = offsetPair.offset() - offsetPair.origin();
    if (vec.squared_length() > _sqMaxThickness) {
        auto scaledVector =
            vec * (_config.maxthickness() / sqrt(CGAL::to_double(vec.squared_length())));
        offsetPair.setOffset(offsetPair.origin() + scaledVector);
    } else if (vec.squared_length() < _sqMinThickness) {
        auto scaledVector =
            vec * (_config.minthickness() / sqrt(CGAL::to_double(vec.squared_length())));
        offsetPair.setOffset(offsetPair.origin() + scaledVector);
    }
}

auto AlternateRoute::coupleList(const Halfedge& halfedge,
                                int32_t direction) -> std::vector<AlternateRoute::OffsetPair> {
    std::vector<OffsetPair> left;
    auto halfedgeIterator = halfedge.ccb();
    const auto ending = halfedge.ccb();
    ++halfedgeIterator;
    if (halfedgeIterator->curve().data().direction() != direction + 1) {
        std::cout << " ite->curve().data().direction() "
                  << halfedgeIterator->curve().data().direction() << '\n';
        std::cout << " ite->curve()  " << halfedgeIterator->curve() << '\n';
        std::cout << "direction of ccb face is not voronoi ??\n";

        for (const Halfedge& neighborHalfedge : RangeHelper::make(halfedge.ccb())) {
            std::cout << neighborHalfedge.source()->point() << ","
                      << neighborHalfedge.target()->point() << " "
                      << neighborHalfedge.curve().data().direction() << '\n';
        }
        std::cout << '\n';
        throw std::runtime_error("direction of ccb face is not voronoi");
    }

    Kernel::Line_2 const supportLine(halfedge.curve());

    {
        addPoint(left, {halfedgeIterator->source()->point(), halfedgeIterator->target()->point()});
        OffsetPair& pair = left.back();
        Kernel::Vector_2 const vect = pair.offset() - supportLine.projection(pair.offset());
        pair.setOffset(pair.origin() + vect);
    }

    ++halfedgeIterator;
    for (; halfedgeIterator != ending; ++halfedgeIterator) {
        if (halfedgeIterator->target() != halfedge.source()) {
            addPoint(left, {supportLine.projection(halfedgeIterator->target()->point()),
                            halfedgeIterator->target()->point()});
        } else {

            addPoint(left,
                     {halfedgeIterator->target()->point(), halfedgeIterator->source()->point()});
            OffsetPair& pair = left.back();
            Kernel::Vector_2 const vect = pair.offset() - supportLine.projection(pair.offset());
            pair.setOffset(pair.origin() + vect);
            break;
        }
    }
    return OffsetPair::simplify(left, _config.simplifydist());
}

void AlternateRoute::addTriplet(alter::OffsetTriplet& triplet, const OffsetPair& offsetPair,
                                const Kernel::Point_2& lineStart, const Kernel::Point_2& lineEnd) {

    triplet.setOrigin(offsetPair.origin());
    triplet.setOffset1(offsetPair.offset());
    Kernel::Line_2 const firstLine(lineStart, lineEnd);
    Kernel::Line_2 const secondLine(triplet.origin(), triplet.offset1());
    auto variant2 = CGAL::intersection(firstLine, secondLine);
    if (const Kernel::Point_2* intersectionPoint = boost::get<Kernel::Point_2>(&*variant2)) {
        triplet.setOffset2(*intersectionPoint);
    }
}

auto AlternateRoute::voronoiArr(const Arrangement_2& arrangement, int32_t direction,
                                const CGAL::Bbox_2& viewBox,
                                const Ribbon& ribLimit) -> Arrangement_2 {
    std::vector<Segment_info_2> segments;
    for (const Halfedge& eit :
         RangeHelper::make(arrangement.edges_begin(), arrangement.edges_end())) {
        const Segment_info_2& curve = eit.curve();
        if (curve.data().direction() == direction) {
            segments.emplace_back(curve);
        }
    }
    Arrangement_2 arr2;
    CGAL::insert(arr2, segments.begin(), segments.end());

    Ribbon ribContour = Ribbon::createRibbonOfEdge(arr2, _config.simplifydist());
    ribContour.setFillColor(direction);
    Ribbon ribContourFramed = ribContour;
    ribContourFramed.lines().emplace_back();
    Polyline& framePolyline = ribContourFramed.lines().back();
    const double viewBoxLengthX = viewBox.xmax() - viewBox.xmin();
    const double viewBoxLengthY = viewBox.ymax() - viewBox.ymin();
    framePolyline.points.emplace_back(viewBox.xmin() - viewBoxLengthX,
                                      viewBox.ymin() - viewBoxLengthY);
    framePolyline.points.emplace_back(viewBox.xmax() + viewBoxLengthX,
                                      viewBox.ymin() - viewBoxLengthY);
    framePolyline.points.emplace_back(viewBox.xmax() + viewBoxLengthX,
                                      viewBox.ymax() + viewBoxLengthY);
    framePolyline.points.emplace_back(viewBox.xmin() - viewBoxLengthX,
                                      viewBox.ymax() + viewBoxLengthY);
    framePolyline.closed = true;

    CGAL::Bbox_2 const frameBox(viewBox.xmin() - 1, viewBox.ymin() - 1, viewBox.xmax() + 1,
                                viewBox.ymax() + 1);
    VoronoiMedialSkeleton const vor(ribContourFramed, frameBox);

    std::cout << "vor.get_vor_segments().size() " << vor.get_vor_segments().size() << '\n';

    Arrangement_2 arrDir = vor.getSimpleArr(ribContour, ribLimit);

    return arrDir;
}

auto AlternateRoute::createTripletList(const Halfedge& halfedge,
                                       int32_t direction) -> std::vector<alter::OffsetTriplet> {
    std::vector<OffsetPair> left = coupleList(halfedge, direction);
    std::vector<OffsetPair> right = coupleList(*halfedge.twin(), direction);

    std::reverse(right.begin(), right.end());

    std::vector<alter::OffsetTriplet> tripletList;
    {
        std::size_t leftIndex = 0;
        std::size_t rightIndex = 0;

        while (leftIndex < left.size() and rightIndex < right.size()) {
            tripletList.emplace_back();
            alter::OffsetTriplet& triplet = tripletList.back();

            Kernel::Comparison_result const predicat = CGAL::compare_distance_to_point(
                halfedge.target()->point(), left.at(leftIndex).origin(),
                right.at(rightIndex).origin());
            switch (predicat) {
            case CGAL::Comparison_result::EQUAL: {
                triplet.setOrigin(left.at(leftIndex).origin());
                triplet.setOffset1(left.at(leftIndex).offset());
                triplet.setOffset2(right.at(rightIndex).offset());
                ++leftIndex;
                ++rightIndex;

                break;
            }
            case CGAL::Comparison_result::SMALLER: {
                addTriplet(triplet, left.at(leftIndex), right.at(rightIndex - 1).offset(),
                           right.at(rightIndex).offset());

                ++leftIndex;
                break;
            }
            case CGAL::Comparison_result::LARGER: {
                addTriplet(triplet, right.at(rightIndex), left.at(leftIndex - 1).offset(),
                           left.at(leftIndex).offset());
                triplet.swapOffsets();
                ++rightIndex;
                break;
            }
            }
        }
    }

    return tripletList;
}

void AlternateRoute::ribToTrapeze(const Ribbon& rib,
                                  std::vector<alter::SegmentTrapezeInfo2>& trapezeVect,
                                  const Arrangement_2& arrangement, const CGAL::Bbox_2& viewBox,
                                  const Ribbon& ribLimit) {

    int32_t const direction = rib.strokeColor();
    std::cout << "start voronoi_arr \n";

    Arrangement_2 arrDir = voronoiArr(arrangement, direction, viewBox, ribLimit);
    std::cout << "end voronoi_arr \n";
    for (const Halfedge& halfedge : RangeHelper::make(arrDir.edges_begin(), arrDir.edges_end())) {
        // here (direction + 1) indicate all the segments generated by voronoi
        if (halfedge.curve().data().direction() == direction &&
            (++halfedge.ccb())->curve().data().direction() == direction + 1) {
            std::vector<alter::OffsetTriplet> tripletList = createTripletList(halfedge, direction);
            for (std::size_t i = 1; i < tripletList.size(); ++i) {
                const alter::OffsetTriplet& firstTriplet = tripletList.at(i - 1);
                const alter::OffsetTriplet& secondTriplet = tripletList.at(i);
                trapezeVect.emplace_back(
                    Kernel::Segment_2(firstTriplet.origin(), secondTriplet.origin()),
                    alter::TrapezeEdgeInfo(alter::TrapezeEdgeInfo::SourceTriplet{firstTriplet},
                                           alter::TrapezeEdgeInfo::TargetTriplet{secondTriplet},
                                           direction));
            }
        }
    }
}

void AlternateRoute::populateTrapeze(const GridIndex& gridIndex, const std::vector<Ribbon>& ribList,
                                     const CGAL::Bbox_2& viewBox,
                                     std::vector<alter::SegmentTrapezeInfo2>& trapezeVect) {
    const Ribbon& ribLimit = gridIndex.limit(ribList);
    const Ribbon& ribCircular = ribList.at(gridIndex.circularIndex());
    const Ribbon& ribRadial = ribList.at(gridIndex.radialIndex());
    Arrangement_2 arr = gridIndex.getArr(ribList);
    // remove inner antenna
    arr = pruneArrangement(arr);
    arr = removeAntenna(arr);

    ribToTrapeze(ribCircular, trapezeVect, arr, viewBox, ribLimit);
    ribToTrapeze(ribRadial, trapezeVect, arr, viewBox, ribLimit);
}

void AlternateRoute::copyStrokeColorToFillColor(std::vector<Ribbon>& ribList) {
    for (Ribbon& rib : ribList) {
        // The skeleton grid info is on stroke color. Move it to fill color so
        // arrangement construction keeps circular/radial metadata separate.
        rib.setFillColor(rib.strokeColor());
    }
}

auto AlternateRoute::buildTrapezeVector(
    const std::unordered_map<uint32_t, GridIndex>& mapOfGrids, const std::vector<Ribbon>& ribList,
    const CGAL::Bbox_2& viewBox) -> std::vector<alter::SegmentTrapezeInfo2> {
    std::vector<alter::SegmentTrapezeInfo2> trapezeVect;
    for (const auto& pair : mapOfGrids) {
        populateTrapeze(pair.second, ribList, viewBox, trapezeVect);
    }
    return trapezeVect;
}

auto AlternateRoute::buildPolyConvexVector(
    const alter::ArrTrapeze& arrTrapeze,
    std::unordered_map<const alter::SegmentTrapezeInfo2*, std::size_t>& curvePlcMap)
    -> std::vector<PolyConvex> {
    std::vector<PolyConvex> polyConvexVect;
    for (const alter::ArrTrapeze::Halfedge& edgeIt :
         RangeHelper::make(arrTrapeze.edges_begin(), arrTrapeze.edges_end())) {
        const alter::SegmentTrapezeInfo2& curve = edgeIt.curve();
        const std::size_t index = polyConvexVect.size();
        polyConvexVect.emplace_back(edgeIt.source()->point(), edgeIt.target()->point(), index,
                                    curve.data().getGeometry(curve));
        curvePlcMap.emplace(&curve, polyConvexVect.back()._id);
    }
    return polyConvexVect;
}

void AlternateRoute::connectPolyConvexes(
    const alter::ArrTrapeze& arrTrapeze,
    const std::unordered_map<const alter::SegmentTrapezeInfo2*, std::size_t>& curvePlcMap,
    std::vector<PolyConvex>& polyConvexVect) {
    for (const alter::ArrTrapeze::Vertex& vertex :
         RangeHelper::make(arrTrapeze.vertices_begin(), arrTrapeze.vertices_end())) {
        const ColorPolyConvexMap colorPlcMap = buildColorPolyConvexMap(vertex, curvePlcMap);
        const auto [orphan, onlyOrphan] = collectOrphanPolyConvexes(colorPlcMap);

        if (!onlyOrphan) {
            connectSharedColorPolyConvexes(colorPlcMap, orphan, polyConvexVect, vertex.point());
        } else {
            connectOrphanPolyConvexes(orphan, polyConvexVect, vertex.point());
        }
    }
}

AlternateRoute::AlternateRoute(proto::AlternateRouting config, const proto::Filepaths& filepaths)
    : _config(std::move(config)), _sqMaxThickness(_config.maxthickness() * _config.maxthickness()),
      _sqMinThickness(_config.minthickness() * _config.minthickness()) {

    svgp::Loader load(filepaths.inputfile());
    std::vector<Ribbon>& ribList = load.ribList();
    copyStrokeColorToFillColor(ribList);
    std::unordered_map<uint32_t, GridIndex> const mapOfGrids = GridIndex::getIndexMap(ribList);
    std::vector<alter::SegmentTrapezeInfo2> trapezeVect =
        buildTrapezeVector(mapOfGrids, ribList, load.viewBox());
    std::cout << "trapezeVect filled\n";
    alter::ArrTrapeze arrTrapeze;
    CGAL::insert(arrTrapeze, trapezeVect.begin(), trapezeVect.end());

    std::cout << "arrTrapeze filled\n";
    std::unordered_map<const alter::SegmentTrapezeInfo2*, std::size_t> curvePlcMap;

    std::vector<PolyConvex> polyConvexVect = buildPolyConvexVector(arrTrapeze, curvePlcMap);

    std::cout << "polyconvex populated\n";

    connectPolyConvexes(arrTrapeze, curvePlcMap, polyConvexVect);

    std::cout << "polyconvex connected\n";

    aniso::Routing::connectMaze(polyConvexVect);

    std::cout << "maze connected\n";

    OrientedRibbon orientedRibbon;
    PathRendering::pathRender(polyConvexVect, orientedRibbon);

    GraphicRendering::printRibbonSvg(load.viewBox(), filepaths.outputfile(), kRibbonThickness,
                                     orientedRibbon.getResult());
}

} /* namespace laby */

/*
 * Placement.cpp
 *
 *  Created on: Jun 7, 2018
 *      Author: florian
 */

#include "Placement.h"

#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arrangement_2/Arrangement_2_iterators.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <functional>
#include <iostream>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../GridIndex.h"
#include "../Rendering/GraphicRendering.h"
#include "../Ribbon.h"
#include "../SVGParser/Loader.h"
#include "../Smoothing.h"
#include "../basic/LinearGradient.h"
#include "Cell.h"
#include "GeomData.h"
#include "Net.h"
#include "OrientedRibbon.h"
#include "PolyConvex.h"
#include "PolyVertex.h"
#include "Polyline.h"
#include "Routing.h"
#include "SpatialIndex.h"

#include "../flatteningOverlap/PathRendering.h"
#include "basic/RangeHelper.h"
#include "protoc/AllConfig.pb.h"

namespace laby::aniso {

namespace {

constexpr double kOutputThicknessDivisor = 3.0;
constexpr double kSimplifyTolerance = 0.1;

} // namespace

Placement::Placement(proto::Placement config, const proto::Filepaths& filepaths)
    : _config(std::move(config)) {
    svgp::Loader load(filepaths.inputfile());

    std::vector<Ribbon>& ribList = load.ribList();
    for (Ribbon& rib : ribList) {
        // the skeleton grid info is on stroke color
        // put it on fill color, in order to make the arrangement with different circular, radial
        // info
        rib.setFillColor(rib.strokeColor());
    }
    std::unordered_map<uint32_t, GridIndex> const mapOfGrids = GridIndex::getIndexMap(ribList);

    std::vector<PolyConvex> polyConvexList;

    for (const auto& pair : mapOfGrids) {
        const GridIndex& gridIndex = pair.second;
        Arrangement_2 arr =
            gridIndex.getArr(ribList); // scope of variable : cell only take a reference on it.
        Cell cell(_config.cell(), arr, gridIndex.limit(ribList));

        laby::aniso::Routing routing = createRoute(cell);
        refinePath(cell, routing.polyConvexList(), polyConvexList);
    }
    Routing::connectMaze(polyConvexList);
    OrientedRibbon oRibbon;
    PathRendering::pathRender(polyConvexList, oRibbon);

    GraphicRendering::printRibbonSvg(load.viewBox(), filepaths.outputfile(),
                                     _config.minimal_thickness() / kOutputThicknessDivisor,
                                     oRibbon.getResult());
}

auto Placement::createRoute(Cell& cell) -> Routing {

    Arrangement_2& arr = cell.arr();

    for (Halfedge& halfedge : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        halfedge.curve().data().clearAllPath();
    }

    std::vector<Net>& nets = cell.nets();

    Routing routing(arr, _config.routing()); // seed

    for (Net& net : nets) {
        routing.findRoute(net);
    }

    std::deque<Pin> queue;

    for (Vertex* vertex : cell.vertices()) {
        queue.emplace_back(*vertex, _config.initial_thickness());
    }
    while (!queue.empty()) {
        bool sourceVertexOK = false;
        for (std::size_t i = 0;
             i < std::min(_config.max_routing_attempt(), cell.randomVertices().size()); ++i) {
            auto iteTarget = cell.selectRandomVertex();
            Vertex& vertex2 = **iteTarget;
            if (&vertex2 != &queue.front().vertex()) {

                Pin const pinTarget{vertex2,
                                    std::max(queue.front().thickness() / _config.decrement_factor(),
                                             _config.minimal_thickness())};
                Net netCandidate(Net::SourcePin{queue.front()}, Net::TargetPin{pinTarget},
                                 static_cast<int32_t>(nets.size()));

                if (routing.findRoute(netCandidate)) {

                    nets.emplace_back(netCandidate);
                    std::cout << "commitNewPath" << nets.size() << '\n';
                    queue.emplace_back(pinTarget);
                    cell.removeRandomPoint(iteTarget);
                    sourceVertexOK = true;
                    break;
                }
            }
        }
        if (!sourceVertexOK) {
            queue.pop_front();
            std::cout << "queue size : " << queue.size() << '\n';
        }
    }
    statistics(arr);
    return routing;
}

void Placement::statistics(const Arrangement_2& arr) {

    std::size_t totalCongestion = 0;
    for (const Halfedge& halfedge : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        totalCongestion += static_cast<std::size_t>(halfedge.curve().data().congestion());
    }
    std::cout << "seed  " << _config.routing().seed() << " total_congestion " << totalCongestion
              << " number of edge " << arr.number_of_edges() << '\n';
}

auto Placement::crossOtherNet(const Vertex& vertex, int32_t netId) -> bool {
    std::vector<std::reference_wrapper<const Halfedge>> incidentHalfedges;
    incidentHalfedges.reserve(static_cast<std::size_t>(vertex.degree()));
    for (const Halfedge& halfedge : RangeHelper::make(vertex.incident_halfedges())) {
        incidentHalfedges.emplace_back(halfedge);
    }

    return std::any_of(incidentHalfedges.begin(), incidentHalfedges.end(),
                       [netId](const std::reference_wrapper<const Halfedge> halfedgeRef) {
                           const Halfedge& halfedge = halfedgeRef.get();
                           return halfedge.curve().data().congestion() > 0 &&
                                  halfedge.curve().data().getNet() != netId;
                       });
}

auto Placement::explodeGraph(const std::vector<PolyConvex>& initialConvex,
                             Cell& cell) -> std::vector<PolyVertex> {
    std::vector<PolyVertex> ribbonTemp;

    for (Net& net : cell.nets()) {

        PolyVertex polytemp(net.id());
        for (std::size_t const polyConvexId : net.path()) {
            const PolyConvex& polyConvex = initialConvex.at(polyConvexId);
            Vertex& vertex = *polyConvex._supportHe->source();
            polytemp.vertexList().emplace_back(&vertex);
            if (crossOtherNet(*polyConvex._supportHe->source(), net.id())) {
                ribbonTemp.emplace_back(polytemp);
                polytemp.vertexList().front() = polytemp.vertexList().back();
                polytemp.vertexList().resize(1);
            }
        }
        polytemp.vertexList().emplace_back(&net.source().vertex());
        ribbonTemp.emplace_back(polytemp);
    }

    return ribbonTemp;
}

auto Placement::refinePath(Cell& cell, const std::vector<PolyConvex>& initialConvex,
                           std::vector<PolyConvex>& polyConvexList) -> std::vector<PolyConvex> {
    std::vector<PolyVertex> const ribbonTemp = explodeGraph(initialConvex, cell);

    std::unordered_map<int32_t, Net*> netMap;
    for (Net& net : cell.nets()) {
        netMap.try_emplace(net.id(), &net);
    }

    Ribbon ribbon;
    int32_t netId = -1;
    for (const PolyVertex& plv : ribbonTemp) {

        Polyline polyline = plv.polyline();

        polyline.simplify(kSimplifyTolerance);

        polyline = Smoothing::getCurveSmoothingChaikin(polyline, _config.smoothing_tension(),
                                                       _config.smoothing_iteration());

        polyline.simplify(kSimplifyTolerance);
        if (polyline.id() != netId) {
            netId = polyline.id();
            ribbon.lines().emplace_back(netId);
            ribbon.lines().back().points().emplace_back(polyline.points().front());
        }
        std::vector<Point_2>& points = ribbon.lines().back().points();
        for (std::size_t pointIndex = 1; pointIndex < polyline.points().size(); ++pointIndex) {
            points.emplace_back(polyline.points().at(pointIndex));
        }
    }

    SpatialIndex spatialIndex(polyConvexList);
    for (Polyline& poly : ribbon.lines()) {
        Net& net = *netMap.at(poly.id());
        basic::LinearGradient lgrad = net.gradient();
        std::size_t const begin = polyConvexList.size();
        for (std::size_t i = 1; i < poly.points().size(); ++i) {
            polyConvexList.emplace_back(
                PolyConvexEndpoints{poly.points().at(i - 1), poly.points().at(i)},
                polyConvexList.size(), lgrad);
        }
        PolyConvex::connect(begin, polyConvexList);
        net.target().setPolyConvexIndex(begin);
        net.source().setPolyConvexIndex(polyConvexList.size() - 1);
        for (std::size_t j = begin; j < polyConvexList.size(); ++j) {
            spatialIndex.insert(polyConvexList.at(j));
        }
    }
    Routing::connectTwoPinPath(cell.nets(), spatialIndex, polyConvexList);

    return polyConvexList;
}

} // namespace laby::aniso

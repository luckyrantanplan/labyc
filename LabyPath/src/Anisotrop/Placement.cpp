/*
 * Placement.cpp
 *
 *  Created on: Jun 7, 2018
 *      Author: florian
 */

#include "Placement.h"

#include <CGAL/Arr_dcel_base.h>
#include <CGAL/Arr_geometry_traits/Curve_data_aux.h>
#include <CGAL/Arrangement_2/Arrangement_2_iterators.h>
#include <CGAL/Arrangement_on_surface_2.h>
#include <algorithm>
#include <cstddef>
#include <deque>
#include <iostream>
#include <iterator>
#include <unordered_map>

#include "../basic/LinearGradient.h"
#include "../basic/PairInteger.h"
#include "../Ribbon.h"
#include "../Smoothing.h"
#include "Cell.h"
#include "Net.h"
#include "Routing.h"
#include "SpatialIndex.h"
#include "../SVGParser/Loader.h"
#include "../GridIndex.h"
#include "../Rendering/GraphicRendering.h"

#include "../flatteningOverlap/PathRendering.h"

namespace laby {
namespace aniso {

Placement::Placement(const proto::Placement& config, const proto::Filepaths& filepaths) :
        _config(config) {
    svgp::Loader load(filepaths.inputfile());

    std::vector<Ribbon> &ribList = load.ribList();
    for (std::size_t i = 0; i < ribList.size(); ++i) {
        // the skeleton grid info is on stroke color
        // put it on fill color, in order to make the arrangement with different circular, radial info
        Ribbon& rib = ribList.at(i);
        rib.set_fill_color(rib.strokeColor());
    }
    std::unordered_map<uint32_t, GridIndex> mapOfGrids = GridIndex::getIndexMap(ribList);

    std::vector<PolyConvex> polyConvexList;

    for (const auto & pair : mapOfGrids) {
        const GridIndex& gridIndex = pair.second;
        Arrangement_2 arr = gridIndex.getArr(ribList); // scope of variable : cell only take a reference on it.
        Cell cell(_config.cell(), arr,gridIndex.limit(ribList));

        laby::aniso::Routing routing = create_route(cell);
        refine_path(cell, routing.polyConvexList(), polyConvexList);

    }
    Routing::connectMaze(polyConvexList);
    OrientedRibbon oRibbon;
    PathRendering::pathRender(polyConvexList, oRibbon);

    GraphicRendering::printRibbonSvg(load.viewBox(), filepaths.outputfile(), _config.minimal_thickness() / 3., oRibbon.getResult());

}

Routing Placement::create_route(Cell& cell) {

    Arrangement_2& arr = cell.arr();

    for (Halfedge& he : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        he.curve().data().clearAllPath();
    }

    std::vector<Net>& nets = cell.nets();

    Routing routing(arr, _config.routing()); // seed

    for (Net& n : nets) {
        routing.findRoute(n);
    }

    std::deque<Pin> queue;

    for (Vertex* vertex : cell.vertices()) {
        queue.emplace_back(*vertex, _config.initial_thickness());
    }
    while (!queue.empty()) {
        bool sourceVertexOK = false;
        for (std::size_t i = 0; i < std::min(_config.max_routing_attempt(), cell.randomVertex.size()); ++i) {
            std::vector<Vertex*>::iterator ite_target = cell.selectRandomVertex();
            Vertex& vertex2 = **ite_target;
            if (&vertex2 != &queue.front().vertex()) {

                Pin pin_target { vertex2, std::max(queue.front().thickness() / _config.decrement_factor(), _config.minimal_thickness()) };
                Net n(queue.front(), pin_target, static_cast<int32_t>(nets.size()));

                if (routing.findRoute(n)) {

                    nets.emplace_back(n);
                    std::cout << "commitNewPath" << nets.size() << std::endl;
                    queue.emplace_back(pin_target);
                    cell.removeRandomPoint(ite_target);
                    sourceVertexOK = true;
                    break;
                }
            }

        }
        if (!sourceVertexOK) {
            queue.pop_front();
            std::cout << "queue size : " << queue.size() << std::endl;
        }
    }
    statistics(arr);
    return routing;
}

void Placement::statistics(const Arrangement_2& arr) {

    std::size_t total_congestion = 0;
    for (const Halfedge& he : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        total_congestion += static_cast<std::size_t>(he.curve().data().congestion());
    }
    std::cout << "seed  " << _config.routing().seed() << " total_congestion " << total_congestion << " number of edge " << arr.number_of_edges() << std::endl;

}

bool Placement::crossOtherNet(const Vertex& v, int32_t netId) {

    for (const Halfedge& he : RangeHelper::make(v.incident_halfedges())) {
        if (he.curve().data().congestion() > 0 and he.curve().data().get_net() != netId) {
            return true;

        }
    }
    return false;
}

std::vector<PolyVertex> Placement::explodeGraph(const std::vector<PolyConvex>& initialConvex, Cell& cell) {
    std::vector<PolyVertex> ribbonTemp;

    for (Net& net : cell.nets()) {

        PolyVertex polytemp(net.id());
        for (std::size_t i : net.path()) {
            const PolyConvex& pc = initialConvex.at(i);
            Vertex& v = *pc._supportHe->source();
            polytemp.vertexList().emplace_back(&v);
            if (crossOtherNet(*pc._supportHe->source(), net.id())) {
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

std::vector<PolyConvex> Placement::refine_path(Cell& cell, const std::vector<PolyConvex>& initialConvex, std::vector<PolyConvex>& polyConvexList) {
    std::vector<PolyVertex> ribbonTemp = explodeGraph(initialConvex, cell);

    std::unordered_map<int32_t, Net*> netMap;
    for (Net& net : cell.nets()) {
        netMap.try_emplace(net.id(), &net);
    }

    Ribbon ribbon;
    int32_t net_id = -1;
    for (const PolyVertex& plv : ribbonTemp) {

        Polyline pl = plv.polyline();

        pl.simplify(0.1);

        pl = Smoothing::getCurveSmoothingChaikin(pl, _config.smoothing_tension(), _config.smoothing_iteration());

        pl.simplify(0.1);
        if (pl.id != net_id) {
            net_id = pl.id;
            ribbon.lines().emplace_back(net_id);
            ribbon.lines().back().points.emplace_back(pl.points.front());
        }
        std::vector<Point_2>& points = ribbon.lines().back().points;
        for (std::size_t i = 1; i < pl.points.size(); ++i) {
            points.emplace_back(pl.points.at(i));
        }
    }

    SpatialIndex si(polyConvexList);
    for (Polyline& poly : ribbon.lines()) {
        Net& net = *netMap.at(poly.id);
        basic::LinearGradient lgrad = net.gradient();
        std::size_t begin = polyConvexList.size();
        for (std::size_t i = 1; i < poly.points.size(); ++i) {
            polyConvexList.emplace_back(poly.points.at(i - 1), poly.points.at(i), polyConvexList.size(), lgrad);
        }
        PolyConvex::connect(begin, polyConvexList);
        net.target().setPolyConvexIndex(begin);
        net.source().setPolyConvexIndex(polyConvexList.size() - 1);
        for (std::size_t j = begin; j < polyConvexList.size(); ++j) {
            si.insert(polyConvexList.at(j));
        }
    }
    Routing::connectTwoPinPath(cell.nets(), si, polyConvexList);

    return polyConvexList;
}

} /* namespace aniso */
} /* namespace laby */

/*
 * AlternateRoute.cpp
 *
 *  Created on: Aug 21, 2018
 *      Author: florian
 */

#include "AlternateRoute.h"
#include "../VoronoiMedialSkeleton.h"
#include "../SVGParser/Loader.h"
#include "../PolyConvex.h"
#include "../basic/LinearGradient.h"
#include "../flatteningOverlap/PathRendering.h"
#include "../Rendering/GraphicRendering.h"
#include "../Anisotrop/Routing.h"
#include "../basic/SimplifyLines.h"
#include "../basic/Color.h"
#include "../GridIndex.h"

namespace laby {

std::vector<AlternateRoute::Offset_pair> AlternateRoute::Offset_pair::simplify(std::vector<AlternateRoute::Offset_pair>& list, const double& dist) {

    if (list.size() > 2) {
        SimplifyLines::LineStringIndexed lineString;
        for (std::size_t i = 0; i < list.size(); ++i) {
            const Offset_pair& pair = list.at(i);
            lineString.emplace_back(Indexed_Point(CGAL::to_double(pair.offset.x()), CGAL::to_double(pair.offset.y()), i));
        }
        SimplifyLines::LineStringIndexed simpleLine = SimplifyLines::decimateIndex(lineString, dist);

        std::vector<AlternateRoute::Offset_pair> result;
        result.reserve(simpleLine.size());
        for (const Indexed_Point& offset : simpleLine) {

            result.emplace_back(list.at(offset.index));
        }
        return result;
    }
    return list;

}

laby::Arrangement_2 AlternateRoute::prune_arrangement(const laby::Arrangement_2& arr) {
    // remove inner  antenna
    std::vector<Segment_info_2> result2;
    uint32_t counter = 0;
    for (Edge_const_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {

        if (counter < _config.pruning()) {
            result2.emplace_back(eit->curve());
        } else {
            counter = 0;
        }
        ++counter;

    }
    std::cout << "start random remove edges" << std::endl;
    laby::Arrangement_2 arr2;
    CGAL::insert(arr2, result2.begin(), result2.end());

    std::cout << "random edges are remove" << std::endl;
    return arr2;
}

laby::Arrangement_2 AlternateRoute::remove_antenna(const laby::Arrangement_2& arr) {
    // remove inner  antenna
    std::vector<Segment_info_2> result2;

    for (Edge_const_iterator eit = arr.edges_begin(); eit != arr.edges_end(); ++eit) {
        if (&*eit->face() != &*eit->twin()->face()) {

            result2.emplace_back(eit->curve());

        }
    }
    std::cout << "start remove antenna edges" << std::endl;
    laby::Arrangement_2 arr2;
    CGAL::insert(arr2, result2.begin(), result2.end());

    std::cout << "  antenna edges are remove" << std::endl;
    return arr2;
}

void AlternateRoute::add_point(std::vector<Offset_pair>& left, const Kernel::Point_2& o1, const Kernel::Point_2& o2) {
    left.emplace_back();
    Offset_pair& p = left.back();

    p.origin = o1;
    p.offset = CGAL::barycenter(p.origin, _config.thicknesspercent(), o2);
    auto vec = p.offset - p.origin;
    if (vec.squared_length() > sq_max_thickness) {
        auto v2 = vec * (_config.maxthickness() / sqrt(CGAL::to_double(vec.squared_length())));
        p.offset = p.origin + v2;
    } else if (vec.squared_length() < sq_min_thickness) {
        auto v2 = vec * (_config.minthickness() / sqrt(CGAL::to_double(vec.squared_length())));
        p.offset = p.origin + v2;
    }

}

std::vector<AlternateRoute::Offset_pair> AlternateRoute::couple_list(const Halfedge& he, int32_t direction) {
    std::vector<Offset_pair> left;
    auto ite = he.ccb();
    auto ending = he.ccb();
    ++ite;
    if (ite->curve().data().direction() != direction + 1) {
        std::cout << " ite->curve().data().direction() " << ite->curve().data().direction() << std::endl;
        std::cout << " ite->curve()  " << ite->curve() << std::endl;
        std::cout << "direction of ccb face is not voronoi ??" << std::endl;

        for (const Halfedge& he2 : RangeHelper::make(he.ccb())) {
            std::cout << he2.source()->point() << "," << he2.target()->point() << " " << he2.curve().data().direction() << std::endl;
            ;
        }
        std::cout << std::endl;
        exit(1);
    }

    Kernel::Line_2 l(he.curve());

    {
        add_point(left, ite->source()->point(), ite->target()->point());
        Offset_pair& pair = left.back();
        Kernel::Vector_2 vect = pair.offset - l.projection(pair.offset);
        pair.offset = pair.origin + vect;
    }

    ++ite;
    for (; ite != ending; ++ite) {
        if (ite->target() != he.source()) {
            add_point(left, l.projection(ite->target()->point()), ite->target()->point());
        } else {

            add_point(left, ite->target()->point(), ite->source()->point());
            Offset_pair& pair = left.back();
            Kernel::Vector_2 vect = pair.offset - l.projection(pair.offset);
            pair.offset = pair.origin + vect;
            break;
        }
    }
    return Offset_pair::simplify(left, _config.simplifydist());
}

void AlternateRoute::add_triplet(alter::Offset_triplet& triplet, //
        const Offset_pair& a, const Kernel::Point_2& b, const Kernel::Point_2& c) {

    triplet.origin = a.origin;
    triplet.offset1 = a.offset;
    Kernel::Line_2 l1(b, c);
    Kernel::Line_2 l2(triplet.origin, triplet.offset1);
    auto variant2 = CGAL::intersection(l1, l2);
    if (variant2) {
        if (const Kernel::Point_2* p2 = boost::get<Kernel::Point_2>(&*variant2)) {
            triplet.offset2 = *p2;
        }
    }
}

Arrangement_2 AlternateRoute::voronoi_arr(const Arrangement_2& arr, const int32_t& direction, const CGAL::Bbox_2& viewBox, const Ribbon& ribLimit) {
    std::vector<Segment_info_2> segments;
    for (const Halfedge& eit : RangeHelper::make(arr.edges_begin(), arr.edges_end())) {
        const Segment_info_2& curve = eit.curve();
        if (curve.data().direction() == direction) {
            segments.emplace_back(curve);

        }
    }
    Arrangement_2 arr2;
    CGAL::insert(arr2, segments.begin(), segments.end());

    Ribbon ribContour = Ribbon::createRibbonOfEdge(arr2, _config.simplifydist());
    ribContour.set_fill_color(direction);
    Ribbon ribContourFramed = ribContour;
    ribContourFramed.lines().emplace_back();
    Polyline& pl = ribContourFramed.lines().back();
    const double lx = viewBox.xmax() - viewBox.xmin();
    const double ly = viewBox.ymax() - viewBox.ymin();
    pl.points.emplace_back(Kernel::Point_2(viewBox.xmin() - lx, viewBox.ymin() - ly));
    pl.points.emplace_back(Kernel::Point_2(viewBox.xmax() + lx, viewBox.ymin() - ly));
    pl.points.emplace_back(Kernel::Point_2(viewBox.xmax() + lx, viewBox.ymax() + ly));
    pl.points.emplace_back(Kernel::Point_2(viewBox.xmin() - lx, viewBox.ymax() + ly));
    pl.closed = true;

    CGAL::Bbox_2 frameBox(viewBox.xmin() - 1, viewBox.ymin() - 1, viewBox.xmax() + 1, viewBox.ymax() + 1);
    VoronoiMedialSkeleton vor(ribContourFramed, frameBox);

    std::cout << "vor.get_vor_segments().size() " << vor.get_vor_segments().size() << std::endl;

    Arrangement_2 arrDir = vor.getSimpleArr(ribContour, ribLimit);

    return arrDir;
}

std::vector<alter::Offset_triplet> AlternateRoute::create_triplet_list(const Halfedge& he, const int32_t& direction) {
    std::vector<Offset_pair> left = couple_list(he, direction);
    std::vector<Offset_pair> right = couple_list(*he.twin(), direction);

    std::reverse(right.begin(), right.end());

    std::vector<alter::Offset_triplet> triplet_list;
    {
        std::size_t i = 0;
        std::size_t ii = 0;

        while (i < left.size() and ii < right.size()) {
            triplet_list.emplace_back();
            alter::Offset_triplet& triplet = triplet_list.back();

            Kernel::Comparison_result predicat = CGAL::compare_distance_to_point(he.target()->point(), left.at(i).origin, right.at(ii).origin);
            switch (predicat) {
            case CGAL::Comparison_result::EQUAL: {
                triplet.origin = left.at(i).origin;
                triplet.offset1 = left.at(i).offset;
                triplet.offset2 = right.at(ii).offset;
                ++i;
                ++ii;

                break;
            }
            case CGAL::Comparison_result::SMALLER: {
                add_triplet(triplet, left.at(i), right.at(ii - 1).offset, right.at(ii).offset);

                ++i;
                break;
            }
            case CGAL::Comparison_result::LARGER: {
                add_triplet(triplet, right.at(ii), left.at(i - 1).offset, left.at(i).offset);
                std::swap(triplet.offset1, triplet.offset2);
                ++ii;
                break;
            }
            }

        }
    }

    return triplet_list;
}

void AlternateRoute::ribToTrapeze(const Ribbon& rib, std::vector<alter::Segment_trapeze_info_2>& trapeze_vect, //
        const Arrangement_2& arr, const CGAL::Bbox_2& viewBox, const Ribbon& ribLimit) {

    int32_t direction = rib.strokeColor();
    std::cout << "start voronoi_arr " << std::endl;

    Arrangement_2 arrDir = voronoi_arr(arr, direction, viewBox, ribLimit);
    std::cout << "end voronoi_arr " << std::endl;
    for (const Halfedge& he : RangeHelper::make(arrDir.edges_begin(), arrDir.edges_end())) {
        // here (direction + 1) indicate all the segments generated by voronoi
        if (he.curve().data().direction() == direction && (++he.ccb())->curve().data().direction() == direction + 1) {
            std::vector<alter::Offset_triplet> triplet_list = create_triplet_list(he, direction);
            for (std::size_t i = 1; i < triplet_list.size(); ++i) {
                const alter::Offset_triplet& t1 = triplet_list.at(i - 1);
                const alter::Offset_triplet& t2 = triplet_list.at(i);
                trapeze_vect.emplace_back(Kernel::Segment_2(t1.origin, t2.origin), alter::TrapezeEdgeInfo(t1, t2, direction));
            }
        }
    }
}

void AlternateRoute::populateTrapeze(const GridIndex& gridIndex, const std::vector<Ribbon>& ribList, const CGAL::Bbox_2& viewBox, std::vector<alter::Segment_trapeze_info_2>& trapeze_vect) {
    const Ribbon& ribLimit = gridIndex.limit(ribList);
    const Ribbon& ribCircular = ribList.at(gridIndex._circular);
    const Ribbon& ribRadial = ribList.at(gridIndex._radial);
    Arrangement_2 arr = gridIndex.getArr(ribList);
    // remove inner  antenna
    arr = prune_arrangement(arr);
    arr = remove_antenna(arr);

    ribToTrapeze(ribCircular, trapeze_vect, arr, viewBox, ribLimit);
    ribToTrapeze(ribRadial, trapeze_vect, arr, viewBox, ribLimit);

}

AlternateRoute::AlternateRoute(const proto::AlternateRouting& config, const proto::Filepaths& filepaths) :
        _config(config) {

    sq_max_thickness = _config.maxthickness() * _config.maxthickness();
    sq_min_thickness = _config.minthickness() * _config.minthickness();

    svgp::Loader load(filepaths.inputfile());
    std::vector<Ribbon> &ribList = load.ribList();
    for (std::size_t i = 0; i < ribList.size(); ++i) {
        // the skeleton grid info is on stroke color
        // put it on fill color, in order to make the arrangement with different circular, radial info
        Ribbon& rib = ribList.at(i);
        rib.set_fill_color(rib.strokeColor());
    }
    std::unordered_map<uint32_t, GridIndex> mapOfGrids = GridIndex::getIndexMap(ribList);
// to be link to config

    std::vector<alter::Segment_trapeze_info_2> trapeze_vect;

    for (const auto& pair : mapOfGrids) {
        populateTrapeze(pair.second, ribList, load.viewBox(), trapeze_vect);
    }
    std::cout << "trapeze_vect filled " << std::endl;
    alter::ArrTrapeze arrTrapeze;
    CGAL::insert(arrTrapeze, trapeze_vect.begin(), trapeze_vect.end());

    std::cout << "arrTrapeze filled " << std::endl;
    std::unordered_map<const alter::Segment_trapeze_info_2*, std::size_t> curve_plc_map;

    std::vector<PolyConvex> polyConvexVect;
//we populate polyConvex
    for (const alter::ArrTrapeze::Halfedge& eit : RangeHelper::make(arrTrapeze.edges_begin(), arrTrapeze.edges_end())) {
        const alter::Segment_trapeze_info_2& curve = eit.curve();
        std::size_t index = polyConvexVect.size();
        polyConvexVect.emplace_back(eit.source()->point(), eit.target()->point(), index, curve.data().getGeometry(curve));
        curve_plc_map.emplace(&curve, polyConvexVect.back()._id);

    }

    std::cout << "polyconvex populated" << std::endl;

//we connect polyConvex
    for (const alter::ArrTrapeze::Vertex & v : RangeHelper::make(arrTrapeze.vertices_begin(), arrTrapeze.vertices_end())) {
        std::unordered_map<int32_t, std::vector<std::size_t>> color_plc_map;

        for (const alter::ArrTrapeze::Halfedge& he : RangeHelper::make(v.incident_halfedges())) {
            const alter::Segment_trapeze_info_2& curve = he.curve();

            std::vector<std::size_t> &plc_vect = color_plc_map.try_emplace(curve.data().direction()).first->second;
            plc_vect.emplace_back(curve_plc_map.at(&curve));
        }
        std::vector<std::size_t> orphan;
        bool only_orphan = true;
        for (const auto& ite : color_plc_map) {
            const std::vector<std::size_t>& vect = ite.second;
            if (vect.size() == 1) {
                orphan.emplace_back(vect.front());
            } else {
                only_orphan = false;
            }
        }

        if (!only_orphan) {
            for (const auto& ite : color_plc_map) {
                const std::vector<std::size_t>& vect = ite.second;
                if (vect.size() > 1) {
                    for (std::size_t i = 1; i < vect.size(); ++i) {
                        PolyConvex::connect(vect.at(i - 1), vect.at(i), polyConvexVect, v.point());
                    }
                    for (std::size_t i = 0; i < orphan.size(); ++i) {
                        PolyConvex::connect(vect.at(0), orphan.at(i), polyConvexVect, v.point());
                    }
                }
            }
        } else {
            for (std::size_t i = 1; i < orphan.size(); ++i) {
                PolyConvex::connect(orphan.at(0), orphan.at(i), polyConvexVect, v.point());
            }
        }
    }

    std::cout << "polyconvex connected" << std::endl;

    aniso::Routing::connectMaze(polyConvexVect);

    std::cout << "maze connected" << std::endl;

    OrientedRibbon oriented_ribbon;
    PathRendering::pathRender(polyConvexVect, oriented_ribbon);

    GraphicRendering::printRibbonSvg(load.viewBox(), filepaths.outputfile(), 0.3, oriented_ribbon.getResult());

}

} /* namespace laby */

/*
 * PenStroke.cpp
 *
 *  Created on: Jun 11, 2018
 *      Author: florian
 */

#include "PenStroke.h"

#include <cstdint>
#include <CGAL/enum.h>
#include <CGAL/Kernel/global_functions_2.h>
#include <CGAL/number_utils.h>
#include <CGAL/Point_2.h>
#include <CGAL/Vector_2.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <random>
#include <vector>
#include <CGAL/Polygon_set_2.h>
#include "../agg/agg_arc.h"

namespace laby {

const Polyline& PenStroke::LineConstruct::getMedianList() const {
    return _anti;
}

void PenStroke::LineConstruct::setClosed() {
    _anti.closed = true;
    _anti.points.emplace_back(_anti.points.front());
    _sym.emplace_back(_sym.front());

}
bool PenStroke::LineConstruct::isClosed() const {
    return _anti.closed;
}

const Point_2& PenStroke::LineConstruct::getMedian(const size_t& i) const {
    return _anti.points.at(i);
}

const double& PenStroke::LineConstruct::getBorder(const size_t& i) const {
    return _sym.at(i);
}

double PenStroke::smoothstep(double edge0, double edge1, double x) const {
    // Scale, bias and saturate x to 0..1 range
    x = std::clamp((x - edge0) / (edge1 - edge0), 0.0, 1.0);
    // Evaluate polynomial
    return x * x * (3 - 2 * x);
}

Point_2 PenStroke::barycentre(const Point_2& ori, const int32_t vi, const CGAL::Vector_2<Kernel>& u) const {
    return ori + vi * u;
}

double PenStroke::smooth(const Point_2& p, const Point_2& o) const {
    if (p == o) {
        return 0;
    }
    return smoothstep(0, _config.antisymmetric_amplitude(), sqrt(CGAL::to_double((p - o).squared_length())));
}

void PenStroke::LineConstruct::addPoint(const Point_2& p, const double& v) {

    _anti.points.emplace_back(p);
    _sym.emplace_back(v);

}

void PenStroke::addPoint(LineConstruct& lc, const Point_2& p, const Point_2& o, const Point_2& d, int32_t inc, const CGAL::Vector_2<Kernel>& v) {

    double sym = hqNoise2D_sym.get(CGAL::to_double(p.x()) + 10., CGAL::to_double(p.y()) + 10.);
    double anti = hqNoise2D_anti.get(CGAL::to_double(p.x()) + 10., CGAL::to_double(p.y()) + 10.);

    double antiClamp = anti * smooth(p, o) * smooth(p, d);

    lc.addPoint(p + v * (inc * antiClamp), sym + _config.thickness());

}

void PenStroke::line_to(svg::Path& cr, const Point_2& p) {
    cr << p;
}

void PenStroke::move_to(svg::Path& cr, const Point_2& p) {
    cr.startNewSubPath();
    cr << p;
}

PenStroke PenStroke::createPenStroke(const proto::PenStroke& config, const CGAL::Bbox_2& bbox) {
    generator::HqNoiseConfig config_sym;
    config_sym.maxN = static_cast<uint32_t>(std::max(bbox.xmax(), bbox.ymax()));
    config_sym.seed = config.symmetric_seed();
    config_sym.amplitude = config.symmetric_amplitude();
    config_sym.accuracy = static_cast<uint32_t>(std::ceil(1. / config.resolution()));
    config_sym.gaussian.frequency = config_sym.maxN * 100.;
    config_sym.powerlaw.frequency = config_sym.maxN / config.symmetric_freq();
    config_sym.powerlaw.power = 2;
    config_sym.complex = false;

    generator::HqNoiseConfig config_anti;
    config_anti.maxN = static_cast<uint32_t>(std::max(bbox.xmax(), bbox.ymax()));
    config_anti.seed = config.antisymmetric_seed();
    config_anti.amplitude = config.antisymmetric_amplitude();
    config_anti.accuracy = static_cast<uint32_t>(std::ceil(1. / config.resolution()));
    config_anti.gaussian.frequency = config_sym.maxN * 100.;
    config_anti.powerlaw.frequency = config_sym.maxN / config.antisymmetric_freq();
    config_anti.powerlaw.power = 2;
    config_anti.complex = false;

    return PenStroke(config, config_sym, config_anti);
}

void PenStroke::drawRibbonStroke(svg::Path& cr, const Ribbon& ribbon) {
    for (const Polyline& pl : ribbon.lines()) {
        if (pl.points.size() > 1ul) {

            move_to(cr, pl.points.at(0));

            for (std::size_t i = 1; i < pl.points.size(); ++i) {
                line_to(cr, pl.points.at(i));
            }
            if (pl.closed) {
                cr.closeSubPath();
            }

        }
    }
}

std::vector<Segment_info_2> PenStroke::getSegmentFromMedian(const std::unordered_set<std::size_t>& refMedLineSet) {
    std::vector<Segment_info_2> listSeg;
    for (const std::size_t& i : refMedLineSet) {
        const Polyline& pl = _medrib.at(i).getMedianList();
        for (std::size_t j = 1; j < pl.points.size(); ++j) {
            listSeg.push_back(Segment_info_2(Kernel::Segment_2(pl.points.at(j - 1), pl.points.at(j)), EdgeInfo { 1, 0 }));
        }
    }
    return listSeg;
}

Ribbon PenStroke::fillFace(const std::vector<Segment_info_2>& listSeg) const {
    Ribbon rib;

    Arrangement_2 arr;
    CGAL::insert(arr, listSeg.begin(), listSeg.end());
    for (const Face& fc : RangeHelper::make(arr.unbounded_faces_begin(), arr.unbounded_faces_end())) {
        for (Arrangement_2::Inner_ccb_const_iterator ite = fc.holes_begin(); ite != fc.holes_end(); ++ite) {
            rib.lines().emplace_back();
            Polyline& pl = rib.lines().back();

            pl.points.emplace_back((*ite)->source()->point());

            for (const Halfedge& he : RangeHelper::make(*ite)) {
                pl.points.emplace_back(he.target()->point());

            }
            pl.closed = true;
        }
    }

    return rib;
}

const Face& PenStroke::faceWithoutAntenna(Arrangement_2& arr, const Face& fc) {
    std::vector<Segment_info_2> listSeg;
    for (const Halfedge& he : RangeHelper::make(fc.outer_ccb())) {
        if (&*he.face() != &*he.twin()->face()) { // not an antenna
            listSeg.push_back(he.curve());
        }
    }

    for (Arrangement_2::Inner_ccb_const_iterator ite = fc.holes_begin(); ite != fc.holes_end(); ++ite) {
        for (const Halfedge& he : RangeHelper::make(*ite)) {
            if (&*he.face() != &*he.twin()->face()) { // not an antenna
                listSeg.push_back(he.curve());
            }
        }
    }

    CGAL::insert(arr, listSeg.begin(), listSeg.end());

    const Face& unbound = *arr.unbounded_face();
    Arrangement_2::Inner_ccb_const_iterator ite = unbound.holes_begin();
    return *((*ite)->twin()->face());
}

void PenStroke::drawFace(svg::Path& cr, const Face& fc) {

    if (fc.has_outer_ccb()) {
        Arrangement_2 arrTemp;
        const Face& fc2 = faceWithoutAntenna(arrTemp, fc);
        {
            std::unordered_set<std::size_t> refMedLineSet;
            for (const Halfedge& he : RangeHelper::make(fc2.outer_ccb())) {
                refMedLineSet.emplace(he.curve().data().coordinate());
            };
            Ribbon rib = fillFace(getSegmentFromMedian(refMedLineSet));
            rib.reverse();
            drawRibbonStroke(cr, rib);
        }

        for (Arrangement_2::Inner_ccb_const_iterator ite = fc2.holes_begin(); ite != fc2.holes_end(); ++ite) {
            std::unordered_set<std::size_t> refMedLineSet;
            for (const Halfedge& he : RangeHelper::make(*ite)) {
                refMedLineSet.emplace(he.curve().data().coordinate());
            }
            drawRibbonStroke(cr, fillFace(getSegmentFromMedian(refMedLineSet)));

        }

    }
}

void PenStroke::createUnion(Ribbon& ribs, const std::vector<PolyConvex>& polyConvexList) const {

    CGAL::Polygon_set_2<Kernel> set;

    std::vector<Linear_polygon> plv;
    plv.reserve(polyConvexList.size());
    for (const PolyConvex& pc : polyConvexList) {
        plv.emplace_back(pc._geometry);
    }
    set.join(plv.begin(), plv.end());

    std::vector<CGAL::Polygon_with_holes_2<Kernel> > polygons;

    set.polygons_with_holes(std::back_inserter(polygons));

    for (const CGAL::Polygon_with_holes_2<Kernel>& polygon : polygons) {

        ribs.lines().emplace_back(polygon.outer_boundary());

        for (const auto& hole : RangeHelper::make(polygon.holes_begin(), polygon.holes_end())) {
            ribs.lines().emplace_back(hole);

        }
    }
}

void PenStroke::draw_outline(svg::Path& cr) const {
    for (const LineConstruct & medLine : _medrib) {

        std::vector<PolyConvex> polyConvexVect;
        //we populate polyConvex
        std::size_t size = medLine.getMedianList().points.size();
        std::size_t begin = polyConvexVect.size();
        for (std::size_t j = 1; j < size; ++j) {
            std::size_t index = polyConvexVect.size();
            const Point_2& ps = medLine.getMedian(j - 1);
            const Point_2& pt = medLine.getMedian(j);
            polyConvexVect.emplace_back(ps, pt, index, PolygonTools::makeTrapeze(ps, pt, medLine.getBorder(j - 1), medLine.getBorder(j)));

        }
        std::size_t last = polyConvexVect.size() - 1;

        PolyConvex::connect(begin, polyConvexVect);
        {
            const PolyConvex& first = polyConvexVect.front();
            agg::Arc arc(medLine.getMedian(0), medLine.getBorder(0) / 2., first._geometry.vertex(0), first._geometry.vertex(1));
            polyConvexVect.emplace_back();
            polyConvexVect.back()._geometry.insert(polyConvexVect.back()._geometry.vertices_end(), arc.getPoints().begin(), arc.getPoints().end());

        }
        {
            const PolyConvex& pcl = polyConvexVect.at(last);
            agg::Arc arc(medLine.getMedian(size - 1), medLine.getBorder(size - 1) / 2., pcl._geometry.vertex(2), pcl._geometry.vertex(3));
            polyConvexVect.emplace_back();
            polyConvexVect.back()._geometry.insert(polyConvexVect.back()._geometry.vertices_end(), arc.getPoints().begin(), arc.getPoints().end());

        }

        Ribbon rib;
        createUnion(rib, polyConvexVect);
        rib.simplify(0.0001);
        drawRibbonStroke(cr, rib);

    }

}

void PenStroke::createStroke(const Polyline& pl) {

    if (pl.points.size() <= 1ul) {
        std::cout << "warning : polyline with size: " << pl.points.size() << std::endl;
        return;
    }
    const Point_2& o = pl.points.front();
    const Point_2& d = pl.points.back();

    _medrib.emplace_back();
    LineConstruct &medLine = _medrib.back();
    for (std::size_t i = 1; i < pl.points.size(); ++i) {

        CGAL::Vector_2<Kernel> vec = pl.points.at(i) - pl.points.at(i - 1);

        double length = sqrt(CGAL::to_double(vec.squared_length()));
        if (length > 0) {
            CGAL::Vector_2<Kernel> u = vec / length;
            CGAL::Vector_2<Kernel> v = u.perpendicular(CGAL::LEFT_TURN);
            u = u * _config.resolution();

            addPoint(medLine, pl.points.at(i - 1), o, d, +1, v);
            for (int32_t vi = 1; vi < length / _config.resolution(); ++vi) {
                addPoint(medLine, barycentre(pl.points.at(i - 1), vi, u), o, d, +1, v);

            }
            if (i + 1 == pl.points.size()) {

                if (pl.closed or pl.points.front() == pl.points.back()) {

                    medLine.setClosed();
                } else {
                    addPoint(medLine, pl.points.at(i), o, d, +1, v);
                }
            }
        }
    }

}

} /* namespace laby */

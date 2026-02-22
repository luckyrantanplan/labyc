/*
 * PenStroke.h
 *
 *  Created on: Jun 11, 2018
 *      Author: florian
 */

#ifndef RENDERING_PENSTROKE_H_
#define RENDERING_PENSTROKE_H_

#include "../generator/HqNoise.h"
#include "../GeomData.h"
#include "../Ribbon.h"
#include "../protoc/AllConfig.pb.h"
#include "../PolyConvex.h"
#include "../SVGWriter/DocumentSVG.h"

namespace laby {

class PenStroke {
public:

    class LineConstruct {
    public:
        const Point_2& getMedian(const size_t& i) const;

        const Polyline& getMedianList() const;

        const double& getBorder(const size_t& i) const;

        void addPoint(const Point_2& p, const double& v);

        void setClosed();

        bool isClosed() const;

    private:

        Polyline _anti;
        std::vector<double> _sym;

    };

    PenStroke(const proto::PenStroke& config, const generator::HqNoiseConfig& config_sym, const generator::HqNoiseConfig& config_anti) :
            _config(config), hqNoise2D_sym(config_sym), hqNoise2D_anti(config_anti) {
    }

    void createStroke(const Polyline& pl);
    void drawFace(svg::Path& cr, const Face& fc);
    static void drawRibbonStroke(svg::Path& cr, const Ribbon& ribbon);

    static PenStroke createPenStroke(const proto::PenStroke& config, const CGAL::Bbox_2& bbox);
    void draw_outline(svg::Path& cr) const;

private:
    void createUnion(Ribbon& ribs, const std::vector<PolyConvex>& polyConvexList) const;
    void addPoint(LineConstruct& lc, const Point_2& p, const Point_2& o, const Point_2& d, const int32_t inc, const CGAL::Vector_2<Kernel>& v);
    const Face& faceWithoutAntenna(Arrangement_2& arr, const Face& fc);

    Point_2 barycentre(const Point_2& ori, const int32_t vi, const CGAL::Vector_2<Kernel>& u) const;

    static void line_to(svg::Path& cr, const Point_2& p);
    static void move_to(svg::Path& cr, const Point_2& p);

    Ribbon fillFace(const std::vector<Segment_info_2>& listSeg) const;

    double smoothstep(double edge0, double edge1, double x) const;
    double smooth(const Point_2& p, const Point_2& o) const;
    std::vector<Segment_info_2> getSegmentFromMedian(const std::unordered_set<std::size_t>& refMedLineSet);

    const proto::PenStroke _config;
    generator::HqNoise2D hqNoise2D_sym;
    generator::HqNoise2D hqNoise2D_anti;

    std::vector<LineConstruct> _medrib;
};

} /* namespace laby */

#endif /* RENDERING_PENSTROKE_H_ */

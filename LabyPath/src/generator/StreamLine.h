/*
 * StreamLine.h
 *
 *  Created on: Feb 1, 2018
 *      Author: florian
 */

#ifndef STREAMLINE_H_
#define STREAMLINE_H_

#include <boost/multi_array.hpp>
#include <CGAL/Cartesian_converter.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Lazy_kernel.h>
#include <CGAL/Point_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Regular_grid_2.h>
#include <CGAL/Runge_kutta_integrator_2.h>
#include <CGAL/squared_distance_2_1.h>
#include <CGAL/Stream_lines_2.h>
#include <CGAL/Triangular_field_2.h>
#include <CGAL/Triangulation_vertex_base_2.h>
#include <CGAL/Vector_2.h>
#include <complex>
#include <cstddef>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../GeomData.h"
#include "../Ribbon.h"

namespace laby {
namespace generator {
class RibbonCoord {
public:

    RibbonCoord(Polyline* pl, std::size_t pos) :
            _pl { pl }, _pos { pos } {
    }
    Polyline* _pl;
    std::size_t _pos;
};



class StreamLine {

public:

    struct Config {
        int resolution = 4;
        double simplify_distance = 0.1;
        double dRat = 1; //1.6
        double epsilon = 0.01; //remaining field outside the feature
        double size; // width of the square canvas
        double divisor; // distance between lines
        bool old_RegularGrid = false;

    };
    const Ribbon& circularList() const {
        return _circularList;
    }

    const Ribbon& radialList() const {
        return _radialList;
    }

    typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

    struct VectorCompute {

        typedef CGAL::Cartesian_converter<Kernel, K> Kernel_To_K;

        VectorCompute(const double resolution) :
                scale(CGAL::SCALING, resolution) {
        }

        void addSegLong(std::vector<CGAL::Point_2<K> >& pointList, const CGAL::Segment_2<Kernel>& seg, std::vector<CGAL::Vector_2<K> >& vectorList);
        void addSegPerp(std::vector<CGAL::Point_2<K> >& pointList, const CGAL::Segment_2<Kernel>& seg, std::vector<CGAL::Vector_2<K> >& vectorList);

        Kernel_To_K kernel_to_k;
        CGAL::Aff_transformation_2<K> scale;
        double epsilon = 0.001;
    };

    typedef CGAL::Point_set_2<Kernel> PS;
    typedef CGAL::Triangular_field_2<K> FieldTri;

    typedef CGAL::Regular_grid_2<StreamLine::K> Field;

    typedef std::list<CGAL::Point_2<K>> Strl_polyline;
    typedef std::list<std::pair<Strl_polyline::iterator, Strl_polyline::iterator>> Strl_iterator_container;

    explicit StreamLine(const Config& config);

    void addToArrangement(Arrangement_2& arr);

    void drawSpiral(const std::complex<double>& o, const double& r, const double& angle);

    void render();

    static Ribbon getRadial(const Config& config, const std::vector<CGAL::Polygon_with_holes_2<Kernel> >& segs);
    static Ribbon getLongitudinal(const Config& config, const std::vector<CGAL::Polygon_with_holes_2<Kernel> >& segs);
private:

    Ribbon connectExtreme(Ribbon& ribbon);
    void connectExtremInPlace(laby::Ribbon& ribbon);
    void changeLine(const Point_2& mid, std::unordered_map<PS::Vertex*, RibbonCoord>& map, PS::Vertex* seg);

    template<typename FIELD, typename Rk_integrator = CGAL::Runge_kutta_integrator_2<FIELD> >
    CGAL::Stream_lines_2<FIELD, Rk_integrator> streamPlacement(const FIELD& field, double dSep, double dRat) {

        Rk_integrator runge_kutta_integrator;
        return CGAL::Stream_lines_2<FIELD, Rk_integrator>(field, runge_kutta_integrator, dSep, dRat);

    }

    void postStreamCompute(const Strl_iterator_container& Stream_lines, Ribbon& ribbon);

    static Ribbon generateTriangularField(std::vector<CGAL::Point_2<K> > pointList, std::vector<CGAL::Vector_2<K> > vectorList, const Config& config);
    CGAL::Vector_2<CGAL::Epick> addSeg(const std::vector<CGAL::Point_2<K> >& pointList, const CGAL::Segment_2<Kernel>& seg, VectorCompute& vCompute, std::vector<CGAL::Vector_2<K> >& vectorList);

    Config _config;
    Ribbon _radialList;
    Ribbon _circularList;
    boost::multi_array<std::complex<double>, 2> _field;
};
} /* namespace generator */
} /* namespace laby */
#endif /* STREAMLINE_H_ */

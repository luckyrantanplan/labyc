/*
 * StreamLine.h
 *
 *  Created on: Feb 1, 2018
 *      Author: florian
 */

#ifndef STREAMLINE_H_
#define STREAMLINE_H_

#include <CGAL/Cartesian_converter.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Lazy_exact_nt.h>
#include <CGAL/Lazy_kernel.h>
#include <CGAL/Point_2.h>
#include <CGAL/Point_set_2.h>
#include <CGAL/Polygon_with_holes_2.h>
#include <CGAL/Regular_grid_2.h>
#include <CGAL/Runge_kutta_integrator_2.h>
#include <CGAL/Stream_lines_2.h>
#include <CGAL/Triangular_field_2.h>
#include <CGAL/Triangulation_vertex_base_2.h>
#include <CGAL/Vector_2.h>
#include <CGAL/squared_distance_2.h>
#include <boost/multi_array.hpp>
#include <complex>
#include <cstddef>
#include <list>
#include <unordered_map>
#include <utility>
#include <vector>

#include "../GeomData.h"
#include "../Ribbon.h"

namespace laby::generator {
class RibbonCoord {
  public:
    RibbonCoord(Polyline* polyline, std::size_t position)
        : _polyline{polyline}, _position{position} {}

    [[nodiscard]] auto polyline() const -> Polyline* {
        return _polyline;
    }

    [[nodiscard]] auto position() const -> std::size_t {
        return _position;
    }

  private:
    Polyline* _polyline;
    std::size_t _position;
};

class StreamLine {

  public:
    struct Config {
        static constexpr double kDefaultSimplifyDistance = 0.1;
        static constexpr double kDefaultDRat = 1.0;
        static constexpr double kDefaultEpsilon = 0.01;

        int resolution = 4;
        double simplify_distance = kDefaultSimplifyDistance;
        double dRat = kDefaultDRat;       // 1.6
        double epsilon = kDefaultEpsilon; // remaining field outside the feature
        double size;                      // width of the square canvas
        double divisor;                   // distance between lines
        bool old_RegularGrid = false;
    };

    struct SpiralParameters {
        std::complex<double> origin;
        double radius = 0.0;
        double angleRadians = 0.0;
    };
    [[nodiscard]] auto circularList() const -> const Ribbon& {
        return _circularList;
    }

    [[nodiscard]] auto radialList() const -> const Ribbon& {
        return _radialList;
    }

    using K = CGAL::Exact_predicates_inexact_constructions_kernel;

    struct VectorCompute {

        using KernelToK = CGAL::Cartesian_converter<Kernel, K>;

        explicit VectorCompute(const double resolution)
            : _kernelToK(), _scale(CGAL::SCALING, resolution) {}

        void addSegLong(std::vector<CGAL::Point_2<K>>& pointList,
                        const CGAL::Segment_2<Kernel>& seg,
                        std::vector<CGAL::Vector_2<K>>& vectorList) const;
        void addSegPerp(std::vector<CGAL::Point_2<K>>& pointList,
                        const CGAL::Segment_2<Kernel>& seg,
                        std::vector<CGAL::Vector_2<K>>& vectorList) const;

      private:
        static constexpr double kDefaultEpsilon = 0.001;

        KernelToK _kernelToK;
        CGAL::Aff_transformation_2<K> _scale;
        double _epsilon = kDefaultEpsilon;

        friend class StreamLine;
    };

    using PS = CGAL::Point_set_2<Kernel>;
    using FieldTri = CGAL::Triangular_field_2<K>;

    using Field = CGAL::Regular_grid_2<StreamLine::K>;

    using Strl_polyline = std::list<CGAL::Point_2<K>>;
    using Strl_iterator_container =
        std::list<std::pair<Strl_polyline::iterator, Strl_polyline::iterator>>;

    explicit StreamLine(const Config& config);

    void addToArrangement(Arrangement_2& arr);

    void drawSpiral(const SpiralParameters& parameters);

    void render();

    static auto
    getRadial(const Config& config,
              const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons) -> Ribbon;
    static auto
    getLongitudinal(const Config& config,
                    const std::vector<CGAL::Polygon_with_holes_2<Kernel>>& polygons) -> Ribbon;

  private:
    static auto connectExtreme(Ribbon& ribbon) -> Ribbon;
    static void connectExtremInPlace(laby::Ribbon& ribbon);
    static void changeLine(const Point_2& midpoint,
                           std::unordered_map<PS::Vertex*, RibbonCoord>& ribbonCoordMap,
                           PS::Vertex* vertex);

    template <typename FIELD, typename Rk_integrator = CGAL::Runge_kutta_integrator_2<FIELD>>
    auto streamPlacement(const FIELD& field, double dSep,
                         double dRat) -> CGAL::Stream_lines_2<FIELD, Rk_integrator> {

        Rk_integrator rungeKuttaIntegrator;
        return CGAL::Stream_lines_2<FIELD, Rk_integrator>(field, rungeKuttaIntegrator, dSep, dRat);
    }

    void postStreamCompute(const Strl_iterator_container& stream_lines, Ribbon& ribbon) const;

    static auto generateTriangularField(std::vector<CGAL::Point_2<K>> pointList,
                                        std::vector<CGAL::Vector_2<K>> vectorList,
                                        const Config& config) -> Ribbon;
    auto addSeg(const std::vector<CGAL::Point_2<K>>& pointList, const CGAL::Segment_2<Kernel>& seg,
                VectorCompute& vCompute,
                std::vector<CGAL::Vector_2<K>>& vectorList) -> CGAL::Vector_2<CGAL::Epick>;

    Config _config;
    Ribbon _radialList;
    Ribbon _circularList;
    std::size_t _xSampleCount = 0;
    std::size_t _ySampleCount = 0;
    boost::multi_array<std::complex<double>, 2> _field;
};
} // namespace laby::generator

#endif /* STREAMLINE_H_ */

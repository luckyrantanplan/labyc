/*
 * PenStroke.h
 *
 *  Created on: Jun 11, 2018
 *      Author: florian
 */

#ifndef RENDERING_PENSTROKE_H_
#define RENDERING_PENSTROKE_H_

#include <cstdint>
#include <unordered_set>
#include <utility>

#include "../GeomData.h"
#include "../PolyConvex.h"
#include "../Ribbon.h"
#include "../SVGWriter/DocumentSVG.h"
#include "../generator/HqNoise.h"
#include "../protoc/AllConfig.pb.h"

namespace laby {

class PenStroke {
  public:
    struct NoiseConfigs {
        generator::HqNoiseConfig symmetric;
        generator::HqNoiseConfig antisymmetric;
    };

    class LineConstruct {
      public:
        [[nodiscard]] auto getMedian(std::size_t pointIndex) const -> const Point_2&;

        [[nodiscard]] auto getMedianList() const -> const Polyline&;

        [[nodiscard]] auto getBorder(std::size_t pointIndex) const -> const double&;

        void addPoint(const Point_2& point, double borderValue);

        void setClosed();

        [[nodiscard]] auto isClosed() const -> bool;

      private:
        Polyline _anti;
        std::vector<double> _sym{};
    };

    explicit PenStroke(proto::PenStroke config, NoiseConfigs noiseConfigs)
        : _config(std::move(config)), _hqNoise2DSym(noiseConfigs.symmetric),
          _hqNoise2DAnti(noiseConfigs.antisymmetric) {}

    void createStroke(const Polyline& polyline);
    void drawFace(svg::Path& path, const Face& face);
    static void drawRibbonStroke(svg::Path& path, const Ribbon& ribbon);

    static auto createPenStroke(const proto::PenStroke& config,
                                const CGAL::Bbox_2& bbox) -> PenStroke;
    void drawOutline(svg::Path& path) const;

  private:
    static void createUnion(Ribbon& ribbon, const std::vector<PolyConvex>& polyConvexList);
    void addPoint(LineConstruct& lineConstruct, const Point_2& point, const Point_2& origin,
                  const Point_2& destination, int32_t increment,
                  const CGAL::Vector_2<Kernel>& normalVector);
    static auto faceWithoutAntenna(Arrangement_2& arrangement, const Face& face) -> const Face&;

    static auto barycentre(const Point_2& origin, int32_t vertexIndex,
                           const CGAL::Vector_2<Kernel>& unitVector) -> Point_2;

    static void lineTo(svg::Path& path, const Point_2& point);
    static void moveTo(svg::Path& path, const Point_2& point);

    static auto fillFace(const std::vector<Segment_info_2>& segmentList) -> Ribbon;

    static auto smoothstep(double edgeStart, double edgeEnd, double value) -> double;
    [[nodiscard]] auto smooth(const Point_2& point, const Point_2& origin) const -> double;
    auto getSegmentFromMedian(const std::unordered_set<std::size_t>& referenceMedianLineSet)
        -> std::vector<Segment_info_2>;

    proto::PenStroke _config;
    generator::HqNoise2D _hqNoise2DSym;
    generator::HqNoise2D _hqNoise2DAnti;

    std::vector<LineConstruct> _medrib{};
};

} /* namespace laby */

#endif /* RENDERING_PENSTROKE_H_ */

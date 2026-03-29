/**
 * \file PoissonGenerator.h
 * \brief
 *
 * Poisson Disk Points Generator
 *
 * \version 1.1.4
 * \date 19/10/2016
 * \author Sergey Kosarevsky, 2014-2016
 * \author support@linderdaum.com   http://www.linderdaum.com   http://blog.linderdaum.com
 */

/*
 Usage example:

 #define POISSON_PROGRESS_INDICATOR 1
 #include "PoissonGenerator.h"
 ...
 PoissonGenerator::DefaultPRNG PRNG;
 const auto Points = PoissonGenerator::GeneratePoissonPoints( NumPoints, PRNG );
 */

// Fast Poisson Disk Sampling in Arbitrary Dimensions
// http://people.cs.ubc.ca/~rbridson/docs/bridson-siggraph07-poissondisk.pdf
// Implementation based on http://devmag.org.za/2009/05/03/poisson-disk-sampling/
/* Versions history:
 *		1.1.4 	Oct 19, 2016		POISSON_PROGRESS_INDICATOR can be defined outside of the header
 *file, disabled by default 1.1.3a	Jun  9, 2016		Update constructor for DefaultPRNG 1.1.3
 *Mar 10, 2016		Header-only library, no global mutable state 1.1.2		Apr  9, 2015
 *Output a text file with XY coordinates 1.1.1		May 23, 2014		Initialize PRNG seed, fixed
 *uninitialized fields 1.1		May  7, 2014		Support of density maps 1.0		May  6, 2014
 */

#include <CGAL/Bbox_2.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>

#include "../basic/RandomUniDist.h"

namespace laby::generator {

namespace {

constexpr double kUnitIntervalMax = 1.0;
constexpr double kCircleCenterCoordinate = 0.5;
constexpr double kCircleRadiusSquared = 0.25;
constexpr int kNeighbourhoodScanRadius = 5;
constexpr int kDefaultNewPointsCount = 30;
constexpr double kDefaultDensityNumerator = 6.3;
constexpr double kDefaultDensityDenominator = 10.0;

} // namespace

class SPoint {
  public:
    struct Coordinates {
        double xCoord = 0.0;
        double yCoord = 0.0;
    };

    SPoint() = default;

    explicit SPoint(Coordinates coordinates)
        : _xCoord(coordinates.xCoord), _yCoord(coordinates.yCoord), _isValid(true) {}

    [[nodiscard]] auto xCoord() const -> double {
        return _xCoord;
    }

    [[nodiscard]] auto yCoord() const -> double {
        return _yCoord;
    }

    [[nodiscard]] auto isValid() const -> bool {
        return _isValid;
    }

    [[nodiscard]] auto isInRectangle() const -> bool {
        return _xCoord >= 0.0 && _yCoord >= 0.0 && _xCoord <= kUnitIntervalMax &&
               _yCoord <= kUnitIntervalMax;
    }

    [[nodiscard]] auto isInCircle() const -> bool {
        const double xOffset = _xCoord - kCircleCenterCoordinate;
        const double yOffset = _yCoord - kCircleCenterCoordinate;
        return (xOffset * xOffset + yOffset * yOffset) <= kCircleRadiusSquared;
    }

  private:
    double _xCoord = 0.0;
    double _yCoord = 0.0;
    bool _isValid = false;
};

class SGridPoint {
  public:
    struct Coordinates {
        int xIndex = 0;
        int yIndex = 0;
    };

    explicit SGridPoint(Coordinates coordinates)
        : _xIndex(coordinates.xIndex), _yIndex(coordinates.yIndex) {}

    [[nodiscard]] auto xIndex() const -> int {
        return _xIndex;
    }

    [[nodiscard]] auto yIndex() const -> int {
        return _yIndex;
    }

  private:
    int _xIndex = 0;
    int _yIndex = 0;
};

class SGrid {
  public:
    struct Dimensions {
        int width = 0;
        int height = 0;
    };

    explicit SGrid(Dimensions dimensions, double cellSize)
        : _gridWidth(dimensions.width), _gridHeight(dimensions.height), _cellSize(cellSize) {
        _grid.resize(static_cast<std::size_t>(_gridWidth));

        for (auto& column : _grid) {
            column.resize(static_cast<std::size_t>(_gridHeight));
        }
    }

    [[nodiscard]] static auto imageToGrid(const SPoint& point, double cellSize) -> SGridPoint {
        return SGridPoint({static_cast<int>(point.xCoord() / cellSize),
                           static_cast<int>(point.yCoord() / cellSize)});
    }

    [[nodiscard]] static auto getSqDistance(const SPoint& firstPoint,
                                            const SPoint& secondPoint) -> double {
        const double deltaX = firstPoint.xCoord() - secondPoint.xCoord();
        const double deltaY = firstPoint.yCoord() - secondPoint.yCoord();
        return deltaX * deltaX + deltaY * deltaY;
    }

    void insert(const SPoint& point) {
        const SGridPoint gridPoint = imageToGrid(point, _cellSize);
        _grid[static_cast<std::size_t>(gridPoint.xIndex())]
             [static_cast<std::size_t>(gridPoint.yIndex())] = point;
    }

    [[nodiscard]] auto isInNeighbourhood(const SPoint& point,
                                         double squaredMinDistance) const -> bool {
        const SGridPoint gridPoint = imageToGrid(point, _cellSize);

        // scan the neighborhood of the point in the grid
        for (int columnIndex = gridPoint.xIndex() - kNeighbourhoodScanRadius;
             columnIndex < gridPoint.xIndex() + kNeighbourhoodScanRadius; ++columnIndex) {
            if (columnIndex >= 0 && columnIndex < _gridWidth) {
                for (int rowIndex = gridPoint.yIndex() - kNeighbourhoodScanRadius;
                     rowIndex < gridPoint.yIndex() + kNeighbourhoodScanRadius; ++rowIndex) {
                    if (rowIndex >= 0 && rowIndex < _gridHeight) {
                        const SPoint& candidatePoint = _grid[static_cast<std::size_t>(columnIndex)]
                                                            [static_cast<std::size_t>(rowIndex)];

                        if (candidatePoint.isValid() &&
                            getSqDistance(candidatePoint, point) < squaredMinDistance) {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

  private:
    int _gridWidth = 0;
    int _gridHeight = 0;
    double _cellSize = 0.0;

    std::vector<std::vector<SPoint>> _grid{};
};

class PoissonPoints {
  public:
    struct GenerateConfig {
        int newPointsCount = kDefaultNewPointsCount;
        bool circle = true;
        double minDistance = -1.0;
    };

    static auto generateRandomPointAround(const SPoint& point, const double squaredDistance,
                                          laby::basic::RandomUniDist& randomGenerator) -> SPoint {
        // start with non-uniform distribution

        const double squaredRadius =
            (3.0 * squaredDistance) * randomGenerator.get() + squaredDistance;
        // radius should be between MinDist and 2 * MinDist
        const double radius = sqrt(squaredRadius);
        const double angleFactor = randomGenerator.get();

        // random angle
        const double angle = 2.0 * M_PI * angleFactor;

        // the new point is generated around the point (x, y)
        const double xCoord = point.xCoord() + radius * cos(angle);
        const double yCoord = point.yCoord() + radius * sin(angle);

        return SPoint({xCoord, yCoord});
    }

    /**
     Return a vector of generated points

     NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
     Circle  - 'true' to fill a circle, 'false' to fill a rectangle
     MinDist - minimal distance estimator, use negative value for default
     **/

    static auto generate(std::size_t targetPointCount,
                         GenerateConfig config) -> std::vector<SPoint> {

        laby::basic::RandomUniDist randomDouble(0.0, 1.0, 2);

        std::cout << " NumPoints " << targetPointCount << '\n';
        if (config.minDistance < 0.0) {
            // optimal density in circle packing is about π⁄√12 or 90.69
            config.minDistance =
                sqrt(kDefaultDensityNumerator /
                     (kDefaultDensityDenominator * static_cast<double>(targetPointCount)));
        }

        const double squaredMinDistance = config.minDistance * config.minDistance;

        std::vector<SPoint> samplePoints;
        std::vector<SPoint> processList;

        // create the grid
        const double cellSize = config.minDistance / M_SQRT2;

        const int gridWidth = static_cast<int>(ceil(1.0 / cellSize));
        const int gridHeight = static_cast<int>(ceil(1.0 / cellSize));

        SGrid grid(SGrid::Dimensions{gridWidth, gridHeight}, cellSize);

        SPoint firstPoint;
        while (true) {
            firstPoint = SPoint({randomDouble.get(), randomDouble.get()});
            if (config.circle ? firstPoint.isInCircle() : firstPoint.isInRectangle()) {
                break;
            }
        }

        // update containers
        processList.push_back(firstPoint);
        samplePoints.push_back(firstPoint);
        grid.insert(firstPoint);

        // generate new points for each point in the queue
        while (!processList.empty() && samplePoints.size() < 2 * targetPointCount) {

            // a progress indicator, kind of
            //  if ( SamplePoints.size() % 100 == 0 ) std::cout << ".";

            const std::size_t idx = randomDouble.select(0, processList.size());
            const SPoint& point = processList[idx];
            bool noActive = true;
            for (int candidateIndex = 0; candidateIndex < config.newPointsCount; ++candidateIndex) {
                SPoint const newPoint =
                    generateRandomPointAround(point, squaredMinDistance, randomDouble);

                const bool fits = config.circle ? newPoint.isInCircle() : newPoint.isInRectangle();

                if (fits && !grid.isInNeighbourhood(newPoint, squaredMinDistance)) {
                    processList.push_back(newPoint);
                    samplePoints.push_back(newPoint);
                    grid.insert(newPoint);
                    noActive = false;
                    break;
                }
            }

            if (noActive) {
                std::swap(processList.at(idx), processList.back());
                processList.resize(processList.size() - 1U);
            }
        }

        randomDouble.shuffle(samplePoints);
        if (samplePoints.size() > targetPointCount) {
            samplePoints.resize(targetPointCount);
        }
        return samplePoints;
    }

    static auto generate(std::size_t targetPointCount) -> std::vector<SPoint> {
        return generate(targetPointCount, GenerateConfig{});
    }

    static auto generateRectangle(const CGAL::Bbox_2& bbox, const std::size_t numberOfPoints)
        -> std::vector<std::complex<double>> {

        const double width = bbox.xmax() - bbox.xmin();
        const double height = bbox.ymax() - bbox.ymin();

        const double maxLength = std::max(width, height);
        const double ratio = maxLength * maxLength / (width * height);

        std::vector<SPoint> const points =
            generate(static_cast<std::size_t>(ratio * static_cast<double>(numberOfPoints) + 1),
                     GenerateConfig{.circle = false});

        std::vector<std::complex<double>> result;
        for (const SPoint& pt : points) {
            if (pt.xCoord() * maxLength < width && pt.yCoord() * maxLength < height) {
                result.emplace_back(pt.xCoord() * maxLength + bbox.xmin(),
                                    pt.yCoord() * maxLength + bbox.ymin());
            }
        }

        return result;
    }
};
} // namespace laby::generator

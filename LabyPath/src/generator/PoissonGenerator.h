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

struct sPoint {
    sPoint() : x(0), y(0), m_Valid(false) {}
    sPoint(double X, double Y) : x(X), y(Y), m_Valid(true) {}
    double x;
    double y;
    bool m_Valid;
    //
    [[nodiscard]] auto isInRectangle() const -> bool {
        return x >= 0 && y >= 0 && x <= 1 && y <= 1;
    }
    //
    [[nodiscard]] auto isInCircle() const -> bool {
        double const fx = x - 0.5;
        double const fy = y - 0.5;
        return (fx * fx + fy * fy) <= 0.25;
    }
};

struct sGridPoint {
    sGridPoint(int X, int Y) : x(X), y(Y) {}
    int x;
    int y;
};

struct SGrid {

    [[nodiscard]] static sGridPoint imageToGrid(const sPoint& P, double CellSize) {
        return {(int)(P.x / CellSize), (int)(P.y / CellSize)};
    }
    [[nodiscard]] static double getSqDistance(const sPoint& P1, const sPoint& P2) {
        return (P1.x - P2.x) * (P1.x - P2.x) + (P1.y - P2.y) * (P1.y - P2.y);
    }
    SGrid(int W, int H, double CellSize) : _m_W(W), _m_H(H), _m_CellSize(CellSize) {
        m_Grid.resize(static_cast<std::size_t>(_m_W));

        for (auto & i : m_Grid) {
            i.resize(static_cast<std::size_t>(_m_H));
        }
    }
    void insert(const sPoint& P) {
        sGridPoint const g = imageToGrid(P, _m_CellSize);
        m_Grid[static_cast<std::size_t>(g.x)][static_cast<std::size_t>(g.y)] = P;
    }
    auto isInNeighbourhood(const sPoint& Point, double sqMinDist, double CellSize) -> bool {
        sGridPoint const g = imageToGrid(Point, CellSize);

        // number of adjacent cells to look for neighbor points
        const int d = 5;

        // scan the neighborhood of the point in the grid
        for (int i = g.x - d; i < g.x + d; ++i) {
            if (i >= 0 && i < _m_W) {
                for (int j = g.y - d; j < g.y + d; ++j) {
                    if (j >= 0 && j < _m_H) {
                        const sPoint& p =
                            m_Grid[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];

                        if (p.m_Valid && getSqDistance(p, Point) < sqMinDist) {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

  private:
    int _m_W;
    int _m_H;
    double _m_CellSize;

    std::vector<std::vector<sPoint>> m_Grid;
};

class PoissonPoints {
  public:
    static sPoint generateRandomPointAround(const sPoint& P, const double sqDist,
                                            laby::basic::RandomUniDist& Generator) {
        // start with non-uniform distribution

        double const sqR1 = (3 * sqDist) * Generator.get() + sqDist;
        // radius should be between MinDist and 2 * MinDist
        double const radius = sqrt(sqR1);
        double const r2 = Generator.get();

        // random angle
        double const angle = 2. * M_PI * r2;

        // the new point is generated around the point (x, y)
        double const x = P.x + radius * cos(angle);
        double const y = P.y + radius * sin(angle);

        return {x, y};
    }

    /**
     Return a vector of generated points

     NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
     Circle  - 'true' to fill a circle, 'false' to fill a rectangle
     MinDist - minimal distance estimator, use negative value for default
     **/

    static auto generate(size_t NumPoints, int NewPointsCount = 30,
                                        bool Circle = true, double MinDist = -1.0) -> std::vector<sPoint> {

        laby::basic::RandomUniDist randomDouble(0.0, 1.0, 2);

        std::cout << " NumPoints " << NumPoints << '\n';
        if (MinDist < 0.0) {
            // optimal density in circle packing is about π⁄√12 or 90.69
            MinDist = sqrt(6.3 / (10. * static_cast<double>(NumPoints)));
        }

        double const sqMinDist = MinDist * MinDist;

        std::vector<sPoint> samplePoints;
        std::vector<sPoint> processList;

        // create the grid
        double const cellSize = MinDist / M_SQRT2;

        int const gridW = static_cast<int>(ceil(1.0 / cellSize));
        int const gridH = static_cast<int>(ceil(1.0 / cellSize));

        SGrid grid(gridW, gridH, cellSize);

        sPoint firstPoint;
        while (true) {
            firstPoint = sPoint(randomDouble.get(), randomDouble.get());
            if (Circle ? firstPoint.isInCircle() : firstPoint.isInRectangle()) {
                break;
            }
        }

        // update containers
        processList.push_back(firstPoint);
        samplePoints.push_back(firstPoint);
        grid.insert(firstPoint);

        // generate new points for each point in the queue
        while (!processList.empty() && samplePoints.size() < 2 * NumPoints) {

            // a progress indicator, kind of
            //  if ( SamplePoints.size() % 100 == 0 ) std::cout << ".";

            const std::size_t idx = randomDouble.select(0, processList.size());
            const sPoint& point = processList[idx];
            bool noActive = true;
            for (int i = 0; i < NewPointsCount; ++i) {
                sPoint const newPoint = generateRandomPointAround(point, sqMinDist, randomDouble);

                bool const fits = Circle ? newPoint.isInCircle() : newPoint.isInRectangle();

                if (fits && !grid.isInNeighbourhood(newPoint, sqMinDist, cellSize)) {
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
        if (samplePoints.size() > NumPoints) {
            samplePoints.resize(NumPoints);
        }
        return samplePoints;
    }

    static auto generateRectangle(const CGAL::Bbox_2& bbox,
                                                               const size_t number_of_point) -> std::vector<std::complex<double>> {

        double const w = bbox.xmax() - bbox.xmin();
        double const h = bbox.ymax() - bbox.ymin();

        double const maxLength = std::max(w, h);
        double const ratio = maxLength * maxLength / (w * h);

        std::vector<sPoint> const points = generate(
            static_cast<std::size_t>(ratio * static_cast<double>(number_of_point) + 1), 30, false);

        std::vector<std::complex<double>> result;
        for (const sPoint& pt : points) {
            if (pt.x * maxLength < w and pt.y * maxLength < h) {
                result.emplace_back(pt.x * maxLength + bbox.xmin(), pt.y * maxLength + bbox.ymin());
            }
        }

        return result;
    }
};
} // namespace laby::generator


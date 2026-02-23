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
 *		1.1.4 	Oct 19, 2016		POISSON_PROGRESS_INDICATOR can be defined outside of the header file, disabled by default
 *		1.1.3a	Jun  9, 2016		Update constructor for DefaultPRNG
 *		1.1.3		Mar 10, 2016		Header-only library, no global mutable state
 *		1.1.2		Apr  9, 2015		Output a text file with XY coordinates
 *		1.1.1		May 23, 2014		Initialize PRNG seed, fixed uninitialized fields
 *    1.1		May  7, 2014		Support of density maps
 *		1.0		May  6, 2014
 */

#include <utility>
#include <CGAL/Bbox_2.h>
#include <algorithm>
#include <cmath>
#include <complex>
#include <cstddef>
#include <iostream>
#include <iterator>
#include <vector>

#include "../basic/RandomUniDist.h"

namespace laby {
namespace generator {

struct sPoint {
    sPoint() :
            x(0), y(0), m_Valid(false) {
    }
    sPoint(double X, double Y) :
            x(X), y(Y), m_Valid(true) {
    }
    double x;
    double y;
    bool m_Valid;
    //
    bool IsInRectangle() const {
        return x >= 0 && y >= 0 && x <= 1 && y <= 1;
    }
    //
    bool IsInCircle() const {
        double fx = x - 0.5;
        double fy = y - 0.5;
        return (fx * fx + fy * fy) <= 0.25;
    }
};

struct sGridPoint {
    sGridPoint(int X, int Y) :
            x(X), y(Y) {
    }
    int x;
    int y;
};

struct sGrid {

    sGridPoint ImageToGrid(const sPoint& P, double CellSize) const {
        return sGridPoint((int) (P.x / CellSize), (int) (P.y / CellSize));
    }
    double GetSqDistance(const sPoint& P1, const sPoint& P2) const {
        return (P1.x - P2.x) * (P1.x - P2.x) + (P1.y - P2.y) * (P1.y - P2.y);
    }
    sGrid(int W, int H, double CellSize) :
            m_W(W), m_H(H), m_CellSize(CellSize) {
        m_Grid.resize(static_cast<std::size_t>(m_W));

        for (auto i = m_Grid.begin(); i != m_Grid.end(); i++) {
            i->resize(static_cast<std::size_t>(m_H));
        }
    }
    void Insert(const sPoint& P) {
        sGridPoint G = ImageToGrid(P, m_CellSize);
        m_Grid[static_cast<std::size_t>(G.x)][static_cast<std::size_t>(G.y)] = P;
    }
    bool IsInNeighbourhood(const sPoint& Point, double sqMinDist, double CellSize) {
        sGridPoint G = ImageToGrid(Point, CellSize);

        // number of adjacent cells to look for neighbor points
        const int D = 5;

        // scan the neighborhood of the point in the grid
        for (int i = G.x - D; i < G.x + D; ++i) {
            if (i >= 0 && i < m_W) {
                for (int j = G.y - D; j < G.y + D; ++j) {
                    if (j >= 0 && j < m_H) {
                        const sPoint& P = m_Grid[static_cast<std::size_t>(i)][static_cast<std::size_t>(j)];

                        if (P.m_Valid && GetSqDistance(P, Point) < sqMinDist) {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

private:
    int m_W;
    int m_H;
    double m_CellSize;

    std::vector<std::vector<sPoint> > m_Grid;
};

class PoissonPoints {
public:

    static sPoint GenerateRandomPointAround(const sPoint& P, const double sqDist, laby::basic::RandomUniDist& Generator) {
        // start with non-uniform distribution

        double sqR1 = (3 * sqDist) * Generator.get() + sqDist;
        // radius should be between MinDist and 2 * MinDist
        double Radius = sqrt(sqR1);
        double R2 = Generator.get();

        // random angle
        double Angle = 2. * M_PI * R2;

        // the new point is generated around the point (x, y)
        double X = P.x + Radius * cos(Angle);
        double Y = P.y + Radius * sin(Angle);

        return sPoint(X, Y);
    }

    /**
     Return a vector of generated points

     NewPointsCount - refer to bridson-siggraph07-poissondisk.pdf for details (the value 'k')
     Circle  - 'true' to fill a circle, 'false' to fill a rectangle
     MinDist - minimal distance estimator, use negative value for default
     **/

    static std::vector<sPoint> generate(size_t NumPoints, int NewPointsCount = 30, bool Circle = true, double MinDist = -1.0) {

        laby::basic::RandomUniDist randomDouble(0.0, 1.0, 2);

        std::cout << " NumPoints " << NumPoints << std::endl;
        if (MinDist < 0.0) {
            //optimal density in circle packing is about π⁄√12 or 90.69
            MinDist = sqrt(6.3 / (10. * static_cast<double>(NumPoints)));
        }

        double sqMinDist = MinDist * MinDist;

        std::vector<sPoint> SamplePoints;
        std::vector<sPoint> ProcessList;

        // create the grid
        double CellSize = MinDist / M_SQRT2;

        int GridW = static_cast<int>(ceil(1.0 / CellSize));
        int GridH = static_cast<int>(ceil(1.0 / CellSize));

        sGrid Grid(GridW, GridH, CellSize);

        sPoint FirstPoint;
        do {
            FirstPoint = sPoint(randomDouble.get(), randomDouble.get());
        } while (!(Circle ? FirstPoint.IsInCircle() : FirstPoint.IsInRectangle()));

        // update containers
        ProcessList.push_back(FirstPoint);
        SamplePoints.push_back(FirstPoint);
        Grid.Insert(FirstPoint);

        // generate new points for each point in the queue
        while (!ProcessList.empty() && SamplePoints.size() < 2 * NumPoints) {

            // a progress indicator, kind of
            //  if ( SamplePoints.size() % 100 == 0 ) std::cout << ".";

            const std::size_t Idx = randomDouble.select(0, ProcessList.size());
            const sPoint& Point = ProcessList[Idx];
            bool no_active = true;
            for (int i = 0; i < NewPointsCount; ++i) {
                sPoint NewPoint = GenerateRandomPointAround(Point, sqMinDist, randomDouble);

                bool Fits = Circle ? NewPoint.IsInCircle() : NewPoint.IsInRectangle();

                if (Fits && !Grid.IsInNeighbourhood(NewPoint, sqMinDist, CellSize)) {
                    ProcessList.push_back(NewPoint);
                    SamplePoints.push_back(NewPoint);
                    Grid.Insert(NewPoint);
                    no_active = false;
                    break;
                }
            }

            if (no_active) {
                std::swap(ProcessList.at(Idx), ProcessList.back());
                ProcessList.resize(ProcessList.size() - 1u);
            }

        }

        randomDouble.shuffle(SamplePoints);
        if (SamplePoints.size() > NumPoints) {
            SamplePoints.resize(NumPoints);
        }
        return SamplePoints;
    }

    static std::vector<std::complex<double>> generateRectangle(const CGAL::Bbox_2& bbox, const size_t number_of_point) {

        double w = bbox.xmax() - bbox.xmin();
        double h = bbox.ymax() - bbox.ymin();

        double maxLength = std::max(w, h);
        double ratio = maxLength * maxLength / (w * h);

        std::vector<sPoint> Points = generate(static_cast<std::size_t>(ratio * static_cast<double>(number_of_point) + 1), 30, false);

        std::vector<std::complex<double>> result;
        for (const sPoint & pt : Points) {
            if (pt.x * maxLength < w and pt.y * maxLength < h) {
                result.emplace_back(pt.x * maxLength + bbox.xmin(), pt.y * maxLength + bbox.ymin());
            }
        }

        return result;
    }

};
} /* namespace generator */
} // namespace PoissonGenerator

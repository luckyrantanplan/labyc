/*
 * GlobalRouter.h
 *
 *  Created on: Jan 18, 2018
 *      Author: florian
 */

#ifndef GLOBALROUTER_H_
#define GLOBALROUTER_H_

#include <src/grdb/RoutingRegion.h>
#include <src/misc/geometry.h>
#include <random>
#include <vector>

class Rectangle {
public:
    Rectangle(int xmin, int xlen, int ymin, int ylen, int layer) :
            xmin { xmin }, xmax { xmin + xlen }, ymin { ymin }, ymax { ymin + ylen }, //
            layer { layer }, //
            rd { }, gen { rd() }, //
            dis { 0.0, 1.0 } {

    }

    std::vector<NTHUR::Coordinate_3d> getPin(int space) {
        std::vector<NTHUR::Coordinate_3d> v;
        for (int x = xmin + 1; x < xmax; x += space) {

            v.emplace_back(x, ymin, layer);
            v.emplace_back(x, ymax, layer);
        }
        for (int y = ymin; y <= ymax; y += space) {
            v.emplace_back(xmax, y, layer);
            v.emplace_back(xmin, y, layer);
        }

        return v;
    }

    void adjustEdgeCapacity(NTHUR::RoutingRegion& rr, const int value, const double probability, const int layer) {
        for (int x = xmin; x <= xmax - 1; ++x) {
            for (int y = ymin; y <= ymax; ++y) {
                if (dis(gen) < probability) {
                    rr.adjustEdgeCapacity(x, y, layer, x + 1, y, layer, value);
                }
            }
        }
        for (int x = xmin; x <= xmax; ++x) {
            for (int y = ymin; y <= ymax - 1; ++y) {
                if (dis(gen) < probability) {
                    rr.adjustEdgeCapacity(x, y, layer, x, y + 1, layer, value);
                }
            }
        }
    }
//private:
    int xmin;
    int xmax;
    int ymin;
    int ymax;
    int layer;
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<> dis;
};

#endif /* GLOBALROUTER_H_ */

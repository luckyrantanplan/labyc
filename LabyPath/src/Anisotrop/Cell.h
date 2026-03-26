/*
 * Cell.h
 *
 *  Created on: Feb 5, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_CELL_H_
#define ANISOTROP_CELL_H_

#include <cstddef>
#include <vector>

#include "../protoc/AllConfig.pb.h"
#include "../basic/RandomUniDist.h"
#include "../Ribbon.h"
#include "../GeomData.h"
#include "Net.h"

namespace laby {
namespace aniso {

class Cell {

public:
    Cell(const proto::Cell& config, Arrangement_2& arr, const Ribbon& limit);

    void drawRectOutline(const CGAL::Bbox_2& bbox, const double quantity, const double thickness, const double raylength);

    const std::vector<Vertex*>& vertices() const { return listVertex; }

    void createRandomPin(const CGAL::Bbox_2& bbox, const std::size_t maxPin);
    void createRandomPinOnExistingVerticesOnly();
    void startNetWithRandomPin();

    double resolution() const;

    std::vector<Point_2> subdivide(const Polyline& pl);

    std::vector<aniso::Net>& nets() { return _nets; }

    const std::vector<aniso::Net>& nets() const { return _nets; }

    std::vector<Vertex*>::iterator selectRandomVertex() {
        std::vector<Vertex*>::iterator it = randomVertex.begin();
        std::advance(it, _random.select(0, randomVertex.size()));
        return it;
    }
    void removeRandomPoint(std::vector<Vertex*>::iterator& ite) {
        std::swap(*ite, randomVertex.back());
        randomVertex.resize(randomVertex.size() - 1u);
    }
    void insertPointAndConnect(const Point_2& point2);
    void selectNearestPoint(const Point_2& point2);
    void shuffleVertices();

    const Arrangement_2& arr() const { return _arr; }

    Arrangement_2& arr() { return _arr; }

    const CGAL::Bbox_2& bbox() const { return _bbox; }

    std::vector<Vertex*> randomVertex;
    std::vector<Vertex*> listVertex;

private:
    void createOutlinedNet(std::size_t begin, double thickness);
    const proto::Cell _config;
    CGAL::Bbox_2 _bbox;
    Arrangement_2& _arr;

    std::vector<aniso::Net> _nets;
    basic::RandomUniDist _random;
};

} /* namespace aniso */
} /* namespace laby */

#endif /* ANISOTROP_CELL_H_ */

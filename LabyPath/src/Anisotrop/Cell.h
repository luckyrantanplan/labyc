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

#include "../GeomData.h"
#include "../Ribbon.h"
#include "../basic/RandomUniDist.h"
#include "../protoc/AllConfig.pb.h"
#include "Net.h"

namespace laby::aniso {

struct RectOutlineConfig {
    double quantity = 0.0;
    double thickness = 0.0;
    double rayLength = 0.0;
};

class Cell {

  public:
    Cell(proto::Cell config, Arrangement_2& arr, const Ribbon& limit);

    void drawRectOutline(const CGAL::Bbox_2& bbox, RectOutlineConfig config);

    [[nodiscard]] auto vertices() const -> const std::vector<Vertex*>& {
        return _listVertex;
    }
    [[nodiscard]] auto randomVertices() const -> const std::vector<Vertex*>& {
        return _randomVertices;
    }

    static void createRandomPin(const CGAL::Bbox_2& bbox, std::size_t maxPin);
    void createRandomPinOnExistingVerticesOnly();
    void startNetWithRandomPin();

    [[nodiscard]] auto resolution() const -> double;

    [[nodiscard]] auto subdivide(const Polyline& polyline) const -> std::vector<Point_2>;

    auto nets() -> std::vector<aniso::Net>& {
        return _nets;
    }

    [[nodiscard]] auto nets() const -> const std::vector<aniso::Net>& {
        return _nets;
    }

    static auto selectRandomVertex() -> std::vector<Vertex*>::iterator {
        auto iterator = _randomVertices.begin();
        std::advance(iterator, _random.select(0, _randomVertices.size()));
        return iterator;
    }

    void removeRandomPoint(std::vector<Vertex*>::iterator& iterator) {
        std::swap(*iterator, _randomVertices.back());
        _randomVertices.resize(_randomVertices.size() - 1U);
    }

    void insertPointAndConnect(const Point_2& point2);
    void selectNearestPoint(const Point_2& point2);
    void shuffleVertices();

    [[nodiscard]] auto arr() const -> const Arrangement_2& {
        return *_arr;
    }

    auto arr() -> Arrangement_2& {
        return *_arr;
    }

    [[nodiscard]] auto bbox() const -> const CGAL::Bbox_2& {
        return _bbox;
    }

  private:
    static void createOutlinedNet(std::size_t begin, double thickness);
    proto::Cell _config;
    CGAL::Bbox_2 _bbox;
    Arrangement_2* _arr = nullptr;

    std::vector<Vertex*> _randomVertices{};
    std::vector<Vertex*> _listVertex{};

    std::vector<aniso::Net> _nets{};
    basic::RandomUniDist _random;
};

} /* namespace laby::aniso */

#endif /* ANISOTROP_CELL_H_ */

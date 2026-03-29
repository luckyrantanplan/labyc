/*
 * PolyVertexList.h
 *
 *  Created on: Jun 5, 2018
 *      Author: florian
 */

#ifndef POLYVERTEX_H_
#define POLYVERTEX_H_

#include <cstdint>
#include <iostream>
#include <vector>

#include "GeomData.h"
#include "Polyline.h"

namespace laby {

class PolyVertex {
  public:
    [[nodiscard]] auto number() const -> int32_t {
        return _number;
    }

    [[nodiscard]] auto vertexList() const -> const std::vector<Vertex*>& {
        return _vertexList;
    }
    auto vertexList() -> std::vector<Vertex*>& {
        return _vertexList;
    }

    explicit PolyVertex(const int32_t number = 0, const std::vector<Vertex*>& vertexList = {})
        : _number{number}, _vertexList{vertexList} {}

    void print(std::ostream& outputStream) const {
        outputStream << " number " << _number << " vertex size " << _vertexList.size();
    }

    [[nodiscard]] auto polyline() const -> Polyline {
        Polyline polyline(_number);
        for (Vertex* vertex : _vertexList) {
            polyline.points().emplace_back(vertex->point());
        }
        return polyline;
    }

  private:
    int32_t _number = 0;
    std::vector<Vertex*> _vertexList{};
};

} /* namespace laby */

#endif /* POLYVERTEX_H_ */

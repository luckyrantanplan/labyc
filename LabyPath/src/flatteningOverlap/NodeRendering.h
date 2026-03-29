/*
 * NodeRendering.h
 *
 *  Created on: Mar 13, 2018
 *      Author: florian
 */

#ifndef NODERENDERING_H_
#define NODERENDERING_H_

#include <cstdint>
#include <vector>

#include "../GeomData.h"
#include "../OrientedRibbon.h"
#include "../PolyConvex.h"
#include "../basic/AugmentedPolygonSet.h"

#include "Node.h"

namespace laby {

class NodeOverlap {

  public:
    [[nodiscard]] auto nodes() const -> const std::vector<Node*>& {
        return _nodes;
    }

    auto nodes() -> std::vector<Node*>& {
        return _nodes;
    }

    auto sortNode() -> void;
    auto render(OrientedRibbon& orientedRibbon,
                const std::vector<PolyConvex>& polyConvexList) -> void;

  private:
    auto addIdToPolygon(const std::vector<PolyConvex>& polyConvexList) -> void;

    std::vector<Node*> _nodes{};
};

class NodeRendering {
  public:
    static auto render(OrientedRibbon& orientedRibbon, std::vector<Node>& nodes,
                       const std::vector<PolyConvex>& polyConvexList) -> void;
};

} /* namespace laby */

#endif /* NODERENDERING_H_ */

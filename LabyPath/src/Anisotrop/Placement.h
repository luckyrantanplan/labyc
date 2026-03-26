/*
 * Placement.h
 *
 *  Created on: Jun 7, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_PLACEMENT_H_
#define ANISOTROP_PLACEMENT_H_

#include <cstdint>
#include <cstddef>
#include <vector>

#include "../protoc/AllConfig.pb.h"
#include "../GeomData.h"
#include "../PolyConvex.h"
#include "../PolyVertex.h"
#include "Routing.h"

namespace laby {
namespace aniso {
class Cell;

class Placement {

public:

    Placement(const proto::Placement& config, const proto::Filepaths& filepaths);

private:
    void statistics(const Arrangement_2& arr);
    bool crossOtherNet(const Vertex& v, int32_t netId);
    Routing create_route(Cell& cell);
    std::vector<PolyConvex> refine_path(Cell& cell, const std::vector<PolyConvex>& initialConvex, std::vector<PolyConvex>& polyConvexList);
    std::vector<PolyVertex> explodeGraph(const std::vector<PolyConvex>& initialConvex, Cell& cell);

    const proto::Placement _config;
}
;
} /* namespace aniso */
} /* namespace laby */

#endif /* ANISOTROP_PLACEMENT_H_ */

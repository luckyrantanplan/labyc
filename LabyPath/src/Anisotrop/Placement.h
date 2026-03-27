/*
 * Placement.h
 *
 *  Created on: Jun 7, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_PLACEMENT_H_
#define ANISOTROP_PLACEMENT_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../GeomData.h"
#include "../PolyConvex.h"
#include "../PolyVertex.h"
#include "../protoc/AllConfig.pb.h"
#include "Routing.h"

namespace laby::aniso {
class Cell;

class Placement {

  public:
    Placement(proto::Placement config, const proto::Filepaths& filepaths);

  private:
    void statistics(const Arrangement_2& arr);
    static auto crossOtherNet(const Vertex& vertex, int32_t netId) -> bool;
    auto createRoute(Cell& cell) -> Routing;
    auto refinePath(Cell& cell, const std::vector<PolyConvex>& initialConvex,
                    std::vector<PolyConvex>& polyConvexList) -> std::vector<PolyConvex>;
    static auto explodeGraph(const std::vector<PolyConvex>& initialConvex,
                             Cell& cell) -> std::vector<PolyVertex>;

    proto::Placement _config;
};

} // namespace laby::aniso

#endif /* ANISOTROP_PLACEMENT_H_ */

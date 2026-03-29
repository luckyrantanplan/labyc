/*
 * Routing.h
 *
 *  Created on: Mar 27, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_ROUTING_H_
#define ANISOTROP_ROUTING_H_

#include <cstddef>
#include <cstdint>
#include <vector>

#include "../GeomData.h"
#include "../PolyConvex.h"
#include "../basic/RandomInteger.h"
#include "../protoc/AllConfig.pb.h"
#include "Net.h"
#include "QueueCost.h"
#include "QueueElement.h"
#include "SpatialIndex.h"

namespace laby::basic {
class LinearGradient;
} // namespace laby::basic

namespace laby::aniso {

class Routing {

  public:
    Routing(Arrangement_2& arr, proto::RoutingCost config);

    [[nodiscard]] auto polyConvexList() const -> const std::vector<PolyConvex>& {
        return _convexList;
    }

    auto polyConvexList() -> std::vector<PolyConvex>& {
        return _convexList;
    }

    static void connectTwoPinPath(const std::vector<aniso::Net>& nets,
                                  const SpatialIndex& spatialIndex,
                                  std::vector<PolyConvex>& convexList);

    auto findRoute(Net& net) -> bool;

    static void createMaze();
    static void connectMaze(std::vector<PolyConvex>& polyConvexList);

  private:
    static void commitNewPath(const int32_t& targetId, Net& net);
    proto::RoutingCost _config;
    Arrangement_2* _arr = nullptr;
    std::vector<PolyConvex> _convexList{};
    std::vector<QueueElement> _edgesQList{};
    SpatialIndex _spatialIndex;

    basic::RandomInteger<int32_t> _random;
};

} // namespace laby::aniso

#endif /* ANISOTROP_ROUTING_H_ */

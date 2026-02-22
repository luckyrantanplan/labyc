/*
 * Routing.h
 *
 *  Created on: Mar 27, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_ROUTING_H_
#define ANISOTROP_ROUTING_H_

#include <bits/stdint-intn.h>
#include <cstddef>
#include <vector>

#include "../protoc/AllConfig.pb.h"
#include "../GeomData.h"
#include "../PolyConvex.h"
#include "../basic/RandomInteger.h"
#include "Net.h"
#include "QueueCost.h"
#include "QueueElement.h"
#include "SpatialIndex.h"

namespace laby {
namespace basic {
class LinearGradient;
} /* namespace basic */

namespace aniso {

class Routing {

public:

    Routing(Arrangement_2& arr, const proto::RoutingCost& config);

    const std::vector<PolyConvex>& polyConvexList() const {
        return _convexList;
    }

    std::vector<PolyConvex>& polyConvexList() {
        return _convexList;
    }

    static void connectTwoPinPath(const std::vector<aniso::Net>& nets, const SpatialIndex& si, std::vector<PolyConvex>& _convexList);

    bool findRoute(Net & net);

    void createMaze();
    static void connectMaze(std::vector<PolyConvex>& _convexList);
private:
    void commitNewPath(const int32_t& targetId, Net& net);
    const proto::RoutingCost _config;
    Arrangement_2& _arr;
    std::vector<PolyConvex> _convexList;
    std::vector<QueueElement> edgesQList;
    SpatialIndex si;

    basic::RandomInteger<int32_t> _random;
};

} /* namespace aniso */
} /* namespace laby */

#endif /* ANISOTROP_ROUTING_H_ */

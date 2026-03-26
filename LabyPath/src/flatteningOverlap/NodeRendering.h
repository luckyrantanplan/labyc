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

#include "../basic/AugmentedPolygonSet.h"
#include "../GeomData.h"
#include "../PolyConvex.h"
#include "../OrientedRibbon.h"

#include "Node.h"

namespace laby {

struct NodeOverlap {

    std::vector<Node*> _nodes;

    void sortNode();
    void render(OrientedRibbon& oribbon, const std::vector<PolyConvex>& polyConvexList);

private:
    void addIdToPolygon(const std::vector<PolyConvex>& polyConvexList);
    bool testSeg(int32_t index, const basic::HalfedgeNode& he);
    bool has_face(basic::Arrangement_2Node& res);
};

class NodeRendering {
public:
    static void render(OrientedRibbon& oribbon, std::vector<Node>& nodes, const std::vector<PolyConvex>& polyConvexList);
};

} /* namespace laby */

#endif /* NODERENDERING_H_ */

/*
 * QueueCost.h
 *
 *  Created on: Apr 26, 2018
 *      Author: florian
 */

#ifndef ANISOTROP_QUEUECOST_H_
#define ANISOTROP_QUEUECOST_H_

#include <cstdint>
#include <iostream>
#include <tuple>
#include <unordered_map>
#include <unordered_set>

#include "../GeomData.h"

namespace laby {
namespace aniso {

class QueueCost {
public:
    int32_t distance = -1;           //Distance from source to current element
    int32_t via_num = 0;            //Via count from source to current element
    int32_t congestion = 0;

    int32_t randomization = 0;
    std::unordered_set<int32_t> memory_source;
    std::unordered_set<int32_t> future_memory_source;
    std::unordered_set<int32_t> memory_target;
    std::unordered_set<int32_t> future_memory_target;

    auto tie() const {
        return std::tie(congestion, //

                via_num, //
                distance, //
                randomization);

    }

    void print(std::ostream& os) const {
        os << " congestion " << congestion;
        os << " via_num " << via_num;
        os << "distance " << distance;
        os << " randomization " << randomization;

    }

    bool operator >(const QueueCost &rhs) const {
        return tie() > rhs.tie();

    }

    bool operator <(const QueueCost &rhs) const {
        return tie() < rhs.tie();

    }
};

} /* namespace aniso */
} /* namespace laby */

#endif /* ANISOTROP_QUEUECOST_H_ */

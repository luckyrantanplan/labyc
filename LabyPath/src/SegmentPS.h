/*
 * SegmentPS.h
 *
 *  Created on: Jul 24, 2018
 *      Author: florian
 */

#ifndef SEGMENTPS_H_
#define SEGMENTPS_H_

#include <CGAL/Point_set_2.h>

#include "GeomData.h"

namespace laby {

/// Segment between two Point_set_2 vertices, ordered by squared distance.
class SegmentPS {

public:
    using PS = CGAL::Point_set_2<Kernel>;

    SegmentPS(PS::Vertex* source, PS::Vertex* target) :
            _source(source), _target(target) {
        _squared_distance = CGAL::squared_distance(_source->point(), _target->point());
    }

    auto operator <(const SegmentPS &rhs) const -> bool {
        return _squared_distance < rhs._squared_distance;
    }

    [[nodiscard]] auto squaredDistance() const -> Kernel::FT { return _squared_distance; }
    [[nodiscard]] auto source() const -> PS::Vertex* { return _source; }
    [[nodiscard]] auto target() const -> PS::Vertex* { return _target; }

private:
    Kernel::FT _squared_distance;
    PS::Vertex* _source;
    PS::Vertex* _target;

};

} /* namespace laby */

#endif /* SEGMENTPS_H_ */

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

class SegmentPS {

public:
    typedef CGAL::Point_set_2<Kernel> PS;

    SegmentPS(PS::Vertex* v1, PS::Vertex* v2) :
            _v1(v1), _v2(v2) {
        _length = CGAL::squared_distance(_v1->point(), _v2->point());

    }

    bool operator <(const SegmentPS &rhs) const {

        return _length < rhs._length;

    }

    Kernel::FT _length;

    PS::Vertex* _v1;
    PS::Vertex* _v2;

};

} /* namespace laby */

#endif /* SEGMENTPS_H_ */

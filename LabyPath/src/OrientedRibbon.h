/*
 * OrientedRibbon.h
 *
 *  Created on: Sep 11, 2018
 *      Author: florian
 */

#ifndef ORIENTEDRIBBON_H_
#define ORIENTEDRIBBON_H_

#include "Ribbon.h"

namespace laby {

class OrientedRibbon {

public:
    void addCW(const Kernel::Segment_2& segment);
    void addCCW(const Kernel::Segment_2& segment);

    [[nodiscard]] std::vector<Ribbon> getResult() const;
    [[nodiscard]] Ribbon createOrientedRibbon() const;

private:
    static bool isRight(const Kernel::Point_2& p1, const Kernel::Point_2& p2);
    static Ribbon createRibbonOfEdge(const Arrangement_2& arr);
    static void setPolylineOrientation(const Halfedge& he, Polyline& poly);

    std::vector<Kernel::Segment_2> _left;
    std::vector<Kernel::Segment_2> _right;
};

} /* namespace laby */

#endif /* ORIENTEDRIBBON_H_ */

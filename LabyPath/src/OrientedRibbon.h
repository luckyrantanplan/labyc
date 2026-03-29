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
    auto addCW(const Kernel::Segment_2& segment) -> void;
    auto addCCW(const Kernel::Segment_2& segment) -> void;

    [[nodiscard]] auto getResult() const -> std::vector<Ribbon>;
    [[nodiscard]] auto createOrientedRibbon() const -> Ribbon;

  private:
    std::vector<Kernel::Segment_2> _left;
    std::vector<Kernel::Segment_2> _right;
};

} /* namespace laby */

#endif /* ORIENTEDRIBBON_H_ */

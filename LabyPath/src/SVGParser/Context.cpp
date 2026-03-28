/*
 * Context.cpp
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#include "Context.h"
#include "SVGParser/Stylable.h"


namespace laby::svgp {

BaseContext::BaseContext() {
    setRibbonRef(&ribbonStorage());
}

BaseContext::BaseContext(BaseContext& parent)
    : Stylable(parent) {
    setRibbonRef(parent.ribbonRef());
}

BaseContext::BaseContext(const BaseContext& parent)
    : Stylable(parent) {
    setRibbonRef(parent.ribbonRef());
}
} /* namespace laby::svgp */

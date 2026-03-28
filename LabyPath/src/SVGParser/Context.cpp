/*
 * Context.cpp
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#include "Context.h"
#include "SVGParser/Stylable.h"


namespace laby::svgp {

BaseContext::BaseContext() : _vectRibbonRef(&_vectRibbon) {}

BaseContext::BaseContext(BaseContext& parent)
    : Stylable(parent), _vectRibbonRef(parent._vectRibbonRef) {}

BaseContext::BaseContext(const BaseContext& parent)
    : Stylable(parent), _vectRibbonRef(parent._vectRibbonRef) {}
} /* namespace laby::svgp */

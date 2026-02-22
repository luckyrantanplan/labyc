/*
 * Context.cpp
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#include "Context.h"

#include <boost/mpl/insert.hpp>
#include <svgpp/attribute_dispatcher.hpp>
#include <svgpp/definitions.hpp>
#include <svgpp/document_traversal.hpp>
#include <svgpp/policy/marker_events.hpp>
#include <svgpp/traits/element_groups.hpp>
#include <iostream>
#include <vector>

namespace laby {
namespace svgp {

BaseContext::BaseContext() :
        _vectRibbonRef(_vectRibbon) {
}

BaseContext::BaseContext(BaseContext & parent) :
        _vectRibbonRef(parent._vectRibbonRef) {

}
} /* namespace svgp */
} /* namespace laby */

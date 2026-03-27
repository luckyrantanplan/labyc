/*
 * UseContext.cpp
 *
 *  Created on: Jun 18, 2018
 *      Author: florian
 */

#include "UseContext.h"

namespace laby::svgp {

void UseContext::on_exit_element() { 
    if (xml_element_t element = FindCurrentDocumentElementById(_fragmentId)) {
        // TODO: Check for cyclic references
        // TODO: Apply translate transform (_x, _y)
        document_traversal_t::load_referenced_element<
            svgpp::referencing_element<svgpp::tag::element::use_>,             //
            svgpp::expected_elements<svgpp::traits::reusable_elements>,        //
            svgpp::processed_elements<boost::mpl::insert<processed_elements_t, //
                                                         svgpp::tag::element::symbol>::type>>::
            load(element, *this);
    } else {
        std::cerr << "Element referenced by 'use' not found\n";
    }
}

} /* namespace laby::svgp */

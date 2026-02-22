/*
 * Loader.cpp
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#include "Loader.h"

#include "UseContext.h"

#include <iostream>

namespace laby {
namespace svgp {

Loader::Loader(const std::string& filename) {

    rapidxml_ns::file<> xml_file(filename.data());
    std::cout << "load svg" << std::endl;
    rapidxml_ns::xml_document<char> doc;
    doc.parse<rapidxml_ns::parse_no_string_terminators>(xml_file.data());
    if (rapidxml_ns::xml_node<> * svg_element = doc.first_node("svg")) {

        BaseContext context;
        document_traversal_t::load_document(svg_element, context);

        _ribList = context.getRibbon();
        _viewBox = context.getViewbox();
        std::cout << "context.getRibbon() " << _ribList.size() << std::endl;

    } else {
        std::cout << "invalid SVG file " << std::endl;
    }

}

} /* namespace svgp */
} /* namespace laby */

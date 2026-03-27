/*
 * Loader.cpp
 *
 *  Created on: Jun 15, 2018
 *      Author: florian
 */

#include "Loader.h"

#include "UseContext.h"

#include <iostream>

namespace laby::svgp {

Loader::Loader(const std::string& filename) {

    rapidxml_ns::file<> xmlFile(filename.data());
    std::cout << "load svg\n";
    rapidxml_ns::xml_document<char> doc;
    doc.parse<rapidxml_ns::parse_no_string_terminators>(xmlFile.data());
    if (rapidxml_ns::xml_node<>* svgElement = doc.first_node("svg")) {

        BaseContext context;
        document_traversal_t::load_document(svgElement, context);

        _ribList = context.getRibbon();
        _viewBox = context.getViewbox();
        std::cout << "context.getRibbon() " << _ribList.size() << '\n';

    } else {
        std::cout << "invalid SVG file \n";
    }
}

} /* namespace laby::svgp */

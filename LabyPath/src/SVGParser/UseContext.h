/*
 * UseContext.h
 *
 *  Created on: Jun 18, 2018
 *      Author: florian
 */

#ifndef SVGPARSER_USECONTEXT_H_
#define SVGPARSER_USECONTEXT_H_

#include <rapidxml_ns/rapidxml_ns.hpp>
#include <rapidxml_ns/rapidxml_ns_utils.hpp>
#include <svgpp/policy/xml/rapidxml_ns.hpp>
#include <svgpp/svgpp.hpp>
#include <boost/variant.hpp>

#include "ShapeContext.h"
#include "Context.h"

namespace laby {
namespace svgp {

typedef rapidxml_ns::xml_node<> const * xml_element_t;

class UseContext: public BaseContext {
public:
    UseContext(BaseContext const & parent) {
    }

    boost::optional<double> const & width() const {
        return width_;
    }
    boost::optional<double> const & height() const {
        return height_;
    }

    using BaseContext::set;

    template<class IRI>
    void set(svgpp::tag::attribute::xlink::href, svgpp::tag::iri_fragment, IRI const & fragment) {
        fragment_id_.assign(boost::begin(fragment), boost::end(fragment));
    }

    template<class IRI>
    void set(svgpp::tag::attribute::xlink::href, IRI const & fragment) {
        std::cerr << "External references aren't supported\n";
    }

    void set(svgpp::tag::attribute::x, double val) {
        x_ = val;
    }

    void set(svgpp::tag::attribute::y, double val) {
        y_ = val;
    }

    void set(svgpp::tag::attribute::width, double val) {
        width_ = val;
    }

    void set(svgpp::tag::attribute::height, double val) {
        height_ = val;
    }

    void on_exit_element();

    xml_element_t FindCurrentDocumentElementById(std::string const &) {
        return NULL;
    }

private:
    std::string fragment_id_;
    double x_ = 0;
    double y_ = 0;
    boost::optional<double> width_, height_;
};

class ReferencedSymbolOrSvgContext: public BaseContext {
public:
    ReferencedSymbolOrSvgContext(UseContext & referencing) :
            BaseContext(referencing), referencing_(referencing) {
    }

    void get_reference_viewport_size(double & width, double & height) {
        if (referencing_.width())
            width = *referencing_.width();
        if (referencing_.height())
            height = *referencing_.height();
    }

private:
    UseContext & referencing_;
};

struct ChildContextFactories {
    template<class ParentContext, class ElementTag, class Enable = void>
    struct apply {
        // Default definition handles "svg" and "g" elements
        typedef svgpp::factory::context::on_stack<BaseContext> type;
    };
};

// This specialization handles all shape elements (elements from svgpp::traits::shape_elements sequence)
template<class ElementTag>
struct ChildContextFactories::apply<BaseContext, ElementTag, typename boost::enable_if<boost::mpl::has_key<svgpp::traits::shape_elements, ElementTag> >::type> {
    typedef svgpp::factory::context::on_stack<ShapeContext> type;
};

template<>
struct ChildContextFactories::apply<BaseContext, svgpp::tag::element::use_> {
    typedef svgpp::factory::context::on_stack<UseContext> type;
};

// Elements referenced by 'use' element
template<>
struct ChildContextFactories::apply<UseContext, svgpp::tag::element::svg, void> {
    typedef svgpp::factory::context::on_stack<ReferencedSymbolOrSvgContext> type;
};

template<>
struct ChildContextFactories::apply<UseContext, svgpp::tag::element::symbol, void> {
    typedef svgpp::factory::context::on_stack<ReferencedSymbolOrSvgContext> type;
};

template<class ElementTag>
struct ChildContextFactories::apply<UseContext, ElementTag, void>: ChildContextFactories::apply<BaseContext, ElementTag> {
};

template<class ElementTag>
struct ChildContextFactories::apply<ReferencedSymbolOrSvgContext, ElementTag, void>: ChildContextFactories::apply<BaseContext, ElementTag> {
};

typedef boost::mpl::set<
// SVG Structural Elements
        svgpp::tag::element::svg, svgpp::tag::element::g, svgpp::tag::element::use_,
        // SVG Shape Elements
        svgpp::tag::element::circle, svgpp::tag::element::ellipse, svgpp::tag::element::line, svgpp::tag::element::path, svgpp::tag::element::polygon, svgpp::tag::element::polyline,
        svgpp::tag::element::rect>::type processed_elements_t;

// Joining some sequences from traits namespace with chosen attributes
typedef boost::mpl::fold< //
        boost::mpl::protect<boost::mpl::joint_view<svgpp::traits::shapes_attributes_by_element, svgpp::traits::viewport_attributes> >, //
        boost::mpl::set<svgpp::tag::attribute::transform, //
                svgpp::tag::attribute::stroke, //
                svgpp::tag::attribute::stroke_width, //

                svgpp::tag::attribute::fill, //

                svgpp::tag::attribute::fill_rule, //

                boost::mpl::pair<svgpp::tag::element::use_, svgpp::tag::attribute::xlink::href> >::type, boost::mpl::insert<boost::mpl::_1, boost::mpl::_2> >::type processed_attributes_t;

struct path_policy: svgpp::policy::path::no_shorthands {
    static const bool arc_as_cubic_bezier = true;
};

typedef svgpp::document_traversal<svgpp::processed_elements<processed_elements_t>, //
        svgpp::processed_attributes<processed_attributes_t>, //
        svgpp::path_policy<path_policy>, //
        svgpp::viewport_policy<svgpp::policy::viewport::as_transform>, //
        svgpp::context_factories<ChildContextFactories>, //
        svgpp::markers_policy<svgpp::policy::markers::calculate_always> > document_traversal_t;

} /* namespace svgp */
} /* namespace laby */

#endif /* SVGPARSER_USECONTEXT_H_ */

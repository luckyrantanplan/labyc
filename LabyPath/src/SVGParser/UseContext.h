/*
 * UseContext.h
 *
 *  Created on: Jun 18, 2018
 *      Author: florian
 */

#ifndef SVGPARSER_USECONTEXT_H_
#define SVGPARSER_USECONTEXT_H_

#include "Context.h"
#include "ShapeContext.h"
#include <optional>
#include <rapidxml_ns/rapidxml_ns.hpp>
#include <rapidxml_ns/rapidxml_ns_utils.hpp>
#include <svgpp/policy/xml/rapidxml_ns.hpp>
#include <svgpp/svgpp.hpp>

namespace laby::svgp {

using xml_element_t = rapidxml_ns::xml_node<> const*;

class UseContext : public BaseContext {
  public:
    explicit UseContext(const BaseContext& parent) : BaseContext(parent) {}

    [[nodiscard]] auto width() const -> const std::optional<double>& {
        return _width;
    }

    [[nodiscard]] auto height() const -> const std::optional<double>& {
        return _height;
    }

    using BaseContext::set;

    template <class IRI>
    void set(svgpp::tag::attribute::xlink::href attributeTag, svgpp::tag::iri_fragment iriTag,
             const IRI& fragment) {
        static_cast<void>(attributeTag);
        static_cast<void>(iriTag);
        _fragmentId.assign(std::begin(fragment), std::end(fragment));
    }

    template <class IRI>
    void set(svgpp::tag::attribute::xlink::href attributeTag, const IRI& fragment) {
        static_cast<void>(attributeTag);
        static_cast<void>(fragment);
        std::cerr << "External references aren't supported\n";
    }

    void set(svgpp::tag::attribute::x attributeTag, double value) {
        static_cast<void>(attributeTag);
        _x = value;
    }

    void set(svgpp::tag::attribute::y attributeTag, double value) {
        static_cast<void>(attributeTag);
        _y = value;
    }

    void set(svgpp::tag::attribute::width attributeTag, double value) {
        static_cast<void>(attributeTag);
        _width = value;
    }

    void set(svgpp::tag::attribute::height attributeTag, double value) {
        static_cast<void>(attributeTag);
        _height = value;
    }

    void on_exit_element();

    // SVG++ discovers this hook by exact name.

    static auto FindCurrentDocumentElementById(const std::string& fragmentId) -> xml_element_t {
        static_cast<void>(fragmentId);
        return nullptr;
    }

  private:
    std::string _fragmentId;
    double _x = 0;
    double _y = 0;
    std::optional<double> _width;
    std::optional<double> _height;
};

class ReferencedSymbolOrSvgContext : public BaseContext {
  public:
    explicit ReferencedSymbolOrSvgContext(UseContext& referencing)
        : BaseContext(static_cast<const BaseContext&>(referencing)), _referencing(&referencing) {}

    // SVG++ discovers this hook by exact name.

    void get_reference_viewport_size(double& width, double& height) {
        if (_referencing->width()) {
            width = *_referencing->width();
        }
        if (_referencing->height()) {
            height = *_referencing->height();
        }
    }

  private:
    UseContext* _referencing;
};

// SVG++ policy customization uses fixed trait names such as 'apply' and 'arc_as_cubic_bezier'.

struct ChildContextFactories {
    template <class ParentContext, class ElementTag, class Enable = void> struct apply {
        // Default definition handles "svg" and "g" elements
        using type = svgpp::factory::context::on_stack<BaseContext>;
    };
};

// This specialization handles all shape elements (elements from svgpp::traits::shape_elements
// sequence)
template <class ElementTag>
struct ChildContextFactories::apply<BaseContext, ElementTag,
                                    typename boost::enable_if<boost::mpl::has_key<
                                        svgpp::traits::shape_elements, ElementTag>>::type> {
    using type = svgpp::factory::context::on_stack<ShapeContext>;
};

template <> struct ChildContextFactories::apply<BaseContext, svgpp::tag::element::use_> {
    using type = svgpp::factory::context::on_stack<UseContext>;
};

// Elements referenced by 'use' element
template <> struct ChildContextFactories::apply<UseContext, svgpp::tag::element::svg, void> {
    using type = svgpp::factory::context::on_stack<ReferencedSymbolOrSvgContext>;
};

template <> struct ChildContextFactories::apply<UseContext, svgpp::tag::element::symbol, void> {
    using type = svgpp::factory::context::on_stack<ReferencedSymbolOrSvgContext>;
};

template <class ElementTag>
struct ChildContextFactories::apply<UseContext, ElementTag, void>
    : ChildContextFactories::apply<BaseContext, ElementTag> {};

template <class ElementTag>
struct ChildContextFactories::apply<ReferencedSymbolOrSvgContext, ElementTag, void>
    : ChildContextFactories::apply<BaseContext, ElementTag> {};

using processed_elements_t = boost::mpl::set<
    // SVG Structural Elements
    svgpp::tag::element::svg, svgpp::tag::element::g, svgpp::tag::element::use_,
    // SVG Shape Elements
    svgpp::tag::element::circle, svgpp::tag::element::ellipse, svgpp::tag::element::line,
    svgpp::tag::element::path, svgpp::tag::element::polygon, svgpp::tag::element::polyline,
    svgpp::tag::element::rect>::type;

// Joining some sequences from traits namespace with chosen attributes
using processed_attributes_t = typename boost::mpl::fold< //
    boost::mpl::protect<boost::mpl::joint_view<svgpp::traits::shapes_attributes_by_element,
                                               svgpp::traits::viewport_attributes>>, //
    boost::mpl::set<
        svgpp::tag::attribute::transform,    //
        svgpp::tag::attribute::stroke,       //
        svgpp::tag::attribute::stroke_width, //

        svgpp::tag::attribute::fill, //

        svgpp::tag::attribute::fill_rule, //

        boost::mpl::pair<svgpp::tag::element::use_, svgpp::tag::attribute::xlink::href>>::type,
    boost::mpl::insert<boost::mpl::_1, boost::mpl::_2>>::type;

struct PathPolicy : svgpp::policy::path::no_shorthands {
    static constexpr bool arc_as_cubic_bezier = true;
};

using document_traversal_t =
    svgpp::document_traversal<svgpp::processed_elements<processed_elements_t>,               //
                              svgpp::processed_attributes<processed_attributes_t>,           //
                              svgpp::path_policy<PathPolicy>,                                //
                              svgpp::viewport_policy<svgpp::policy::viewport::as_transform>, //
                              svgpp::context_factories<ChildContextFactories>,               //
                              svgpp::markers_policy<svgpp::policy::markers::calculate_always>>;

} /* namespace laby::svgp */

#endif /* SVGPARSER_USECONTEXT_H_ */

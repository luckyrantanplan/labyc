/*
 * Stylable.h
 *
 *  Created on: Jun 25, 2018
 *      Author: florian
 */

#ifndef SVGPARSER_STYLABLE_H_
#define SVGPARSER_STYLABLE_H_

#include <boost/optional.hpp>
#include <boost/variant.hpp>

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <utility>
#include <vector>
#include <svgpp/svgpp.hpp>
#include "Common.h"


namespace laby::svgp {

enum line_cap_e { butt_cap, square_cap, round_cap };

//------------------------------------------------------------line_join_e
enum line_join_e { miter_join = 0, miter_join_revert = 1, round_join = 2, bevel_join = 3, miter_join_round = 4 };

//-----------------------------------------------------------inner_join_e
enum inner_join_e { inner_bevel, inner_miter, inner_jag, inner_round };
using color_t = int;
using SolidPaint = boost::variant<svgpp::tag::value::none, svgpp::tag::value::currentColor, color_t>;
struct IRIPaint {
    explicit IRIPaint(std::string  fragment, boost::optional<SolidPaint> const& fallback = boost::optional<SolidPaint>())
        : fragment_(std::move(fragment)), fallback_(fallback) {}

    std::string fragment_;
    boost::optional<SolidPaint> fallback_;
};
using Paint = boost::variant<SolidPaint, IRIPaint>;
using EffectivePaint = boost::variant<svgpp::tag::value::none, color_t>;

struct InheritedStyle {
    InheritedStyle() = default;
    color_t blackColor = 0;
    color_t color_ = blackColor;
    Paint fill_paint_ = blackColor;
    Paint stroke_paint_ = svgpp::tag::value::none();
    number_t stroke_opacity_ = 1.0;
    number_t fill_opacity_ = 1.0;
    bool nonzero_fill_rule_ = true;

    number_t stroke_width_ = 1.0;

    line_cap_e line_cap_ = butt_cap;
    line_join_e line_join_ = miter_join;

    number_t miterlimit_ = 4.0;

    std::vector<number_t> stroke_dasharray_;
    number_t stroke_dashoffset_ = 0;
    boost::optional<std::string> marker_start_, marker_mid_, marker_end_;
};

struct NoninheritedStyle {
    NoninheritedStyle() = default;

    number_t opacity_ = 1.0;
    bool display_ = true;
    boost::optional<std::string> mask_fragment_, clip_path_fragment_;
    boost::optional<std::string> filter_;
    bool overflow_clip_ = true;
};

struct Style : InheritedStyle, NoninheritedStyle {
    struct InheritTag {};

    Style() = default;

    Style(Style const& src, InheritTag /*unused*/) : InheritedStyle(src) {}

    [[nodiscard]] auto getEffectivePaint(Paint const& paint) const -> EffectivePaint {
        SolidPaint const* solidPaint = nullptr;
        if (IRIPaint const* iri = boost::get<IRIPaint>(&paint)) {
            // do not manage Gradient yet
            if (iri->fallback_) {
                solidPaint = &*iri->fallback_;
            } else {
                throw std::runtime_error("Can't find paint server");
}
        }
        else {
            solidPaint = boost::get<SolidPaint>(&paint);
}
        if (boost::get<svgpp::tag::value::none>(solidPaint) != nullptr) {
            return svgpp::tag::value::none();
}
        if (boost::get<svgpp::tag::value::currentColor>(solidPaint) != nullptr) {
            return color_;
}
        return boost::get<color_t>(*solidPaint);
    }
};

template <class AttributeTag> class PaintContext {
public:
    explicit PaintContext(Paint& paint) : paint_(paint) {}

    void set(AttributeTag /*unused*/, svgpp::tag::value::none /*unused*/) { paint_ = svgpp::tag::value::none(); }

    void set(AttributeTag /*unused*/, svgpp::tag::value::currentColor /*unused*/) { paint_ = svgpp::tag::value::currentColor(); }

    void set(AttributeTag /*unused*/, color_t color, svgpp::tag::skip_icc_color  /*unused*/= svgpp::tag::skip_icc_color()) { paint_ = color; }

    template <class IRI> void set(AttributeTag /*tag*/, IRI const& /*iri*/) { throw std::runtime_error("Non-local references aren't supported"); }

    template <class IRI> void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    template <class IRI> void set(AttributeTag tag, IRI const& /*unused*/, svgpp::tag::value::none val) { set(tag, val); }

    template <class IRI> void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment, svgpp::tag::value::none val) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)), boost::optional<SolidPaint>(val));
    }

    template <class IRI> void set(AttributeTag tag, IRI const& /*unused*/, svgpp::tag::value::currentColor val) { set(tag, val); }

    template <class IRI> void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment, svgpp::tag::value::currentColor val) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)), boost::optional<SolidPaint>(val));
    }

    template <class IRI> void set(AttributeTag tag, IRI const& /*unused*/, color_t val, svgpp::tag::skip_icc_color  /*unused*/= svgpp::tag::skip_icc_color()) { set(tag, val); }

    template <class IRI>
    void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment, color_t val, svgpp::tag::skip_icc_color  /*unused*/= svgpp::tag::skip_icc_color()) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)), boost::optional<SolidPaint>(val));
    }

protected:
    Paint& paint_;
};

class Stylable : public PaintContext<svgpp::tag::attribute::stroke>, public PaintContext<svgpp::tag::attribute::fill> {
public:
    using stroke_paint = PaintContext<svgpp::tag::attribute::stroke>;
    using fill_paint = PaintContext<svgpp::tag::attribute::fill>;

    Stylable() : stroke_paint(_style.stroke_paint_), fill_paint(_style.fill_paint_) {}

    Stylable(Stylable const& src) : stroke_paint(_style.stroke_paint_), fill_paint(_style.fill_paint_), _style(src._style, Style::InheritTag()) {}

    using fill_paint::set;
    using stroke_paint::set;

    void set(svgpp::tag::attribute::display /*unused*/, svgpp::tag::value::none /*unused*/) { style().display_ = false; }

    void set(svgpp::tag::attribute::display /*unused*/, svgpp::tag::value::inherit /*unused*/) { style().display_ = _parentStyle.display_; }

    template <class ValueTag> void set(svgpp::tag::attribute::display /*unused*/, ValueTag /*unused*/) { style().display_ = true; }

    void set(svgpp::tag::attribute::color /*unused*/, color_t val) { style().color_ = val; }

    void set(svgpp::tag::attribute::stroke_width /*unused*/, number_t val) { style().stroke_width_ = val; }

    void set(svgpp::tag::attribute::stroke_opacity /*unused*/, number_t val) { style().stroke_opacity_ = std::min(number_t(1), std::max(number_t(0), val)); }

    void set(svgpp::tag::attribute::fill_opacity /*unused*/, number_t val) { style().fill_opacity_ = std::min(number_t(1), std::max(number_t(0), val)); }

    void set(svgpp::tag::attribute::opacity /*unused*/, number_t val) { style().opacity_ = std::min(number_t(1), std::max(number_t(0), val)); }

    void set(svgpp::tag::attribute::opacity /*unused*/, svgpp::tag::value::inherit /*unused*/) { style().opacity_ = _parentStyle.opacity_; }

    void set(svgpp::tag::attribute::fill_rule /*unused*/, svgpp::tag::value::nonzero /*unused*/) { style().nonzero_fill_rule_ = true; }

    void set(svgpp::tag::attribute::fill_rule /*unused*/, svgpp::tag::value::evenodd /*unused*/) { style().nonzero_fill_rule_ = false; }

    void set(svgpp::tag::attribute::stroke_linecap /*unused*/, svgpp::tag::value::butt /*unused*/) { style().line_cap_ = butt_cap; }

    void set(svgpp::tag::attribute::stroke_linecap /*unused*/, svgpp::tag::value::round /*unused*/) { style().line_cap_ = round_cap; }

    void set(svgpp::tag::attribute::stroke_linecap /*unused*/, svgpp::tag::value::square /*unused*/) { style().line_cap_ = square_cap; }

    void set(svgpp::tag::attribute::stroke_linejoin /*unused*/, svgpp::tag::value::miter /*unused*/) { style().line_join_ = miter_join; }

    void set(svgpp::tag::attribute::stroke_linejoin /*unused*/, svgpp::tag::value::round /*unused*/) { style().line_join_ = round_join; }

    void set(svgpp::tag::attribute::stroke_linejoin /*unused*/, svgpp::tag::value::bevel /*unused*/) { style().line_join_ = bevel_join; }

    void set(svgpp::tag::attribute::stroke_miterlimit /*unused*/, number_t val)

    {
        style().miterlimit_ = val;
    }

    void set(svgpp::tag::attribute::stroke_dasharray /*unused*/, svgpp::tag::value::none /*unused*/) { style().stroke_dasharray_.clear(); }

    template <class Range> void set(svgpp::tag::attribute::stroke_dasharray /*unused*/, Range const& range) {
        style().stroke_dasharray_.assign(boost::begin(range), boost::end(range));
    }

    void set(svgpp::tag::attribute::stroke_dashoffset /*unused*/, number_t val) { style().stroke_dashoffset_ = val; }

    template <class IRI> void set(svgpp::tag::attribute::mask /*unused*/, IRI const& /*unused*/) { throw std::runtime_error("Non-local references aren't supported"); }

    template <class IRI> void set(svgpp::tag::attribute::mask /*unused*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        style().mask_fragment_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::mask /*unused*/, svgpp::tag::value::none /*val*/) { style().mask_fragment_.reset(); }

    void set(svgpp::tag::attribute::mask /*unused*/, svgpp::tag::value::inherit /*val*/) { style().mask_fragment_ = _parentStyle.mask_fragment_; }

    template <class IRI> void set(svgpp::tag::attribute::clip_path /*unused*/, IRI const& /*unused*/) { throw std::runtime_error("Non-local references aren't supported"); }

    template <class IRI> void set(svgpp::tag::attribute::clip_path /*unused*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        style().clip_path_fragment_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::clip_path /*unused*/, svgpp::tag::value::none /*val*/) { style().clip_path_fragment_.reset(); }

    void set(svgpp::tag::attribute::clip_path /*unused*/, svgpp::tag::value::inherit /*val*/) { style().clip_path_fragment_ = _parentStyle.clip_path_fragment_; }

    auto style() -> Style& { return _style; }
    [[nodiscard]] auto style() const -> Style const& { return _style; }

    template <class IRI> void set(svgpp::tag::attribute::marker_start /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_start_.reset();
    }

    template <class IRI> void set(svgpp::tag::attribute::marker_start /*unused*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        style().marker_start_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_start /*unused*/, svgpp::tag::value::none /*unused*/) { style().marker_start_.reset(); }

    template <class IRI> void set(svgpp::tag::attribute::marker_mid /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_mid_.reset();
    }

    template <class IRI> void set(svgpp::tag::attribute::marker_mid /*unused*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        style().marker_mid_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_mid /*unused*/, svgpp::tag::value::none /*unused*/) { style().marker_mid_.reset(); }

    template <class IRI> void set(svgpp::tag::attribute::marker_end /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_end_.reset();
    }

    template <class IRI> void set(svgpp::tag::attribute::marker_end /*unused*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        style().marker_end_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_end /*unused*/, svgpp::tag::value::none /*unused*/) { style().marker_end_.reset(); }

    template <class IRI> void set(svgpp::tag::attribute::marker /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_start_.reset();
        style().marker_mid_.reset();
        style().marker_end_.reset();
    }

    template <class IRI> void set(svgpp::tag::attribute::marker /*unused*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        std::string iri(boost::begin(fragment), boost::end(fragment));
        style().marker_start_ = iri;
        style().marker_mid_ = iri;
        style().marker_end_ = iri;
    }

    void set(svgpp::tag::attribute::marker /*unused*/, svgpp::tag::value::none /*unused*/) {
        style().marker_start_.reset();
        style().marker_mid_.reset();
        style().marker_end_.reset();
    }

    template <class IRI> void set(svgpp::tag::attribute::filter /*unused*/, IRI const& /*unused*/) { throw std::runtime_error("Non-local references aren't supported"); }

    template <class IRI> void set(svgpp::tag::attribute::filter /*unused*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        style().filter_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::filter /*unused*/, svgpp::tag::value::none /*val*/) { style().filter_.reset(); }

    void set(svgpp::tag::attribute::filter /*unused*/, svgpp::tag::value::inherit /*unused*/) { style().filter_ = _parentStyle.filter_; }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::inherit /*unused*/) { style().overflow_clip_ = _parentStyle.overflow_clip_; }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::visible /*unused*/) { style().overflow_clip_ = false; }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::auto_ /*unused*/) { style().overflow_clip_ = false; }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::hidden /*unused*/) { style().overflow_clip_ = true; }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::scroll /*unused*/) { style().overflow_clip_ = true; }

private:
    Style _style;
    NoninheritedStyle _parentStyle;
};

} // namespace laby::svgp


#endif /* SVGPARSER_STYLABLE_H_ */

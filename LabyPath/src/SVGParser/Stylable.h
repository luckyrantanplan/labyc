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
#include <vector>
#include <svgpp/svgpp.hpp>
#include "Common.h"

namespace laby {
namespace svgp {

enum line_cap_e {
    butt_cap, square_cap, round_cap
};

//------------------------------------------------------------line_join_e
enum line_join_e {
    miter_join = 0, miter_join_revert = 1, round_join = 2, bevel_join = 3, miter_join_round = 4
};

//-----------------------------------------------------------inner_join_e
enum inner_join_e {
    inner_bevel, inner_miter, inner_jag, inner_round
};
typedef int color_t;
typedef boost::variant<svgpp::tag::value::none, svgpp::tag::value::currentColor, color_t> SolidPaint;
struct IRIPaint {
    IRIPaint(std::string const & fragment, boost::optional<SolidPaint> const & fallback = boost::optional<SolidPaint>()) :
            fragment_(fragment), fallback_(fallback) {
    }

    std::string fragment_;
    boost::optional<SolidPaint> fallback_;
};
typedef boost::variant<SolidPaint, IRIPaint> Paint;
typedef boost::variant<svgpp::tag::value::none, color_t> EffectivePaint;

struct InheritedStyle {
    InheritedStyle() :
            color_(blackColor), //
            fill_paint_(blackColor), //
            stroke_paint_(svgpp::tag::value::none()), //
            stroke_opacity_(1.0), //
            fill_opacity_(1.0), //
            nonzero_fill_rule_(true), //
            stroke_width_(1.0), //
            line_cap_(butt_cap), //
            line_join_(miter_join), //
            miterlimit_(4.0), //
            stroke_dashoffset_(0)

    {

    }
    color_t blackColor = 0;
    color_t color_;
    Paint fill_paint_;
    Paint stroke_paint_;
    number_t stroke_opacity_;
    number_t fill_opacity_;
    bool nonzero_fill_rule_;

    number_t stroke_width_;

    line_cap_e line_cap_;
    line_join_e line_join_;

    number_t miterlimit_;

    std::vector<number_t> stroke_dasharray_;
    number_t stroke_dashoffset_;
    boost::optional<std::string> marker_start_, marker_mid_, marker_end_;
};

struct NoninheritedStyle {
    NoninheritedStyle() :
            opacity_(1.0), display_(true), overflow_clip_(true) // TODO:
    {
    }

    number_t opacity_;
    bool display_;
    boost::optional<std::string> mask_fragment_, clip_path_fragment_;
    boost::optional<std::string> filter_;
    bool overflow_clip_;
};

struct Style: InheritedStyle, NoninheritedStyle {
    struct inherit_tag {
    };

    Style() {
    }

    Style(Style const & src, inherit_tag) :
            InheritedStyle(src) {
    }

    EffectivePaint getEffectivePaint(Paint const & paint) const {
        SolidPaint const * solidPaint = NULL;
        if (IRIPaint const * iri = boost::get<IRIPaint>(&paint)) {
            // do not manage Gradient yet
            if (iri->fallback_)
                solidPaint = &*iri->fallback_;
            else
                throw std::runtime_error("Can't find paint server");
        } else
            solidPaint = boost::get<SolidPaint>(&paint);
        if (boost::get<svgpp::tag::value::none>(solidPaint))
            return svgpp::tag::value::none();
        if (boost::get<svgpp::tag::value::currentColor>(solidPaint))
            return color_;
        return boost::get<color_t>(*solidPaint);
    }

};

template<class AttributeTag>
class PaintContext {
public:
    PaintContext(Paint & paint) :
            paint_(paint) {
    }

    void set(AttributeTag, svgpp::tag::value::none) {
        paint_ = svgpp::tag::value::none();
    }

    void set(AttributeTag, svgpp::tag::value::currentColor) {
        paint_ = svgpp::tag::value::currentColor();
    }

    void set(AttributeTag, color_t color, svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
        paint_ = color;
    }

    template<class IRI>
    void set(AttributeTag tag, IRI const & iri) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template<class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const & fragment) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    template<class IRI>
    void set(AttributeTag tag, IRI const &, svgpp::tag::value::none val) {
        set(tag, val);
    }

    template<class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const & fragment, svgpp::tag::value::none val) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)), boost::optional<SolidPaint>(val));
    }

    template<class IRI>
    void set(AttributeTag tag, IRI const &, svgpp::tag::value::currentColor val) {
        set(tag, val);
    }

    template<class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const & fragment, svgpp::tag::value::currentColor val) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)), boost::optional<SolidPaint>(val));
    }

    template<class IRI>
    void set(AttributeTag tag, IRI const &, color_t val, svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
        set(tag, val);
    }

    template<class IRI>
    void set(AttributeTag tag, svgpp::tag::iri_fragment, IRI const & fragment, color_t val, svgpp::tag::skip_icc_color = svgpp::tag::skip_icc_color()) {
        paint_ = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)), boost::optional<SolidPaint>(val));
    }

protected:
    Paint & paint_;
};

class Stylable: public PaintContext<svgpp::tag::attribute::stroke>, public PaintContext<svgpp::tag::attribute::fill> {
public:
    typedef PaintContext<svgpp::tag::attribute::stroke> stroke_paint;
    typedef PaintContext<svgpp::tag::attribute::fill> fill_paint;

    Stylable() :
            stroke_paint(style_.stroke_paint_), fill_paint(style_.fill_paint_) {
    }

    Stylable(Stylable const & src) :
            stroke_paint(style_.stroke_paint_), fill_paint(style_.fill_paint_), style_(src.style_, Style::inherit_tag()) {
    }

    using stroke_paint::set;
    using fill_paint::set;

    void set(svgpp::tag::attribute::display, svgpp::tag::value::none) {
        style().display_ = false;
    }

    void set(svgpp::tag::attribute::display, svgpp::tag::value::inherit) {
        style().display_ = parentStyle_.display_;
    }

    template<class ValueTag>
    void set(svgpp::tag::attribute::display, ValueTag) {
        style().display_ = true;
    }

    void set(svgpp::tag::attribute::color, color_t val) {
        style().color_ = val;
    }

    void set(svgpp::tag::attribute::stroke_width, number_t val) {
        style().stroke_width_ = val;
    }

    void set(svgpp::tag::attribute::stroke_opacity, number_t val) {
        style().stroke_opacity_ = std::min(number_t(1), std::max(number_t(0), val));
    }

    void set(svgpp::tag::attribute::fill_opacity, number_t val) {
        style().fill_opacity_ = std::min(number_t(1), std::max(number_t(0), val));
    }

    void set(svgpp::tag::attribute::opacity, number_t val) {
        style().opacity_ = std::min(number_t(1), std::max(number_t(0), val));
    }

    void set(svgpp::tag::attribute::opacity, svgpp::tag::value::inherit) {
        style().opacity_ = parentStyle_.opacity_;
    }

    void set(svgpp::tag::attribute::fill_rule, svgpp::tag::value::nonzero) {
        style().nonzero_fill_rule_ = true;
    }

    void set(svgpp::tag::attribute::fill_rule, svgpp::tag::value::evenodd) {
        style().nonzero_fill_rule_ = false;
    }

    void set(svgpp::tag::attribute::stroke_linecap, svgpp::tag::value::butt) {
        style().line_cap_ = butt_cap;
    }

    void set(svgpp::tag::attribute::stroke_linecap, svgpp::tag::value::round) {
        style().line_cap_ = round_cap;
    }

    void set(svgpp::tag::attribute::stroke_linecap, svgpp::tag::value::square) {
        style().line_cap_ = square_cap;
    }

    void set(svgpp::tag::attribute::stroke_linejoin, svgpp::tag::value::miter) {
        style().line_join_ = miter_join;
    }

    void set(svgpp::tag::attribute::stroke_linejoin, svgpp::tag::value::round) {
        style().line_join_ = round_join;
    }

    void set(svgpp::tag::attribute::stroke_linejoin, svgpp::tag::value::bevel) {
        style().line_join_ = bevel_join;
    }

    void set(svgpp::tag::attribute::stroke_miterlimit, number_t val)

    {
        style().miterlimit_ = val;
    }

    void set(svgpp::tag::attribute::stroke_dasharray, svgpp::tag::value::none) {
        style().stroke_dasharray_.clear();
    }

    template<class Range>
    void set(svgpp::tag::attribute::stroke_dasharray, Range const & range) {
        style().stroke_dasharray_.assign(boost::begin(range), boost::end(range));
    }

    void set(svgpp::tag::attribute::stroke_dashoffset, number_t val) {
        style().stroke_dashoffset_ = val;
    }

    template<class IRI>
    void set(svgpp::tag::attribute::mask, IRI const &) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template<class IRI>
    void set(svgpp::tag::attribute::mask, svgpp::tag::iri_fragment, IRI const & fragment) {
        style().mask_fragment_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::mask, svgpp::tag::value::none val) {
        style().mask_fragment_.reset();
    }

    void set(svgpp::tag::attribute::mask, svgpp::tag::value::inherit val) {
        style().mask_fragment_ = parentStyle_.mask_fragment_;
    }

    template<class IRI>
    void set(svgpp::tag::attribute::clip_path, IRI const &) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template<class IRI>
    void set(svgpp::tag::attribute::clip_path, svgpp::tag::iri_fragment, IRI const & fragment) {
        style().clip_path_fragment_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::clip_path, svgpp::tag::value::none val) {
        style().clip_path_fragment_.reset();
    }

    void set(svgpp::tag::attribute::clip_path, svgpp::tag::value::inherit val) {
        style().clip_path_fragment_ = parentStyle_.clip_path_fragment_;
    }

    Style & style() {
        return style_;
    }
    Style const & style() const {
        return style_;
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker_start, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_start_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker_start, svgpp::tag::iri_fragment, IRI const & fragment) {
        style().marker_start_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_start, svgpp::tag::value::none) {
        style().marker_start_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker_mid, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_mid_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker_mid, svgpp::tag::iri_fragment, IRI const & fragment) {
        style().marker_mid_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_mid, svgpp::tag::value::none) {
        style().marker_mid_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker_end, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_end_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker_end, svgpp::tag::iri_fragment, IRI const & fragment) {
        style().marker_end_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::marker_end, svgpp::tag::value::none) {
        style().marker_end_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker, IRI const &) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().marker_start_.reset();
        style().marker_mid_.reset();
        style().marker_end_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::marker, svgpp::tag::iri_fragment, IRI const & fragment) {
        std::string iri(boost::begin(fragment), boost::end(fragment));
        style().marker_start_ = iri;
        style().marker_mid_ = iri;
        style().marker_end_ = iri;
    }

    void set(svgpp::tag::attribute::marker, svgpp::tag::value::none) {
        style().marker_start_.reset();
        style().marker_mid_.reset();
        style().marker_end_.reset();
    }

    template<class IRI>
    void set(svgpp::tag::attribute::filter, IRI const &) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template<class IRI>
    void set(svgpp::tag::attribute::filter, svgpp::tag::iri_fragment, IRI const & fragment) {
        style().filter_ = std::string(boost::begin(fragment), boost::end(fragment));
    }

    void set(svgpp::tag::attribute::filter, svgpp::tag::value::none val) {
        style().filter_.reset();
    }

    void set(svgpp::tag::attribute::filter, svgpp::tag::value::inherit) {
        style().filter_ = parentStyle_.filter_;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::inherit) {
        style().overflow_clip_ = parentStyle_.overflow_clip_;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::visible) {
        style().overflow_clip_ = false;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::auto_) {
        style().overflow_clip_ = false;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::hidden) {
        style().overflow_clip_ = true;
    }

    void set(svgpp::tag::attribute::overflow, svgpp::tag::value::scroll) {
        style().overflow_clip_ = true;
    }

private:
    Style style_;
    NoninheritedStyle parentStyle_;
};

}/* namespace svgp */
} /* namespace laby */

#endif /* SVGPARSER_STYLABLE_H_ */

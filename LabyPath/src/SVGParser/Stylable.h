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

#include "Common.h"
#include <algorithm>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <svgpp/svgpp.hpp>
#include <utility>
#include <vector>

namespace laby::svgp {

namespace {
constexpr number_t kDefaultOpacity = 1.0;
constexpr number_t kDefaultStrokeWidth = 1.0;
constexpr number_t kDefaultMiterLimit = 4.0;
} // namespace

enum class LineCap : std::uint8_t { Butt, Square, Round };

enum class LineJoin : std::uint8_t {
    Miter = 0,
    MiterRevert = 1,
    Round = 2,
    Bevel = 3,
    MiterRound = 4
};

enum class InnerJoin : std::uint8_t { Bevel, Miter, Jag, Round };

using color_t = int;
using SolidPaint =
    boost::variant<svgpp::tag::value::none, svgpp::tag::value::currentColor, color_t>;
class IRIPaint {
  public:
    explicit IRIPaint(std::string fragment,
                      boost::optional<SolidPaint> const& fallback = boost::optional<SolidPaint>())
        : _fragment(std::move(fragment)), _fallback(fallback) {}

    [[nodiscard]] auto fragment() const -> const std::string& {
        return _fragment;
    }

    [[nodiscard]] auto fallback() const -> const boost::optional<SolidPaint>& {
        return _fallback;
    }

  private:
    std::string _fragment;
    boost::optional<SolidPaint> _fallback;
};

using Paint = boost::variant<SolidPaint, IRIPaint>;
using EffectivePaint = boost::variant<svgpp::tag::value::none, color_t>;

class InheritedStyle {
  public:
    InheritedStyle() = default;
    InheritedStyle(const InheritedStyle&) = default;
    auto operator=(const InheritedStyle&) -> InheritedStyle& = default;
    InheritedStyle(InheritedStyle&&) noexcept = default;
    auto operator=(InheritedStyle&&) noexcept -> InheritedStyle& = default;
    ~InheritedStyle() = default;

    [[nodiscard]] auto color() const -> color_t {
        return _color;
    }

    void setColor(color_t colorValue) {
        _color = colorValue;
    }

    [[nodiscard]] auto fillPaint() const -> const Paint& {
        return _fillPaint;
    }

    [[nodiscard]] auto strokePaint() const -> const Paint& {
        return _strokePaint;
    }

    auto fillPaintRef() -> Paint& {
        return _fillPaint;
    }

    auto strokePaintRef() -> Paint& {
        return _strokePaint;
    }

    [[nodiscard]] auto strokeOpacity() const -> number_t {
        return _strokeOpacity;
    }

    void setStrokeOpacity(number_t opacityValue) {
        _strokeOpacity = opacityValue;
    }

    [[nodiscard]] auto fillOpacity() const -> number_t {
        return _fillOpacity;
    }

    void setFillOpacity(number_t opacityValue) {
        _fillOpacity = opacityValue;
    }

    [[nodiscard]] auto usesNonzeroFillRule() const -> bool {
        return _nonzeroFillRule;
    }

    void setUsesNonzeroFillRule(bool usesNonzeroFillRuleValue) {
        _nonzeroFillRule = usesNonzeroFillRuleValue;
    }

    [[nodiscard]] auto strokeWidth() const -> number_t {
        return _strokeWidth;
    }

    void setStrokeWidth(number_t strokeWidthValue) {
        _strokeWidth = strokeWidthValue;
    }

    [[nodiscard]] auto lineCap() const -> LineCap {
        return _lineCap;
    }

    void setLineCap(LineCap lineCapValue) {
        _lineCap = lineCapValue;
    }

    [[nodiscard]] auto lineJoin() const -> LineJoin {
        return _lineJoin;
    }

    void setLineJoin(LineJoin lineJoinValue) {
        _lineJoin = lineJoinValue;
    }

    [[nodiscard]] auto miterLimit() const -> number_t {
        return _miterLimit;
    }

    void setMiterLimit(number_t miterLimitValue) {
        _miterLimit = miterLimitValue;
    }

    [[nodiscard]] auto strokeDashArray() const -> const std::vector<number_t>& {
        return _strokeDashArray;
    }

    auto strokeDashArray() -> std::vector<number_t>& {
        return _strokeDashArray;
    }

    void clearStrokeDashArray() {
        _strokeDashArray.clear();
    }

    [[nodiscard]] auto strokeDashOffset() const -> number_t {
        return _strokeDashOffset;
    }

    void setStrokeDashOffset(number_t strokeDashOffsetValue) {
        _strokeDashOffset = strokeDashOffsetValue;
    }

    [[nodiscard]] auto markerStart() const -> const boost::optional<std::string>& {
        return _markerStart;
    }

    [[nodiscard]] auto markerMid() const -> const boost::optional<std::string>& {
        return _markerMid;
    }

    [[nodiscard]] auto markerEnd() const -> const boost::optional<std::string>& {
        return _markerEnd;
    }

    void setMarkerStart(std::string markerValue) {
        _markerStart = std::move(markerValue);
    }

    void setMarkerMid(std::string markerValue) {
        _markerMid = std::move(markerValue);
    }

    void setMarkerEnd(std::string markerValue) {
        _markerEnd = std::move(markerValue);
    }

    void clearMarkerStart() {
        _markerStart.reset();
    }

    void clearMarkerMid() {
        _markerMid.reset();
    }

    void clearMarkerEnd() {
        _markerEnd.reset();
    }

  private:
    static constexpr color_t kBlackColor = 0;

    color_t _color = kBlackColor;
    Paint _fillPaint = kBlackColor;
    Paint _strokePaint = svgpp::tag::value::none();
    number_t _strokeOpacity = kDefaultOpacity;
    number_t _fillOpacity = kDefaultOpacity;
    bool _nonzeroFillRule = true;
    number_t _strokeWidth = kDefaultStrokeWidth;
    LineCap _lineCap = LineCap::Butt;
    LineJoin _lineJoin = LineJoin::Miter;
    number_t _miterLimit = kDefaultMiterLimit;
    std::vector<number_t> _strokeDashArray;
    number_t _strokeDashOffset = 0;
    boost::optional<std::string> _markerStart;
    boost::optional<std::string> _markerMid;
    boost::optional<std::string> _markerEnd;
};

class NoninheritedStyle {
  public:
    NoninheritedStyle() = default;
    NoninheritedStyle(const NoninheritedStyle&) = default;
    auto operator=(const NoninheritedStyle&) -> NoninheritedStyle& = default;
    NoninheritedStyle(NoninheritedStyle&&) noexcept = default;
    auto operator=(NoninheritedStyle&&) noexcept -> NoninheritedStyle& = default;
    ~NoninheritedStyle() = default;

    [[nodiscard]] auto opacity() const -> number_t {
        return _opacity;
    }

    void setOpacity(number_t opacityValue) {
        _opacity = opacityValue;
    }

    [[nodiscard]] auto display() const -> bool {
        return _display;
    }

    void setDisplay(bool displayValue) {
        _display = displayValue;
    }

    [[nodiscard]] auto maskFragment() const -> const boost::optional<std::string>& {
        return _maskFragment;
    }

    void setMaskFragment(std::string maskFragmentValue) {
        _maskFragment = std::move(maskFragmentValue);
    }

    void clearMaskFragment() {
        _maskFragment.reset();
    }

    [[nodiscard]] auto clipPathFragment() const -> const boost::optional<std::string>& {
        return _clipPathFragment;
    }

    void setClipPathFragment(std::string clipPathFragmentValue) {
        _clipPathFragment = std::move(clipPathFragmentValue);
    }

    void clearClipPathFragment() {
        _clipPathFragment.reset();
    }

    [[nodiscard]] auto filter() const -> const boost::optional<std::string>& {
        return _filter;
    }

    void setFilter(std::string filterValue) {
        _filter = std::move(filterValue);
    }

    void clearFilter() {
        _filter.reset();
    }

    [[nodiscard]] auto overflowClip() const -> bool {
        return _overflowClip;
    }

    void setOverflowClip(bool overflowClipValue) {
        _overflowClip = overflowClipValue;
    }

  private:
    number_t _opacity = kDefaultOpacity;
    bool _display = true;
    boost::optional<std::string> _maskFragment;
    boost::optional<std::string> _clipPathFragment;
    boost::optional<std::string> _filter;
    bool _overflowClip = true;
};

class Style : public InheritedStyle, public NoninheritedStyle {
  public:
    struct InheritTag {};

    Style() = default;
    Style(const Style&) = default;
    auto operator=(const Style&) -> Style& = default;
    Style(Style&&) noexcept = default;
    auto operator=(Style&&) noexcept -> Style& = default;
    ~Style() = default;

    Style(Style const& src, InheritTag /*unused*/) : InheritedStyle(src) {}

    [[nodiscard]] auto getEffectivePaint(Paint const& paint) const -> EffectivePaint {
        SolidPaint const* solidPaint = nullptr;
        if (IRIPaint const* iri = boost::get<IRIPaint>(&paint)) {
            // do not manage Gradient yet
            if (iri->fallback()) {
                solidPaint = &*iri->fallback();
            } else {
                throw std::runtime_error("Can't find paint server");
            }
        } else {
            solidPaint = boost::get<SolidPaint>(&paint);
        }
        if (boost::get<svgpp::tag::value::none>(solidPaint) != nullptr) {
            return svgpp::tag::value::none();
        }
        if (boost::get<svgpp::tag::value::currentColor>(solidPaint) != nullptr) {
            return color();
        }
        return boost::get<color_t>(*solidPaint);
    }
};

template <class AttributeTag> class PaintContext {
  public:
    PaintContext() = default;
    explicit PaintContext(Paint& paint) : _paint(&paint) {}
    PaintContext(const PaintContext&) = default;
    auto operator=(const PaintContext&) -> PaintContext& = default;
    PaintContext(PaintContext&&) noexcept = default;
    auto operator=(PaintContext&&) noexcept -> PaintContext& = default;
    ~PaintContext() = default;

    void set(AttributeTag /*unused*/, svgpp::tag::value::none /*unused*/) {
        paint() = svgpp::tag::value::none();
    }

    void set(AttributeTag /*unused*/, svgpp::tag::value::currentColor /*unused*/) {
        paint() = svgpp::tag::value::currentColor();
    }

    void set(AttributeTag /*unused*/, color_t color,
             svgpp::tag::skip_icc_color /*unused*/ = svgpp::tag::skip_icc_color()) {
        paint() = color;
    }

    template <class IRI> void set(AttributeTag /*tag*/, IRI const& /*iri*/) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment) {
        paint() = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    template <class IRI>
    void set(AttributeTag tag, IRI const& /*unused*/, svgpp::tag::value::none val) {
        set(tag, val);
    }

    template <class IRI>
    void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment,
             svgpp::tag::value::none val) {
        paint() = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                           boost::optional<SolidPaint>(val));
    }

    template <class IRI>
    void set(AttributeTag tag, IRI const& /*unused*/, svgpp::tag::value::currentColor val) {
        set(tag, val);
    }

    template <class IRI>
    void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment,
             svgpp::tag::value::currentColor val) {
        paint() = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                           boost::optional<SolidPaint>(val));
    }

    template <class IRI>
    void set(AttributeTag tag, IRI const& /*unused*/, color_t val,
             svgpp::tag::skip_icc_color /*unused*/ = svgpp::tag::skip_icc_color()) {
        set(tag, val);
    }

    template <class IRI>
    void set(AttributeTag /*tag*/, svgpp::tag::iri_fragment /*unused*/, IRI const& fragment,
             color_t val, svgpp::tag::skip_icc_color /*unused*/ = svgpp::tag::skip_icc_color()) {
        paint() = IRIPaint(std::string(boost::begin(fragment), boost::end(fragment)),
                           boost::optional<SolidPaint>(val));
    }

  protected:
    void bind(Paint& paint) {
        _paint = &paint;
    }

    [[nodiscard]] auto paint() -> Paint& {
        return *_paint;
    }

  private:
    Paint* _paint;
};

class Stylable : public PaintContext<svgpp::tag::attribute::stroke>,
                 public PaintContext<svgpp::tag::attribute::fill> {
  public:
    using stroke_paint = PaintContext<svgpp::tag::attribute::stroke>;
    using fill_paint = PaintContext<svgpp::tag::attribute::fill>;

    Stylable() : PaintContext(), PaintContext() {
        stroke_paint::bind(_style.strokePaintRef());
        fill_paint::bind(_style.fillPaintRef());
    }

    Stylable(Stylable const& src)
        : stroke_paint(src), fill_paint(src), _style(src._style, Style::InheritTag()),
          _parentStyle(src._style) {
        stroke_paint::bind(_style.strokePaintRef());
        fill_paint::bind(_style.fillPaintRef());
    }

    auto operator=(const Stylable&) -> Stylable& = delete;
    Stylable(Stylable&&) = delete;
    auto operator=(Stylable&&) -> Stylable& = delete;
    ~Stylable() = default;

    using fill_paint::set;
    using stroke_paint::set;

    void set(svgpp::tag::attribute::display /*unused*/, svgpp::tag::value::none /*unused*/) {
        style().setDisplay(false);
    }

    void set(svgpp::tag::attribute::display /*unused*/, svgpp::tag::value::inherit /*unused*/) {
        style().setDisplay(_parentStyle.display());
    }

    template <class ValueTag>
    void set(svgpp::tag::attribute::display /*unused*/, ValueTag /*unused*/) {
        style().setDisplay(true);
    }

    void set(svgpp::tag::attribute::color /*unused*/, color_t val) {
        style().setColor(val);
    }

    void set(svgpp::tag::attribute::stroke_width /*unused*/, number_t val) {
        style().setStrokeWidth(val);
    }

    void set(svgpp::tag::attribute::stroke_opacity /*unused*/, number_t val) {
        style().setStrokeOpacity(std::min(number_t(1), std::max(number_t(0), val)));
    }

    void set(svgpp::tag::attribute::fill_opacity /*unused*/, number_t val) {
        style().setFillOpacity(std::min(number_t(1), std::max(number_t(0), val)));
    }

    void set(svgpp::tag::attribute::opacity /*unused*/, number_t val) {
        style().setOpacity(std::min(number_t(1), std::max(number_t(0), val)));
    }

    void set(svgpp::tag::attribute::opacity /*unused*/, svgpp::tag::value::inherit /*unused*/) {
        style().setOpacity(_parentStyle.opacity());
    }

    void set(svgpp::tag::attribute::fill_rule /*unused*/, svgpp::tag::value::nonzero /*unused*/) {
        style().setUsesNonzeroFillRule(true);
    }

    void set(svgpp::tag::attribute::fill_rule /*unused*/, svgpp::tag::value::evenodd /*unused*/) {
        style().setUsesNonzeroFillRule(false);
    }

    void set(svgpp::tag::attribute::stroke_linecap /*unused*/, svgpp::tag::value::butt /*unused*/) {
        style().setLineCap(LineCap::Butt);
    }

    void set(svgpp::tag::attribute::stroke_linecap /*unused*/,
             svgpp::tag::value::round /*unused*/) {
        style().setLineCap(LineCap::Round);
    }

    void set(svgpp::tag::attribute::stroke_linecap /*unused*/,
             svgpp::tag::value::square /*unused*/) {
        style().setLineCap(LineCap::Square);
    }

    void set(svgpp::tag::attribute::stroke_linejoin /*unused*/,
             svgpp::tag::value::miter /*unused*/) {
        style().setLineJoin(LineJoin::Miter);
    }

    void set(svgpp::tag::attribute::stroke_linejoin /*unused*/,
             svgpp::tag::value::round /*unused*/) {
        style().setLineJoin(LineJoin::Round);
    }

    void set(svgpp::tag::attribute::stroke_linejoin /*unused*/,
             svgpp::tag::value::bevel /*unused*/) {
        style().setLineJoin(LineJoin::Bevel);
    }

    void set(svgpp::tag::attribute::stroke_miterlimit /*unused*/, number_t val)

    {
        style().setMiterLimit(val);
    }

    void set(svgpp::tag::attribute::stroke_dasharray /*unused*/,
             svgpp::tag::value::none /*unused*/) {
        style().clearStrokeDashArray();
    }

    template <class Range>
    void set(svgpp::tag::attribute::stroke_dasharray /*unused*/, Range const& range) {
        style().strokeDashArray().assign(boost::begin(range), boost::end(range));
    }

    void set(svgpp::tag::attribute::stroke_dashoffset /*unused*/, number_t val) {
        style().setStrokeDashOffset(val);
    }

    template <class IRI> void set(svgpp::tag::attribute::mask /*unused*/, IRI const& /*unused*/) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(svgpp::tag::attribute::mask /*unused*/, svgpp::tag::iri_fragment /*unused*/,
             IRI const& fragment) {
        style().setMaskFragment(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    void set(svgpp::tag::attribute::mask /*unused*/, svgpp::tag::value::none /*val*/) {
        style().clearMaskFragment();
    }

    void set(svgpp::tag::attribute::mask /*unused*/, svgpp::tag::value::inherit /*val*/) {
        if (_parentStyle.maskFragment()) {
            style().setMaskFragment(*_parentStyle.maskFragment());
        } else {
            style().clearMaskFragment();
        }
    }

    template <class IRI>
    void set(svgpp::tag::attribute::clip_path /*unused*/, IRI const& /*unused*/) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(svgpp::tag::attribute::clip_path /*unused*/, svgpp::tag::iri_fragment /*unused*/,
             IRI const& fragment) {
        style().setClipPathFragment(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    void set(svgpp::tag::attribute::clip_path /*unused*/, svgpp::tag::value::none /*val*/) {
        style().clearClipPathFragment();
    }

    void set(svgpp::tag::attribute::clip_path /*unused*/, svgpp::tag::value::inherit /*val*/) {
        if (_parentStyle.clipPathFragment()) {
            style().setClipPathFragment(*_parentStyle.clipPathFragment());
        } else {
            style().clearClipPathFragment();
        }
    }

    auto style() -> Style& {
        return _style;
    }
    [[nodiscard]] auto style() const -> Style const& {
        return _style;
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_start /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().clearMarkerStart();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_start /*unused*/, svgpp::tag::iri_fragment /*unused*/,
             IRI const& fragment) {
        style().setMarkerStart(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    void set(svgpp::tag::attribute::marker_start /*unused*/, svgpp::tag::value::none /*unused*/) {
        style().clearMarkerStart();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_mid /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().clearMarkerMid();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_mid /*unused*/, svgpp::tag::iri_fragment /*unused*/,
             IRI const& fragment) {
        style().setMarkerMid(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    void set(svgpp::tag::attribute::marker_mid /*unused*/, svgpp::tag::value::none /*unused*/) {
        style().clearMarkerMid();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_end /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().clearMarkerEnd();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker_end /*unused*/, svgpp::tag::iri_fragment /*unused*/,
             IRI const& fragment) {
        style().setMarkerEnd(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    void set(svgpp::tag::attribute::marker_end /*unused*/, svgpp::tag::value::none /*unused*/) {
        style().clearMarkerEnd();
    }

    template <class IRI> void set(svgpp::tag::attribute::marker /*unused*/, IRI const& /*unused*/) {
        std::cout << "Non-local references aren't supported\n"; // Not error
        style().clearMarkerStart();
        style().clearMarkerMid();
        style().clearMarkerEnd();
    }

    template <class IRI>
    void set(svgpp::tag::attribute::marker /*unused*/, svgpp::tag::iri_fragment /*unused*/,
             IRI const& fragment) {
        std::string iri(boost::begin(fragment), boost::end(fragment));
        style().setMarkerStart(iri);
        style().setMarkerMid(iri);
        style().setMarkerEnd(std::move(iri));
    }

    void set(svgpp::tag::attribute::marker /*unused*/, svgpp::tag::value::none /*unused*/) {
        style().clearMarkerStart();
        style().clearMarkerMid();
        style().clearMarkerEnd();
    }

    template <class IRI> void set(svgpp::tag::attribute::filter /*unused*/, IRI const& /*unused*/) {
        throw std::runtime_error("Non-local references aren't supported");
    }

    template <class IRI>
    void set(svgpp::tag::attribute::filter /*unused*/, svgpp::tag::iri_fragment /*unused*/,
             IRI const& fragment) {
        style().setFilter(std::string(boost::begin(fragment), boost::end(fragment)));
    }

    void set(svgpp::tag::attribute::filter /*unused*/, svgpp::tag::value::none /*val*/) {
        style().clearFilter();
    }

    void set(svgpp::tag::attribute::filter /*unused*/, svgpp::tag::value::inherit /*unused*/) {
        if (_parentStyle.filter()) {
            style().setFilter(*_parentStyle.filter());
        } else {
            style().clearFilter();
        }
    }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::inherit /*unused*/) {
        style().setOverflowClip(_parentStyle.overflowClip());
    }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::visible /*unused*/) {
        style().setOverflowClip(false);
    }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::auto_ /*unused*/) {
        style().setOverflowClip(false);
    }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::hidden /*unused*/) {
        style().setOverflowClip(true);
    }

    void set(svgpp::tag::attribute::overflow /*unused*/, svgpp::tag::value::scroll /*unused*/) {
        style().setOverflowClip(true);
    }

  private:
    Style _style;
    NoninheritedStyle _parentStyle;
};

} // namespace laby::svgp

#endif /* SVGPARSER_STYLABLE_H_ */

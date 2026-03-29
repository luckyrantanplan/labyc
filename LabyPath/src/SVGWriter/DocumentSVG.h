/*
 * DocumentSVG.h
 *
 *  Created on: Sep 28, 2018
 *      Author: florian
 */

#ifndef SVGWRITER_DOCUMENTSVG_H_
#define SVGWRITER_DOCUMENTSVG_H_

#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "../Polyline.h"

namespace svg {
namespace {
constexpr double kDefaultDocumentWidth = 400.0;
constexpr double kDefaultDocumentHeight = 300.0;
constexpr double kTransparentStrokeWidth = -1.0;
constexpr double kDefaultScale = 1.0;
constexpr double kDefaultFontSize = 12.0;
constexpr int32_t kRgbMax = 255;
constexpr int32_t kGreenChannelHalf = 128;
constexpr int32_t kBrownRed = 165;
constexpr int32_t kBrownGreen = 42;
constexpr int32_t kOrangeGreen = 165;
constexpr int32_t kSilverChannel = 192;
} // namespace

// Utility XML/String Functions.
template <typename T>
inline auto attribute(std::string const& attributeName, T const& value,
                      std::string_view unit = {}) -> std::string {
    std::stringstream stream;
    stream << attributeName << "=\"" << value << unit << "\" ";
    return stream.str();
}
inline auto elemStart(std::string const& elementName) -> std::string {
    return "\t<" + elementName + " ";
}
inline auto elemEnd(std::string const& elementName) -> std::string {
    return "</" + elementName + ">\n";
}
inline auto emptyElemEnd() -> std::string {
    return "/>\n";
}

// Quick optional return type.  This allows functions to return an invalid
//  value if no good return is possible.  The user checks for validity
//  before using the returned value.
template <typename T> class Optional {
  public:
    explicit Optional(T const& val) : _valid(true), _type(val) {}
    Optional() : _valid(false), _type(T()) {}
    auto operator->() -> T* {
        // If we try to access an invalid value, an exception is thrown.
        if (!_valid) {
            throw std::exception();
        }

        return &_type;
    }
    // Test for validity.
    auto operator!() const -> bool {
        return !_valid;
    }

  private:
    bool _valid;
    T _type;
};

struct Dimensions {
    struct Size {
        double width;
        double height;
    };

    explicit Dimensions(Size size) : _width(size.width), _height(size.height) {}
    explicit Dimensions(double combined = 0) : _width(combined), _height(combined) {}

    [[nodiscard]] auto width() const -> double {
        return _width;
    }

    [[nodiscard]] auto height() const -> double {
        return _height;
    }

  private:
    double _width;
    double _height;
};

using Point = laby::Kernel::Point_2;

// struct Point {
//     Point(double x = 0, double y = 0) :
//             x(x), y(y) {
//     }
//     double x;
//     double y;
// };

// Defines the dimensions, scale, origin, and origin offset of the document.
struct Layout {
    enum class Origin : std::uint8_t { TopLeft, BottomLeft, TopRight, BottomRight };

    explicit Layout(Dimensions const& dims = Dimensions(Dimensions::Size{kDefaultDocumentWidth,
                                                                         kDefaultDocumentHeight}),
                    Origin orig = Origin::BottomLeft, double scaleValue = kDefaultScale,
                    Point originOffset = Point(0, 0))
        : _dimensions(dims), _scale(scaleValue), _origin(orig),
          _originOffset(std::move(originOffset)) {}

    [[nodiscard]] auto dimensions() const -> const Dimensions& {
        return _dimensions;
    }

    [[nodiscard]] auto scale() const -> double {
        return _scale;
    }

    [[nodiscard]] auto origin() const -> Origin {
        return _origin;
    }

    [[nodiscard]] auto originOffset() const -> const Point& {
        return _originOffset;
    }

  private:
    Dimensions _dimensions;
    double _scale;
    Origin _origin;
    Point _originOffset{};
};

// Convert coordinates in user space to SVG native space.
inline auto translateX(const Point& point, Layout const& layout) -> double {
    const double xCoordinate = CGAL::to_double(point.x());
    const double offsetX = CGAL::to_double(layout.originOffset().x());
    if (layout.origin() == Layout::Origin::BottomRight ||
        layout.origin() == Layout::Origin::TopRight) {
        return layout.dimensions().width() - ((xCoordinate + offsetX) * layout.scale());
    }
    return (offsetX + xCoordinate) * layout.scale();
}

inline auto translateY(const Point& point, Layout const& layout) -> double {
    const double yCoordinate = CGAL::to_double(point.y());
    const double offsetY = CGAL::to_double(layout.originOffset().y());
    if (layout.origin() == Layout::Origin::BottomLeft ||
        layout.origin() == Layout::Origin::BottomRight) {
        return layout.dimensions().height() - ((yCoordinate + offsetY) * layout.scale());
    }
    return (offsetY + yCoordinate) * layout.scale();
}
inline auto translateScale(double dimension, Layout const& layout) -> double {
    return dimension * layout.scale();
}

class Serializeable {
  public:
    Serializeable() = default;
    Serializeable(const Serializeable&) = default;
    auto operator=(const Serializeable&) -> Serializeable& = default;
    Serializeable(Serializeable&&) noexcept = default;
    auto operator=(Serializeable&&) noexcept -> Serializeable& = default;
    virtual ~Serializeable() = default;
    [[nodiscard]] virtual auto toString(Layout const& layout) const -> std::string = 0;
};

class Color : public Serializeable {
  public:
    struct Rgb {
        int32_t red;
        int32_t green;
        int32_t blue;
    };

    enum class Defaults : std::int8_t {
        Transparent = -1,
        Aqua,
        Black,
        Blue,
        Brown,
        Cyan,
        Fuchsia,
        Green,
        Lime,
        Magenta,
        Orange,
        Purple,
        Red,
        Silver,
        White,
        Yellow
    };

    explicit Color(const Rgb& rgb)
        : _transparent(false), _red(rgb.red), _green(rgb.green), _blue(rgb.blue) {}
    explicit Color(Defaults color) : _transparent(false), _red(0), _green(0), _blue(0) {
        switch (color) {
        case Defaults::Aqua:
            assign({0, kRgbMax, kRgbMax});
            break;
        case Defaults::Black:
            assign({0, 0, 0});
            break;
        case Defaults::Blue:
            assign({0, 0, kRgbMax});
            break;
        case Defaults::Brown:
            assign({kBrownRed, kBrownGreen, kBrownGreen});
            break;
        case Defaults::Cyan:
            assign({0, kRgbMax, kRgbMax});
            break;
        case Defaults::Fuchsia:
            assign({kRgbMax, 0, kRgbMax});
            break;
        case Defaults::Green:
            assign({0, kGreenChannelHalf, 0});
            break;
        case Defaults::Lime:
            assign({0, kRgbMax, 0});
            break;
        case Defaults::Magenta:
            assign({kRgbMax, 0, kRgbMax});
            break;
        case Defaults::Orange:
            assign({kRgbMax, kOrangeGreen, 0});
            break;
        case Defaults::Purple:
            assign({kGreenChannelHalf, 0, kGreenChannelHalf});
            break;
        case Defaults::Red:
            assign({kRgbMax, 0, 0});
            break;
        case Defaults::Silver:
            assign({kSilverChannel, kSilverChannel, kSilverChannel});
            break;
        case Defaults::White:
            assign({kRgbMax, kRgbMax, kRgbMax});
            break;
        case Defaults::Yellow:
            assign({kRgbMax, kRgbMax, 0});
            break;
        default:
            _transparent = true;
            break;
        }
    }
    ~Color() override = default;
    [[nodiscard]] auto toString(Layout const& /*layout*/) const -> std::string override {
        std::stringstream ss;
        if (_transparent) {
            ss << "none";
        } else {
            ss << "rgb(" << _red << "," << _green << "," << _blue << ")";
        }
        return ss.str();
    }

  private:
    bool _transparent;
    int32_t _red;
    int32_t _green;
    int32_t _blue;

    void assign(const Rgb& rgb) {
        _red = rgb.red;
        _green = rgb.green;
        _blue = rgb.blue;
    }
};

class Fill : public Serializeable {
  public:
    explicit Fill(Color::Defaults colorValue) : _color(colorValue) {}
    explicit Fill(const Color& color = Color(Color::Defaults::Transparent)) : _color(color) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << attribute("fill", _color.toString(layout));
        return ss.str();
    }

  private:
    Color _color;
};

class Stroke : public Serializeable {
  public:
    explicit Stroke(double widthValue = kTransparentStrokeWidth,
                    const Color& color = Color(Color::Defaults::Transparent),
                    bool hasNonScalingStroke = false)
        : _width(widthValue), _color(color), _nonScaling(hasNonScalingStroke) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        // If stroke width is invalid.
        if (_width < 0) {
            return {};
        }

        std::stringstream ss;
        ss << attribute("stroke-width", translateScale(_width, layout))
           << attribute("stroke", _color.toString(layout));
        ss << attribute("stroke-linecap", "round");
        ss << attribute(" stroke-linejoin", "round");
        if (_nonScaling) {
            ss << attribute("vector-effect", "non-scaling-stroke");
        }
        return ss.str();
    }

  private:
    double _width;
    Color _color;
    bool _nonScaling;
};

class Font : public Serializeable {
  public:
    explicit Font(double fontSize = kDefaultFontSize, const std::string& family = "Verdana")
        : _size(fontSize), _family(std::move(family)) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << attribute("font-size", translateScale(_size, layout))
           << attribute("font-family", _family);
        return ss.str();
    }

  private:
    double _size;
    std::string _family;
};

class Shape : public Serializeable {
  public:
    explicit Shape(Fill fillValue = Fill(), Stroke strokeValue = Stroke())
        : _fill(std::move(fillValue)), _stroke(std::move(strokeValue)) {}
    Shape(const Shape&) = default;
    auto operator=(const Shape&) -> Shape& = default;
    Shape(Shape&&) noexcept = default;
    auto operator=(Shape&&) noexcept -> Shape& = default;
    ~Shape() override = default;
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override = 0;
    virtual void offset(const laby::Kernel::Vector_2& offset) = 0;

  protected:
    [[nodiscard]] auto fillStyle() const -> const Fill& {
        return _fill;
    }
    [[nodiscard]] auto strokeStyle() const -> const Stroke& {
        return _stroke;
    }

  private:
    Fill _fill;
    Stroke _stroke;
};
template <typename T>
inline auto vectorToString(const std::vector<T>& collection, Layout const& layout) -> std::string {
    std::string combinationStr;
    for (std::size_t collectionIndex = 0; collectionIndex < collection.size(); ++collectionIndex) {
        combinationStr += collection[collectionIndex].toString(layout);
    }

    return combinationStr;
}

class Circle : public Shape {
  public:
    Circle(Point centerPoint, double diameter, Fill const& fillValue,
           Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue), _center(std::move(centerPoint)), _radius(diameter / 2) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("circle") << attribute("cx", translateX(_center, layout))
           << attribute("cy", translateY(_center, layout))
           << attribute("r", translateScale(_radius, layout)) << fillStyle().toString(layout)
           << strokeStyle().toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2& off) override {
        _center += off;
    }

  private:
    Point _center{};
    double _radius;
};

class Elipse : public Shape {
  public:
    Elipse(Point centerPoint, double widthValue, double heightValue, Fill const& fillValue = Fill(),
           Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue), _center(std::move(centerPoint)),
          _radius_width(widthValue / 2), _radius_height(heightValue / 2) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("ellipse") << attribute("cx", translateX(_center, layout))
           << attribute("cy", translateY(_center, layout))
           << attribute("rx", translateScale(_radius_width, layout))
           << attribute("ry", translateScale(_radius_height, layout))
           << fillStyle().toString(layout) << strokeStyle().toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2& off) override {

        _center += off;
    }

  private:
    Point _center{};
    double _radius_width;
    double _radius_height;
};

class Rectangle : public Shape {
  public:
    Rectangle(Point edgePoint, double widthValue, double heightValue,
              Fill const& fillValue = Fill(), Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue), _edge(std::move(edgePoint)), _width(widthValue),
          _height(heightValue) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("rect") << attribute("x", translateX(_edge, layout))
           << attribute("y", translateY(_edge, layout))
           << attribute("width", translateScale(_width, layout))
           << attribute("height", translateScale(_height, layout)) << fillStyle().toString(layout)
           << strokeStyle().toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2& off) override {

        _edge += off;
    }

  private:
    Point _edge{};
    double _width;
    double _height;
};

class Line : public Shape {
  public:
    Line(Point startPoint, Point endPoint, Stroke const& strokeValue = Stroke())
        : Shape(Fill(), strokeValue), _start_point(std::move(startPoint)),
          _end_point(std::move(endPoint)) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("line") << attribute("x1", translateX(_start_point, layout))
           << attribute("y1", translateY(_start_point, layout))
           << attribute("x2", translateX(_end_point, layout))
           << attribute("y2", translateY(_end_point, layout)) << strokeStyle().toString(layout)
           << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2& off) override {
        _start_point += off;
        _end_point += off;
    }

  private:
    Point _start_point{};
    Point _end_point{};
};

class Polygon : public Shape {
  public:
    explicit Polygon(Fill const& fillValue = Fill(), Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue) {}
    explicit Polygon(Stroke const& strokeValue = Stroke()) : Shape(Fill(), strokeValue) {}
    auto operator<<(Point const& point) -> Polygon& {
        _points.push_back(point);
        return *this;
    }
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("polygon");

        ss << "points=\"";
        for (const auto& point : _points) {
            ss << translateX(point, layout) << "," << translateY(point, layout) << " ";
        }
        ss << "\" ";

        ss << fillStyle().toString(layout) << strokeStyle().toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2& off) override {
        for (auto& point : _points) {

            point += off;
        }
    }

  private:
    std::vector<Point> _points{};
};

class Path : public Shape {
  public:
    explicit Path(Fill const& fillValue = Fill(), Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue) {
        startNewSubPath();
    }
    explicit Path(Stroke const& strokeValue = Stroke()) : Shape(Fill(), strokeValue) {
        startNewSubPath();
    }
    auto operator<<(Point const& point) -> Path& {
        _paths.back().points().push_back(point);
        return *this;
    }

    void startNewSubPath() {
        if (_paths.empty() || !_paths.back().points().empty()) {
            _paths.emplace_back();
        }
    }

    void closeSubPath() {
        _paths.back().setClosed(true);
    }

    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
        ss << elemStart("path");

        ss << "d=\"";
        for (const auto& subpath : _paths) {
            if (subpath.empty()) {
                continue;
            }

            ss << "M";
            for (const auto& point : subpath.points()) {
                ss << translateX(point, layout) << "," << translateY(point, layout) << " ";
            }
            if (subpath.isClosed()) {
                ss << "z ";
            }
        }
        ss << "\" ";
        ss << "fill-rule=\"nonzero\" ";

        ss << fillStyle().toString(layout) << strokeStyle().toString(layout) << emptyElemEnd();
        return ss.str();
    }

    void offset(const laby::Kernel::Vector_2& off) override {
        for (auto& subpath : _paths) {
            for (auto& point : subpath.points()) {
                point = point + off;
            }
        }
    }

  private:
    std::vector<laby::Polyline> _paths{};
};

class Polyline : public Shape {
  public:
    explicit Polyline(Fill const& fillValue = Fill(), Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue) {}
    explicit Polyline(Stroke const& strokeValue = Stroke()) : Shape(Fill(), strokeValue) {}
    explicit Polyline(std::vector<Point> const& pointsValue, Fill const& fillValue = Fill(),
                      Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue), _points(pointsValue) {}
    auto operator<<(Point const& point) -> Polyline& {
        _points.push_back(point);
        return *this;
    }
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("polyline");

        ss << "points=\"";
        for (const auto& point : _points) {
            ss << translateX(point, layout) << "," << translateY(point, layout) << " ";
        }
        ss << "\" ";

        ss << fillStyle().toString(layout) << strokeStyle().toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2& off) override {
        for (auto& point : _points) {

            point = point + off;
        }
    }

  private:
    std::vector<Point> _points{};
};

class Text : public Shape {
  public:
    Text(Point originPoint, std::string textContent, Fill const& fillValue = Fill(),
         Font fontValue = Font(), Stroke const& strokeValue = Stroke())
        : Shape(fillValue, strokeValue), _origin(std::move(originPoint)),
          _content(std::move(textContent)), _font(std::move(fontValue)) {}
    [[nodiscard]] auto toString(Layout const& layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("text") << attribute("x", translateX(_origin, layout))
           << attribute("y", translateY(_origin, layout)) << fillStyle().toString(layout)
           << strokeStyle().toString(layout) << _font.toString(layout) << ">" << _content
           << elemEnd("text");
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2& off) override {
        _origin = _origin + off;
    }

  private:
    Point _origin{};
    std::string _content;
    Font _font;
};

class DocumentSVG {
  public:
    explicit DocumentSVG(std::string filename, Layout layout = Layout())
        : _file_name(std::move(filename)), _layout(std::move(layout)) {
        std::cout << "Document SVG constructed" << '\n';
    }

    auto operator<<(Shape const& shape) -> DocumentSVG& {
        _body_nodes_str += shape.toString(_layout);
        return *this;
    }
    [[nodiscard]] auto toString() const -> std::string {
        std::stringstream ss;
        ss << "<?xml " << attribute("version", "1.0") <<                      //
            attribute("standalone", "no") <<                                  //
            "?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" " <<        //
            "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n<svg " << //
            attribute("width", _layout.dimensions().width(), "px") <<         //
            attribute("height", _layout.dimensions().height(), "px") <<       //
            attribute("viewBox", std::string("0 0 ") +
                                     std::to_string(_layout.dimensions().width()) + " " +
                                     std::to_string(_layout.dimensions().height()))
           <<                                                   //
            attribute("xmlns", "http://www.w3.org/2000/svg") << //
            attribute("version", "1.1") << ">\n"
           << //
            _body_nodes_str << elemEnd("svg");
        return ss.str();
    }
    [[nodiscard]] auto save() const -> bool {
        std::ofstream ofs(_file_name);
        if (!ofs.good()) {
            std::cout << "Document not SVG saved !!!" << '\n';
            return false;
        }
        ofs << toString();
        ofs.close();
        std::cout << "Document SVG saved" << '\n';
        return true;
    }

  private:
    std::string _file_name;
    Layout _layout;

    std::string _body_nodes_str;
};

} /* namespace svg */

#endif /* SVGWRITER_DOCUMENTSVG_H_ */

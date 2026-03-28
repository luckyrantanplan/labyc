/*
 * DocumentSVG.h
 *
 *  Created on: Sep 28, 2018
 *      Author: florian
 */

#ifndef SVGWRITER_DOCUMENTSVG_H_
#define SVGWRITER_DOCUMENTSVG_H_

#include <utility>
#include <utility>
#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include <iostream>

#include "../Polyline.h"

namespace svg {
// Utility XML/String Functions.
template<typename T>
inline auto attribute(std::string const & attribute_name, T const & value, std::string const & unit = "") -> std::string {
    std::stringstream ss;
    ss << attribute_name << "=\"" << value << unit << "\" ";
    return ss.str();
}
inline auto elemStart(std::string const & element_name) -> std::string {
    return "\t<" + element_name + " ";
}
inline auto elemEnd(std::string const & element_name) -> std::string {
    return "</" + element_name + ">\n";
}
inline auto emptyElemEnd() -> std::string {
    return "/>\n";
}

// Quick optional return type.  This allows functions to return an invalid
//  value if no good return is possible.  The user checks for validity
//  before using the returned value.
template<typename T>
class Optional {
public:
    explicit Optional(T const & val) :
            _valid(true), _type(val) {
    }
    Optional() :
            _valid(false), _type(T()) {
    }
    auto operator->() -> T * {
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
    Dimensions(double w, double h) :
            width(w), height(h) {
    }
    explicit Dimensions(double combined = 0) :
            width(combined), height(combined) {
    }
    double width;
    double height;
};

using Point=laby::Kernel::Point_2;

//struct Point {
//    Point(double x = 0, double y = 0) :
//            x(x), y(y) {
//    }
//    double x;
//    double y;
//};

// Defines the dimensions, scale, origin, and origin offset of the document.
struct Layout {
    enum Origin {
        TopLeft, BottomLeft, TopRight, BottomRight
    };

    explicit Layout(Dimensions const & dims = Dimensions(400, 300), Origin orig = BottomLeft, double sc = 1, Point  orig_offset = Point(0, 0)) :
            dimensions(dims), scale(sc), origin(orig), origin_offset(std::move(orig_offset)) {
    }
    Dimensions dimensions;
    double scale;
    Origin origin;
    Point origin_offset;
};

// Convert coordinates in user space to SVG native space.
inline auto translateX(const Point& p, Layout const & layout) -> double {
    const double x = CGAL::to_double(p.x());
    const double offx = CGAL::to_double(layout.origin_offset.x());
    if (layout.origin == Layout::BottomRight || layout.origin == Layout::TopRight) {
        return layout.dimensions.width - ((x + offx) * layout.scale);
    }         return (offx + x) * layout.scale;
}

inline auto translateY(const Point& p, Layout const & layout) -> double {
    const double y = CGAL::to_double(p.y());
    const double offy = CGAL::to_double(layout.origin_offset.y());
    if (layout.origin == Layout::BottomLeft || layout.origin == Layout::BottomRight) {
        return layout.dimensions.height - ((y + offy) * layout.scale);
    }         return (offy + y) * layout.scale;
}
inline auto translateScale(double dimension, Layout const & layout) -> double {
    return dimension * layout.scale;
}

class Serializeable {
public:
    Serializeable() = default;
    virtual ~Serializeable() = default
    ;
    [[nodiscard]] virtual auto toString(Layout const & layout) const -> std::string = 0;
};

class Color: public Serializeable {
public:
    enum Defaults {
        Transparent = -1, Aqua, Black, Blue, Brown, Cyan, Fuchsia, Green, Lime, Magenta, Orange, Purple, Red, Silver, White, Yellow
    };

    Color(int32_t r, int32_t g, int32_t b) :
            _transparent(false), _red(r), _green(g), _blue(b) {
    }
    explicit Color(Defaults color) :
            _transparent(false), _red(0), _green(0), _blue(0) {
        switch (color) {
        case Aqua:
            assign(0, 255, 255);
            break;
        case Black:
            assign(0, 0, 0);
            break;
        case Blue:
            assign(0, 0, 255);
            break;
        case Brown:
            assign(165, 42, 42);
            break;
        case Cyan:
            assign(0, 255, 255);
            break;
        case Fuchsia:
            assign(255, 0, 255);
            break;
        case Green:
            assign(0, 128, 0);
            break;
        case Lime:
            assign(0, 255, 0);
            break;
        case Magenta:
            assign(255, 0, 255);
            break;
        case Orange:
            assign(255, 165, 0);
            break;
        case Purple:
            assign(128, 0, 128);
            break;
        case Red:
            assign(255, 0, 0);
            break;
        case Silver:
            assign(192, 192, 192);
            break;
        case White:
            assign(255, 255, 255);
            break;
        case Yellow:
            assign(255, 255, 0);
            break;
        default:
            _transparent = true;
            break;
        }
    }
    ~Color() override = default;
    [[nodiscard]] auto toString(Layout const & /*layout*/) const -> std::string override {
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
    int _red;
    int32_t _green;
    int32_t _blue;

    void assign(int32_t r, int32_t g, int32_t b) {
        _red = r;
        _green = g;
        _blue = b;
    }
};

class Fill: public Serializeable {
public:
    explicit Fill(Color::Defaults col) :
            _color(col) {
    }
    explicit Fill(const Color& col = Color(Color::Transparent)) :
            _color(col) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << attribute("fill", _color.toString(layout));
        return ss.str();
    }
private:
    Color _color;
};

class Stroke: public Serializeable {
public:
    explicit Stroke(double w = -1, const Color& col = Color(Color::Transparent), bool nonScalingStroke = false) :
            _width(w), _color(col), _nonScaling(nonScalingStroke) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        // If stroke width is invalid.
        if (_width < 0) {
            return {};
}

        std::stringstream ss;
        ss << attribute("stroke-width", translateScale(_width, layout)) << attribute("stroke", _color.toString(layout));
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

class Font: public Serializeable {
public:
    explicit Font(double sz = 12, std::string  fam = "Verdana") :
            _size(sz), _family(std::move(fam)) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << attribute("font-size", translateScale(_size, layout)) << attribute("font-family", _family);
        return ss.str();
    }
private:
    double _size;
    std::string _family;
};

class Shape: public Serializeable {
public:
    explicit Shape(Fill  f = Fill(), Stroke  s = Stroke()) :
            fill(std::move(f)), stroke(std::move(s)) {
    }
    ~Shape() override = default;
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override = 0;
    virtual void offset(const laby::Kernel::Vector_2 & offset) = 0;
protected:
    Fill fill;
    Stroke stroke;
};
template<typename T>
inline auto vectorToString(std::vector<T> collection, Layout const & layout) -> std::string {
    std::string combinationStr;
    for (unsigned i = 0; i < collection.size(); ++i) {
        combinationStr += collection[i].toString(layout);
}

    return combinationStr;
}

class Circle: public Shape {
public:
    Circle(Point  c, double diameter, Fill const & f, Stroke const & s = Stroke()) :
            Shape(f, s), _center(std::move(c)), _radius(diameter / 2) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("circle") << attribute("cx", translateX(_center, layout)) << attribute("cy", translateY(_center, layout)) << attribute("r", translateScale(_radius, layout))
                << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & off) override {
        _center += off;

    }
private:
    Point _center;
    double _radius;
};

class Elipse: public Shape {
public:
    Elipse(Point  c, double w, double h, Fill const & f = Fill(), Stroke const & s = Stroke()) :
            Shape(f, s), _center(std::move(c)), _radius_width(w / 2), _radius_height(h / 2) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("ellipse") << attribute("cx", translateX(_center, layout)) << attribute("cy", translateY(_center, layout)) << attribute("rx", translateScale(_radius_width, layout))
                << attribute("ry", translateScale(_radius_height, layout)) << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & off) override {

        _center += off;
    }
private:
    Point _center;
    double _radius_width;
    double _radius_height;
};

class Rectangle: public Shape {
public:
    Rectangle(Point  e, double w, double h, Fill const & f = Fill(), Stroke const & s = Stroke()) :
            Shape(f, s), _edge(std::move(e)), _width(w), _height(h) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("rect") << attribute("x", translateX(_edge, layout)) << attribute("y", translateY(_edge, layout)) << attribute("width", translateScale(_width, layout))
                << attribute("height", translateScale(_height, layout)) << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & off) override {

        _edge += off;
    }
private:
    Point _edge;
    double _width;
    double _height;
};

class Line: public Shape {
public:
    Line(Point  sp, Point  ep, Stroke const & s = Stroke()) :
            Shape(Fill(), s), _start_point(std::move(sp)), _end_point(std::move(ep)) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("line") << attribute("x1", translateX(_start_point, layout)) << attribute("y1", translateY(_start_point, layout)) << attribute("x2", translateX(_end_point, layout))
                << attribute("y2", translateY(_end_point, layout)) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & off) override {
        _start_point += off;
        _end_point += off;
    }
private:
    Point _start_point;
    Point _end_point;
};

class Polygon: public Shape {
public:
    explicit Polygon(Fill const & f = Fill(), Stroke const & s = Stroke()) :
            Shape(f, s) {
    }
    explicit Polygon(Stroke const & s = Stroke()) :
            Shape(Fill(), s) {
    }
    auto operator<<(Point const & point) -> Polygon & {
        points.push_back(point);
        return *this;
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("polygon");

        ss << "points=\"";
        for (const auto & point : points) {
            ss << translateX(point, layout) << "," << translateY(point, layout) << " ";
}
        ss << "\" ";

        ss << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & off) override {
        for (auto & point : points) {

            point += off;
        }
    }
private:
    std::vector<Point> points;
};

class Path: public Shape {
public:
    explicit Path(Fill const & f = Fill(), Stroke const & s = Stroke()) :
            Shape(f, s) {
        startNewSubPath();
    }
    explicit Path(Stroke const & s = Stroke()) :
            Shape(Fill(), s) {
        startNewSubPath();
    }
    auto operator<<(Point const & point) -> Path & {
        paths.back().points.push_back(point);
        return *this;
    }

    void startNewSubPath() {
        if (paths.empty() || !paths.back().points.empty()) {
            paths.emplace_back();
}
    }

    void closeSubPath() {
        paths.back().closed = true;
    }

    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
        ss << elemStart("path");

        ss << "d=\"";
        for (const auto & subpath : paths) {
            if (subpath.empty()) {
                continue;
}

            ss << "M";
            for (const auto & point : subpath.points) {
                ss << translateX(point, layout) << "," << translateY(point, layout) << " ";
}
            if (subpath.closed) {
                ss << "z ";
            }
        }
        ss << "\" ";
        ss << "fill-rule=\"nonzero\" ";

        ss << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }

    void offset(const laby::Kernel::Vector_2 & off) override {
        for (auto& subpath : paths) {
            for (auto& point : subpath.points) {
                point = point + off;

            }
}
    }
private:
    std::vector<laby::Polyline> paths;
};

class Polyline: public Shape {
public:
    explicit Polyline(Fill const & f = Fill(), Stroke const & s = Stroke()) :
            Shape(f, s) {
    }
    explicit Polyline(Stroke const & s = Stroke()) :
            Shape(Fill(), s) {
    }
    explicit Polyline(std::vector<Point> const & pts, Fill const & f = Fill(), Stroke const & s = Stroke()) :
            Shape(f, s), points(pts) {
    }
    auto operator<<(Point const & point) -> Polyline & {
        points.push_back(point);
        return *this;
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("polyline");

        ss << "points=\"";
        for (const auto & point : points) {
            ss << translateX(point, layout) << "," << translateY(point, layout) << " ";
}
        ss << "\" ";

        ss << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & off) override {
        for (auto & point : points) {

            point = point + off;

        }
    }
    std::vector<Point> points;
};

class Text: public Shape {
public:
    Text(Point  orig, std::string  text, Fill const & f = Fill(), Font  fnt = Font(), Stroke const & s = Stroke()) :
            Shape(f, s), _origin(std::move(orig)), _content(std::move(text)), _font(std::move(fnt)) {
    }
    [[nodiscard]] auto toString(Layout const & layout) const -> std::string override {
        std::stringstream ss;
        ss << elemStart("text") << attribute("x", translateX(_origin, layout)) << attribute("y", translateY(_origin, layout)) << fill.toString(layout) << stroke.toString(layout) << _font.toString(layout)
                << ">" << _content << elemEnd("text");
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & off) override {
        _origin = _origin + off;

    }
private:
    Point _origin;
    std::string _content;
    Font _font;
};

class DocumentSVG {
public:
    explicit DocumentSVG(std::string  fname, Layout lay = Layout()) :
            _file_name(std::move(fname)), _layout(std::move(std::move(lay))) {
        std::cout << "Document SVG constructed" << '\n';
    }

    auto operator<<(Shape const & shape) -> DocumentSVG & {
        _body_nodes_str += shape.toString(_layout);
        return *this;
    }
    [[nodiscard]] auto toString() const -> std::string {
        std::stringstream ss;
        ss << "<?xml " << attribute("version", "1.0") << //
                attribute("standalone", "no") << //
                "?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" " << //
                "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n<svg " << //
                attribute("width", _layout.dimensions.width, "px") << //
                attribute("height", _layout.dimensions.height, "px") << //
                attribute("viewBox", std::string("0 0 ") + std::to_string(_layout.dimensions.width) + " " + std::to_string(_layout.dimensions.height)) << //
                attribute("xmlns", "http://www.w3.org/2000/svg") << //
                attribute("version", "1.1") << ">\n" << //
                _body_nodes_str << elemEnd("svg");
        return ss.str();
    }
    [[nodiscard]] auto save() const -> bool {
        std::ofstream ofs(_file_name.c_str());
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

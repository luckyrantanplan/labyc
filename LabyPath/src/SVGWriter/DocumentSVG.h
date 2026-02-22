/*
 * DocumentSVG.h
 *
 *  Created on: Sep 28, 2018
 *      Author: florian
 */

#ifndef SVGWRITER_DOCUMENTSVG_H_
#define SVGWRITER_DOCUMENTSVG_H_

#include <vector>
#include <string>
#include <sstream>
#include <fstream>

#include <iostream>

#include "../Polyline.h"

namespace svg {
// Utility XML/String Functions.
template<typename T>
inline std::string attribute(std::string const & attribute_name, T const & value, std::string const & unit = "") {
    std::stringstream ss;
    ss << attribute_name << "=\"" << value << unit << "\" ";
    return ss.str();
}
inline std::string elemStart(std::string const & element_name) {
    return "\t<" + element_name + " ";
}
inline std::string elemEnd(std::string const & element_name) {
    return "</" + element_name + ">\n";
}
inline std::string emptyElemEnd() {
    return "/>\n";
}

// Quick optional return type.  This allows functions to return an invalid
//  value if no good return is possible.  The user checks for validity
//  before using the returned value.
template<typename T>
class optional {
public:
    optional<T>(T const & type) :
            valid(true), type(type) {
    }
    optional<T>() :
            valid(false), type(T()) {
    }
    T * operator->() {
        // If we try to access an invalid value, an exception is thrown.
        if (!valid)
            throw std::exception();

        return &type;
    }
    // Test for validity.
    bool operator!() const {
        return !valid;
    }
private:
    bool valid;
    T type;
};

struct Dimensions {
    Dimensions(double width, double height) :
            width(width), height(height) {
    }
    Dimensions(double combined = 0) :
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

    Layout(Dimensions const & dimensions = Dimensions(400, 300), Origin origin = BottomLeft, double scale = 1, Point const & origin_offset = Point(0, 0)) :
            dimensions(dimensions), scale(scale), origin(origin), origin_offset(origin_offset) {
    }
    Dimensions dimensions;
    double scale;
    Origin origin;
    Point origin_offset;
};

// Convert coordinates in user space to SVG native space.
inline double translateX(const Point& p, Layout const & layout) {
    const double x = CGAL::to_double(p.x());
    const double offx = CGAL::to_double(layout.origin_offset.x());
    if (layout.origin == Layout::BottomRight || layout.origin == Layout::TopRight)
        return layout.dimensions.width - ((x + offx) * layout.scale);
    else
        return (offx + x) * layout.scale;
}

inline double translateY(const Point& p, Layout const & layout) {
    const double y = CGAL::to_double(p.y());
    const double offy = CGAL::to_double(layout.origin_offset.y());
    if (layout.origin == Layout::BottomLeft || layout.origin == Layout::BottomRight)
        return layout.dimensions.height - ((y + offy) * layout.scale);
    else
        return (offy + y) * layout.scale;
}
inline double translateScale(double dimension, Layout const & layout) {
    return dimension * layout.scale;
}

class Serializeable {
public:
    Serializeable() {
    }
    virtual ~Serializeable() {
    }
    ;
    virtual std::string toString(Layout const & layout) const = 0;
};

class Color: public Serializeable {
public:
    enum Defaults {
        Transparent = -1, Aqua, Black, Blue, Brown, Cyan, Fuchsia, Green, Lime, Magenta, Orange, Purple, Red, Silver, White, Yellow
    };

    Color(int32_t r, int32_t g, int32_t b) :
            transparent(false), red(r), green(g), blue(b) {
    }
    Color(Defaults color) :
            transparent(false), red(0), green(0), blue(0) {
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
            transparent = true;
            break;
        }
    }
    virtual ~Color() {
    }
    std::string toString(Layout const &) const {
        std::stringstream ss;
        if (transparent)
            ss << "none";
        else
            ss << "rgb(" << red << "," << green << "," << blue << ")";
        return ss.str();
    }
private:
    bool transparent;
    int red;
    int32_t green;
    int32_t blue;

    void assign(int32_t r, int32_t g, int32_t b) {
        red = r;
        green = g;
        blue = b;
    }
};

class Fill: public Serializeable {
public:
    Fill(Color::Defaults color) :
            color(color) {
    }
    Fill(Color color = Color::Transparent) :
            color(color) {
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << attribute("fill", color.toString(layout));
        return ss.str();
    }
private:
    Color color;
};

class Stroke: public Serializeable {
public:
    Stroke(double width = -1, Color color = Color::Transparent, bool nonScalingStroke = false) :
            width(width), color(color), nonScaling(nonScalingStroke) {
    }
    std::string toString(Layout const & layout) const {
        // If stroke width is invalid.
        if (width < 0)
            return std::string();

        std::stringstream ss;
        ss << attribute("stroke-width", translateScale(width, layout)) << attribute("stroke", color.toString(layout));
        ss << attribute("stroke-linecap", "round");
        ss << attribute(" stroke-linejoin", "round");
        if (nonScaling)
            ss << attribute("vector-effect", "non-scaling-stroke");
        return ss.str();
    }
private:
    double width;
    Color color;
    bool nonScaling;
};

class Font: public Serializeable {
public:
    Font(double size = 12, std::string const & family = "Verdana") :
            size(size), family(family) {
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << attribute("font-size", translateScale(size, layout)) << attribute("font-family", family);
        return ss.str();
    }
private:
    double size;
    std::string family;
};

class Shape: public Serializeable {
public:
    Shape(Fill const & fill = Fill(), Stroke const & stroke = Stroke()) :
            fill(fill), stroke(stroke) {
    }
    virtual ~Shape() {
    }
    virtual std::string toString(Layout const & layout) const = 0;
    virtual void offset(const laby::Kernel::Vector_2 & offset) = 0;
protected:
    Fill fill;
    Stroke stroke;
};
template<typename T>
inline std::string vectorToString(std::vector<T> collection, Layout const & layout) {
    std::string combination_str;
    for (unsigned i = 0; i < collection.size(); ++i)
        combination_str += collection[i].toString(layout);

    return combination_str;
}

class Circle: public Shape {
public:
    Circle(Point const & center, double diameter, Fill const & fill, Stroke const & stroke = Stroke()) :
            Shape(fill, stroke), center(center), radius(diameter / 2) {
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << elemStart("circle") << attribute("cx", translateX(center, layout)) << attribute("cy", translateY(center, layout)) << attribute("r", translateScale(radius, layout))
                << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & offset) {
        center += offset;

    }
private:
    Point center;
    double radius;
};

class Elipse: public Shape {
public:
    Elipse(Point const & center, double width, double height, Fill const & fill = Fill(), Stroke const & stroke = Stroke()) :
            Shape(fill, stroke), center(center), radius_width(width / 2), radius_height(height / 2) {
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << elemStart("ellipse") << attribute("cx", translateX(center, layout)) << attribute("cy", translateY(center, layout)) << attribute("rx", translateScale(radius_width, layout))
                << attribute("ry", translateScale(radius_height, layout)) << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & offset) {

        center += offset;
    }
private:
    Point center;
    double radius_width;
    double radius_height;
};

class Rectangle: public Shape {
public:
    Rectangle(Point const & edge, double width, double height, Fill const & fill = Fill(), Stroke const & stroke = Stroke()) :
            Shape(fill, stroke), edge(edge), width(width), height(height) {
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << elemStart("rect") << attribute("x", translateX(edge, layout)) << attribute("y", translateY(edge, layout)) << attribute("width", translateScale(width, layout))
                << attribute("height", translateScale(height, layout)) << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & offset) {

        edge += offset;
    }
private:
    Point edge;
    double width;
    double height;
};

class Line: public Shape {
public:
    Line(Point const & start_point, Point const & end_point, Stroke const & stroke = Stroke()) :
            Shape(Fill(), stroke), start_point(start_point), end_point(end_point) {
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << elemStart("line") << attribute("x1", translateX(start_point, layout)) << attribute("y1", translateY(start_point, layout)) << attribute("x2", translateX(end_point, layout))
                << attribute("y2", translateY(end_point, layout)) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & offset) {
        start_point += offset;
        end_point += offset;
    }
private:
    Point start_point;
    Point end_point;
};

class Polygon: public Shape {
public:
    Polygon(Fill const & fill = Fill(), Stroke const & stroke = Stroke()) :
            Shape(fill, stroke) {
    }
    Polygon(Stroke const & stroke = Stroke()) :
            Shape(Color::Transparent, stroke) {
    }
    Polygon & operator<<(Point const & point) {
        points.push_back(point);
        return *this;
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << elemStart("polygon");

        ss << "points=\"";
        for (unsigned i = 0; i < points.size(); ++i)
            ss << translateX(points[i], layout) << "," << translateY(points[i], layout) << " ";
        ss << "\" ";

        ss << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & offset) {
        for (unsigned i = 0; i < points.size(); ++i) {

            points[i] += offset;
        }
    }
private:
    std::vector<Point> points;
};

class Path: public Shape {
public:
    Path(Fill const & fill = Fill(), Stroke const & stroke = Stroke()) :
            Shape(fill, stroke) {
        startNewSubPath();
    }
    Path(Stroke const & stroke = Stroke()) :
            Shape(Color::Transparent, stroke) {
        startNewSubPath();
    }
    Path & operator<<(Point const & point) {
        paths.back().points.push_back(point);
        return *this;
    }

    void startNewSubPath() {
        if (paths.empty() || 0 < paths.back().points.size())
            paths.emplace_back();
    }

    void closeSubPath() {
        paths.back().closed = true;
    }

    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << std::setprecision(std::numeric_limits<long double>::digits10 + 1);
        ss << elemStart("path");

        ss << "d=\"";
        for (const auto & subpath : paths) {
            if (subpath.empty())
                continue;

            ss << "M";
            for (const auto & point : subpath.points)
                ss << translateX(point, layout) << "," << translateY(point, layout) << " ";
            if (subpath.closed) {
                ss << "z ";
            }
        }
        ss << "\" ";
        ss << "fill-rule=\"nonzero\" ";

        ss << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }

    void offset(const laby::Kernel::Vector_2 & offset) {
        for (auto& subpath : paths)
            for (auto& point : subpath.points) {
                point = point + offset;

            }
    }
private:
    std::vector<laby::Polyline> paths;
};

class Polyline: public Shape {
public:
    Polyline(Fill const & fill = Fill(), Stroke const & stroke = Stroke()) :
            Shape(fill, stroke) {
    }
    Polyline(Stroke const & stroke = Stroke()) :
            Shape(Color::Transparent, stroke) {
    }
    Polyline(std::vector<Point> const & points, Fill const & fill = Fill(), Stroke const & stroke = Stroke()) :
            Shape(fill, stroke), points(points) {
    }
    Polyline & operator<<(Point const & point) {
        points.push_back(point);
        return *this;
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << elemStart("polyline");

        ss << "points=\"";
        for (unsigned i = 0; i < points.size(); ++i)
            ss << translateX(points[i], layout) << "," << translateY(points[i], layout) << " ";
        ss << "\" ";

        ss << fill.toString(layout) << stroke.toString(layout) << emptyElemEnd();
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 & offset) {
        for (unsigned i = 0; i < points.size(); ++i) {

            points[i] = points[i] + offset;

        }
    }
    std::vector<Point> points;
};

class Text: public Shape {
public:
    Text(Point const & origin, std::string const & content, Fill const & fill = Fill(), Font const & font = Font(), Stroke const & stroke = Stroke()) :
            Shape(fill, stroke), origin(origin), content(content), font(font) {
    }
    std::string toString(Layout const & layout) const {
        std::stringstream ss;
        ss << elemStart("text") << attribute("x", translateX(origin, layout)) << attribute("y", translateY(origin, layout)) << fill.toString(layout) << stroke.toString(layout) << font.toString(layout)
                << ">" << content << elemEnd("text");
        return ss.str();
    }
    void offset(const laby::Kernel::Vector_2 &offset) {
        origin = origin + offset;

    }
private:
    Point origin;
    std::string content;
    Font font;
};

class DocumentSVG {
public:
    DocumentSVG(std::string const & file_name, Layout layout = Layout()) :
            file_name(file_name), layout(layout) {
        std::cout << "Document SVG constructed" << std::endl;
    }

    DocumentSVG & operator<<(Shape const & shape) {
        body_nodes_str += shape.toString(layout);
        return *this;
    }
    std::string toString() const {
        std::stringstream ss;
        ss << "<?xml " << attribute("version", "1.0") << //
                attribute("standalone", "no") << //
                "?>\n<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" " << //
                "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n<svg " << //
                attribute("width", layout.dimensions.width, "px") << //
                attribute("height", layout.dimensions.height, "px") << //
                attribute("viewBox", std::string("0 0 ") + std::to_string(layout.dimensions.width) + " " + std::to_string(layout.dimensions.height)) << //
                attribute("xmlns", "http://www.w3.org/2000/svg") << //
                attribute("version", "1.1") << ">\n" << //
                body_nodes_str << elemEnd("svg");
        return ss.str();
    }
    bool save() const {
        std::ofstream ofs(file_name.c_str());
        if (!ofs.good()) {
            std::cout << "Document not SVG saved !!!" << std::endl;
            return false;
        }
        ofs << toString();
        ofs.close();
        std::cout << "Document SVG saved" << std::endl;
        return true;
    }
private:
    std::string file_name;
    Layout layout;

    std::string body_nodes_str;
};

} /* namespace svg */

#endif /* SVGWRITER_DOCUMENTSVG_H_ */

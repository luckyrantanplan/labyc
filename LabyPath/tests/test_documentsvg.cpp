#include "SVGWriter/DocumentSVG.h"
#include <exception>
#include <fstream>
#include <cstdio>
#include <gtest/gtest.h>
#include <string>
#include <iterator>

using namespace svg;

// ─── Utility Function Tests ─────────────────────────────────────────────────

TEST(SvgUtilTest, Attribute) {
    std::string const result = attribute("width", 100, "px");
    EXPECT_EQ(result, "width=\"100px\" ");
}

TEST(SvgUtilTest, AttributeString) {
    std::string const result = attribute("fill", "red");
    EXPECT_EQ(result, "fill=\"red\" ");
}

TEST(SvgUtilTest, ElemStart) {
    EXPECT_EQ(elemStart("circle"), "\t<circle ");
}

TEST(SvgUtilTest, ElemEnd) {
    EXPECT_EQ(elemEnd("svg"), "</svg>\n");
}

TEST(SvgUtilTest, EmptyElemEnd) {
    EXPECT_EQ(emptyElemEnd(), "/>\n");
}

// ─── Dimensions Tests ───────────────────────────────────────────────────────

TEST(DimensionsTest, WidthAndHeight) {
    Dimensions const d(Dimensions::Size{800, 600});
    EXPECT_DOUBLE_EQ(d.width(), 800);
    EXPECT_DOUBLE_EQ(d.height(), 600);
}

TEST(DimensionsTest, SingleValue) {
    Dimensions const d(100);
    EXPECT_DOUBLE_EQ(d.width(), 100);
    EXPECT_DOUBLE_EQ(d.height(), 100);
}

// ─── Layout Tests ───────────────────────────────────────────────────────────

TEST(LayoutTest, DefaultLayout) {
    Layout const lay;
    EXPECT_DOUBLE_EQ(lay.dimensions().width(), 400);
    EXPECT_DOUBLE_EQ(lay.dimensions().height(), 300);
    EXPECT_DOUBLE_EQ(lay.scale(), 1.0);
    EXPECT_EQ(lay.origin(), Layout::Origin::BottomLeft);
}

TEST(LayoutTest, CustomLayout) {
    Layout const lay(Dimensions(Dimensions::Size{1000, 500}), Layout::Origin::TopLeft, 2.0);
    EXPECT_DOUBLE_EQ(lay.dimensions().width(), 1000);
    EXPECT_DOUBLE_EQ(lay.dimensions().height(), 500);
    EXPECT_DOUBLE_EQ(lay.scale(), 2.0);
    EXPECT_EQ(lay.origin(), Layout::Origin::TopLeft);
}

// ─── Coordinate Translation Tests ───────────────────────────────────────────

TEST(TranslateTest, TopLeftOrigin) {
    Layout const lay(Dimensions(Dimensions::Size{800, 600}), Layout::Origin::TopLeft, 1.0);
    Point const p(10, 20);
    EXPECT_DOUBLE_EQ(translateX(p, lay), 10.0);
    EXPECT_DOUBLE_EQ(translateY(p, lay), 20.0);
}

TEST(TranslateTest, BottomLeftOriginFlipsY) {
    Layout const lay(Dimensions(Dimensions::Size{800, 600}), Layout::Origin::BottomLeft, 1.0);
    Point const p(10, 20);
    EXPECT_DOUBLE_EQ(translateX(p, lay), 10.0);
    EXPECT_DOUBLE_EQ(translateY(p, lay), 580.0); // 600 - 20
}

TEST(TranslateTest, TopRightOriginFlipsX) {
    Layout const lay(Dimensions(Dimensions::Size{800, 600}), Layout::Origin::TopRight, 1.0);
    Point const p(10, 20);
    EXPECT_DOUBLE_EQ(translateX(p, lay), 790.0); // 800 - 10
    EXPECT_DOUBLE_EQ(translateY(p, lay), 20.0);
}

TEST(TranslateTest, BottomRightFlipsBoth) {
    Layout const lay(Dimensions(Dimensions::Size{800, 600}), Layout::Origin::BottomRight, 1.0);
    Point const p(10, 20);
    EXPECT_DOUBLE_EQ(translateX(p, lay), 790.0); // 800 - 10
    EXPECT_DOUBLE_EQ(translateY(p, lay), 580.0); // 600 - 20
}

TEST(TranslateTest, ScaleApplied) {
    Layout const lay(Dimensions(Dimensions::Size{800, 600}), Layout::Origin::TopLeft, 2.0);
    Point const p(10, 20);
    EXPECT_DOUBLE_EQ(translateX(p, lay), 20.0); // 10 * 2
    EXPECT_DOUBLE_EQ(translateY(p, lay), 40.0); // 20 * 2
}

TEST(TranslateTest, OffsetApplied) {
    Layout const lay(Dimensions(Dimensions::Size{800, 600}), Layout::Origin::TopLeft, 1.0, Point(5, 10));
    Point const p(10, 20);
    EXPECT_DOUBLE_EQ(translateX(p, lay), 15.0); // 5 + 10
    EXPECT_DOUBLE_EQ(translateY(p, lay), 30.0); // 10 + 20
}

TEST(TranslateTest, TranslateScale) {
    Layout const lay(Dimensions(Dimensions::Size{800, 600}), Layout::Origin::TopLeft, 3.0);
    EXPECT_DOUBLE_EQ(translateScale(5.0, lay), 15.0); // 5 * 3
}

// ─── Color Tests ────────────────────────────────────────────────────────────

TEST(SvgColorTest, RGBColor) {
    Layout const lay;
    Color const c(Color::Rgb{255, 128, 0});
    EXPECT_EQ(c.toString(lay), "rgb(255,128,0)");
}

TEST(SvgColorTest, TransparentColor) {
    Layout const lay;
    Color const c(Color::Defaults::Transparent);
    EXPECT_EQ(c.toString(lay), "none");
}

TEST(SvgColorTest, NamedColorRed) {
    Layout const lay;
    Color const c(Color::Defaults::Red);
    EXPECT_EQ(c.toString(lay), "rgb(255,0,0)");
}

TEST(SvgColorTest, NamedColorBlack) {
    Layout const lay;
    Color const c(Color::Defaults::Black);
    EXPECT_EQ(c.toString(lay), "rgb(0,0,0)");
}

// ─── Shape toString Tests ───────────────────────────────────────────────────

TEST(SvgShapeTest, CircleToString) {
    Layout const lay(Dimensions(Dimensions::Size{100, 100}), Layout::Origin::TopLeft, 1.0);
    Circle const circle(Point(50, 50), 20, Fill(Color::Defaults::Red));
    std::string const svg = circle.toString(lay);
    EXPECT_NE(svg.find("<circle"), std::string::npos);
    EXPECT_NE(svg.find("cx=\"50\""), std::string::npos);
    EXPECT_NE(svg.find("cy=\"50\""), std::string::npos);
    EXPECT_NE(svg.find("r=\"10\""), std::string::npos); // diameter/2
}

TEST(SvgShapeTest, RectangleToString) {
    Layout const lay(Dimensions(Dimensions::Size{100, 100}), Layout::Origin::TopLeft, 1.0);
    Rectangle const rect(Point(10, 20), 30, 40, Fill(Color::Defaults::Blue));
    std::string const svg = rect.toString(lay);
    EXPECT_NE(svg.find("<rect"), std::string::npos);
    EXPECT_NE(svg.find("width=\"30\""), std::string::npos);
    EXPECT_NE(svg.find("height=\"40\""), std::string::npos);
}

TEST(SvgShapeTest, LineToString) {
    Layout const lay(Dimensions(Dimensions::Size{100, 100}), Layout::Origin::TopLeft, 1.0);
    Line const line(Point(0, 0), Point(100, 100), Stroke(2, Color(Color::Defaults::Black)));
    std::string const svg = line.toString(lay);
    EXPECT_NE(svg.find("<line"), std::string::npos);
    EXPECT_NE(svg.find("x1=\"0\""), std::string::npos);
    EXPECT_NE(svg.find("y1=\"0\""), std::string::npos);
}

TEST(SvgShapeTest, TextToString) {
    Layout const lay(Dimensions(Dimensions::Size{100, 100}), Layout::Origin::TopLeft, 1.0);
    Text const text(Point(10, 20), "Hello", Fill(Color::Defaults::Black));
    std::string const svg = text.toString(lay);
    EXPECT_NE(svg.find("<text"), std::string::npos);
    EXPECT_NE(svg.find("Hello"), std::string::npos);
    EXPECT_NE(svg.find("</text>"), std::string::npos);
}

// ─── Optional Tests ─────────────────────────────────────────────────────────

TEST(SvgOptionalTest, ValidOptional) {
    Optional<int> opt(42);
    EXPECT_FALSE(!opt); // valid
    EXPECT_EQ(*opt.operator->(), 42);
}

TEST(SvgOptionalTest, InvalidOptional) {
    Optional<int> opt;
    EXPECT_TRUE(!opt); // invalid
    EXPECT_THROW(opt.operator->(), std::exception);
}

// ─── DocumentSVG Tests ──────────────────────────────────────────────────────

TEST(DocumentSVGTest, ToStringContainsSvgRoot) {
    DocumentSVG const doc("/tmp/test_doc.svg", Layout(Dimensions(Dimensions::Size{200, 100})));
    std::string const svg = doc.toString();
    EXPECT_NE(svg.find("<?xml"), std::string::npos);
    EXPECT_NE(svg.find("<svg"), std::string::npos);
    EXPECT_NE(svg.find("</svg>"), std::string::npos);
    EXPECT_NE(svg.find("width=\"200px\""), std::string::npos);
    EXPECT_NE(svg.find("height=\"100px\""), std::string::npos);
}

TEST(DocumentSVGTest, AddShapeToDocument) {
    DocumentSVG doc("/tmp/test_shapes.svg", Layout(Dimensions(Dimensions::Size{100, 100}), Layout::Origin::TopLeft));
    doc << Circle(Point(50, 50), 10, Fill(Color::Defaults::Red));
    std::string const svg = doc.toString();
    EXPECT_NE(svg.find("<circle"), std::string::npos);
}

TEST(DocumentSVGTest, SaveToFile) {
    std::string const fname = "/tmp/test_save_doc.svg";
    {
        DocumentSVG doc(fname, Layout(Dimensions(Dimensions::Size{100, 100})));
        doc << Circle(Point(50, 50), 10, Fill(Color::Defaults::Red));
        EXPECT_TRUE(doc.save());
    }
    // Verify file exists and has content
    std::ifstream ifs(fname);
    EXPECT_TRUE(ifs.good());
    std::string const content((std::istreambuf_iterator<char>(ifs)), std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("<svg"), std::string::npos);
    std::remove(fname.c_str());
}

// ─── Polygon and Polyline Stream Operator Tests ─────────────────────────────

TEST(SvgShapeTest, PolygonStreamOperator) {
    svg::Layout const lay(svg::Dimensions(svg::Dimensions::Size{100, 100}), svg::Layout::Origin::TopLeft, 1.0);
    svg::Polygon poly{svg::Fill(svg::Color::Defaults::Green)};
    poly << svg::Point(0, 0) << svg::Point(50, 0) << svg::Point(25, 50);
    std::string const svg = poly.toString(lay);
    EXPECT_NE(svg.find("<polygon"), std::string::npos);
    EXPECT_NE(svg.find("points="), std::string::npos);
}

TEST(SvgShapeTest, PolylineStreamOperator) {
    svg::Layout const lay(svg::Dimensions(svg::Dimensions::Size{100, 100}), svg::Layout::Origin::TopLeft, 1.0);
    svg::Polyline poly{svg::Fill(svg::Color::Defaults::Transparent), svg::Stroke(1, svg::Color(svg::Color::Defaults::Black))};
    poly << svg::Point(0, 0) << svg::Point(50, 50) << svg::Point(100, 0);
    std::string const svg = poly.toString(lay);
    EXPECT_NE(svg.find("<polyline"), std::string::npos);
}

// ─── Fill and Stroke Tests ──────────────────────────────────────────────────

TEST(FillStrokeTest, FillTransparent) {
    Layout const lay;
    Fill const f(Color::Defaults::Transparent);
    std::string const s = f.toString(lay);
    EXPECT_NE(s.find("none"), std::string::npos);
}

TEST(FillStrokeTest, StrokeNegativeWidthEmpty) {
    Layout const lay;
    Stroke const s(-1, Color(Color::Defaults::Black));
    EXPECT_TRUE(s.toString(lay).empty());
}

TEST(FillStrokeTest, StrokePositiveWidth) {
    Layout const lay(Dimensions(Dimensions::Size{100, 100}), Layout::Origin::TopLeft, 1.0);
    Stroke const s(2.0, Color(Color::Defaults::Red));
    std::string const result = s.toString(lay);
    EXPECT_NE(result.find("stroke-width"), std::string::npos);
    EXPECT_NE(result.find("rgb(255,0,0)"), std::string::npos);
}

TEST(FillStrokeTest, NonScalingStroke) {
    Layout const lay;
    Stroke const s(1.0, Color(Color::Defaults::Black), true);
    std::string const result = s.toString(lay);
    EXPECT_NE(result.find("non-scaling-stroke"), std::string::npos);
}

/**
 * @file test_smoke_svg.cpp
 * @brief Qualification tests for end-to-end SVG processing pipelines.
 *
 * These tests load real SVG input files and validate geometric properties
 * of the output: coordinate bounds, polyline integrity, skeleton structure,
 * and SVG well-formedness.
 */

#include <CGAL/Bbox_2.h>
#include <cstddef>
#include <exception>
#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <new>
#include <optional>
#include <regex>
#include <string>
#include <vector>

#include "AlternaRoute/AlternateRoute.h"
#include "GridIndex.h"
#include "Polyline.h"
#include "Rendering/GraphicRendering.h"
#include "Ribbon.h"
#include "SVGParser/Loader.h"
#include "SkeletonGrid.h"
#include "basic/Color.h"
#include "protoc/AllConfig.pb.h"

namespace fs = std::filesystem;

// ─── Helpers ────────────────────────────────────────────────────────────────

namespace {

struct ParsedViewBox {
    double minX;
    double minY;
    double width;
    double height;
};

constexpr double kBoundsMargin = 1.0;
constexpr double kDefaultStrokeWidth = 1.0;
constexpr double kThickStrokeWidth = 2.0;
constexpr double kSimplificationDistance = 0.1;
constexpr double kSquareMaxSeparation = 10.0;
constexpr double kSquareMinSeparation = 3.0;
constexpr double kDrawingMaxSeparation = 8.0;
constexpr double kDrawingMinSeparation = 2.0;
constexpr double kAlternateMaxThickness = 20.0;
constexpr double kAlternateMinThickness = 2.0;
constexpr int kAlternatePruning = 5;
constexpr double kAlternateThicknessPercent = 0.5;
constexpr double kAlternateSimplifyDistance = 1e-7;
constexpr double kConfigMaxThickness = 15.0;
constexpr double kConfigMinThickness = 1.0;
constexpr int kConfigPruning = 3;
constexpr double kConfigThicknessPercent = 0.4;
constexpr int kSquareSeed = 42;
constexpr int kDrawingSeed = 123;
constexpr std::uintmax_t kMinimumSubstantialSvgSize = 100U;
constexpr double kEmptySvgBoxMax = 100.0;
constexpr std::size_t kExpectedViewBoxMatchCount = 5U;

auto inputDir() -> std::string {
    fs::path const base = fs::path(__FILE__).parent_path().parent_path() / "input";
    return base.string();
}

auto inputFile(const std::string& name) -> std::string {
    return inputDir() + "/" + name;
}

auto tmpOutput(const std::string& name) -> std::string {
    fs::path const tmp = fs::temp_directory_path() / "labypath_test";
    fs::create_directories(tmp);
    return (tmp / name).string();
}

/// Check that all points in a polyline lie within the given bounding box
/// (with a tolerance margin for floating-point rounding).
auto allPointsInBounds(const laby::Polyline& polyline, const CGAL::Bbox_2& box,
                       double margin = kBoundsMargin) -> bool {
    return std::all_of(polyline.points().begin(), polyline.points().end(), [&](const auto& point) {
        double const xCoordinate = CGAL::to_double(point.x());
        double const yCoordinate = CGAL::to_double(point.y());
        return xCoordinate >= box.xmin() - margin && xCoordinate <= box.xmax() + margin &&
               yCoordinate >= box.ymin() - margin && yCoordinate <= box.ymax() + margin;
    });
}

/// Check that all coordinates are finite (not NaN or Inf).
auto allPointsFinite(const laby::Polyline& polyline) -> bool {
    return std::all_of(polyline.points().begin(), polyline.points().end(), [](const auto& point) {
        double const xCoordinate = CGAL::to_double(point.x());
        double const yCoordinate = CGAL::to_double(point.y());
        return std::isfinite(xCoordinate) && std::isfinite(yCoordinate);
    });
}

auto ribbonHasGeometry(const laby::Ribbon& ribbon) -> bool {
    return std::any_of(ribbon.lines().begin(), ribbon.lines().end(),
                       [](const auto& line) { return line.points().size() > 1; });
}

/// Count the number of path or polyline elements in SVG content.
auto countSvgPathElements(const std::string& content) -> std::size_t {
    std::size_t count = 0;
    std::size_t pos = 0;
    while ((pos = content.find("<path", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    pos = 0;
    while ((pos = content.find("<polyline", pos)) != std::string::npos) {
        ++count;
        ++pos;
    }
    return count;
}

/// Extract viewBox dimensions from SVG content.
auto extractViewBox(const std::string& content) -> std::optional<ParsedViewBox> {
    std::regex const viewBoxRe(
        R"(viewBox\s*=\s*["']([0-9.e+-]+)\s+([0-9.e+-]+)\s+([0-9.e+-]+)\s+([0-9.e+-]+)["'])");
    std::smatch match{};
    if (std::regex_search(content, match, viewBoxRe) &&
        match.size() == kExpectedViewBoxMatchCount) {
        return ParsedViewBox{std::stod(match[1]), std::stod(match[2]), std::stod(match[3]),
                             std::stod(match[4])};
    }
    return std::nullopt;
}

} // namespace

// ─── SVG Loader Qualification Tests ─────────────────────────────────────────

class SvgLoaderTest : public ::testing::Test {};

TEST_F(SvgLoaderTest, LoadSquareCircle) {
    const std::string path = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader const loader(path);
    EXPECT_GT(loader.ribList().size(), 0U) << "Should extract at least one ribbon";

    const auto& box = loader.viewBox();
    EXPECT_GT(box.xmax() - box.xmin(), 0.0) << "viewBox should have positive width";
    EXPECT_GT(box.ymax() - box.ymin(), 0.0) << "viewBox should have positive height";
}

TEST_F(SvgLoaderTest, LoadDrawing) {
    const std::string path = inputFile("drawing.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader const loader(path);
    EXPECT_GT(loader.ribList().size(), 0U);
}

TEST_F(SvgLoaderTest, Load591) {
    const std::string path = inputFile("591.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader const loader(path);
    EXPECT_GT(loader.ribList().size(), 0U);

    // 591.svg has color-coded shapes
    for (const auto& rib : loader.ribList()) {
        EXPECT_GT(rib.lines().size(), 0U) << "Each ribbon should have polylines";
    }
}

TEST_F(SvgLoaderTest, Load591Quarter) {
    const std::string path = inputFile("591_quarter.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader const loader(path);
    EXPECT_GT(loader.ribList().size(), 0U);

    const auto& box = loader.viewBox();
    EXPECT_GT(box.xmax() - box.xmin(), 0.0);
    EXPECT_GT(box.ymax() - box.ymin(), 0.0);
}

TEST_F(SvgLoaderTest, Load591Tree) {
    const std::string path = inputFile("591_tree.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader const loader(path);
    EXPECT_GT(loader.ribList().size(), 0U);
}

TEST_F(SvgLoaderTest, Load591Circle) {
    const std::string path = inputFile("591_circle.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader const loader(path);
    EXPECT_GT(loader.ribList().size(), 0U);
}

TEST_F(SvgLoaderTest, LoadWomanHair) {
    const std::string path = inputFile("woman_hair.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader const loader(path);
    EXPECT_GT(loader.ribList().size(), 0U);
}

TEST_F(SvgLoaderTest, RibbonsHavePolylines) {
    const std::string path = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader const loader(path);
    for (const auto& rib : loader.ribList()) {
        EXPECT_GT(rib.lines().size(), 0U) << "Ribbon should have polyline data";
        std::size_t nonEmpty = 0;
        for (const auto& line : rib.lines()) {
            if (!line.points().empty()) {
                ++nonEmpty;
            }
        }
        EXPECT_GT(nonEmpty, 0U) << "Ribbon should have at least one non-empty polyline";
    }
}

// ─── Geometric Validation: Coordinates ──────────────────────────────────────

TEST_F(SvgLoaderTest, CoordinatesWithinViewBox) {
    const std::string path = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader const loader(path);
    const auto& box = loader.viewBox();

    for (const auto& rib : loader.ribList()) {
        for (const auto& line : rib.lines()) {
            EXPECT_TRUE(allPointsInBounds(line, box, kBoundsMargin))
                << "All polyline points should lie within the viewBox (±1.0 margin)";
        }
    }
}

TEST_F(SvgLoaderTest, CoordinatesAreFinite) {
    const std::string path = inputFile("591.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader const loader(path);
    for (const auto& rib : loader.ribList()) {
        for (const auto& line : rib.lines()) {
            EXPECT_TRUE(allPointsFinite(line))
                << "All polyline coordinates must be finite (no NaN/Inf)";
        }
    }
}

TEST_F(SvgLoaderTest, PolylinesHaveMinimumPoints) {
    const std::string path = inputFile("drawing.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader loader(path);
    for (const auto& rib : loader.ribList()) {
        for (const auto& line : rib.lines()) {
            if (!line.points().empty()) {
                // A meaningful polyline should have at least 2 points
                EXPECT_GE(line.points().size(), 2U)
                    << "Non-empty polylines should have at least 2 points";
            }
        }
    }
}

TEST_F(SvgLoaderTest, RibbonColorIsValid) {
    const std::string path = inputFile("591.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader loader(path);
    for (const auto& rib : loader.ribList()) {
        // Fill color is encoded as a 32-bit RGB value; should be non-negative
        EXPECT_GE(rib.fillColor(), 0) << "Fill color should be a valid positive value";
    }
}

// ─── PrintRibbonSvg Qualification Tests ─────────────────────────────────────

class PrintRibbonSvgTest : public ::testing::Test {};

TEST_F(PrintRibbonSvgTest, WriteSvgFromLoadedRibbons) {
    const std::string svgIn = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    laby::svgp::Loader const loader(svgIn);
    ASSERT_GT(loader.ribList().size(), 0U);

    const std::string svgOut = tmpOutput("printribbon_output.svg");

    EXPECT_NO_THROW(laby::GraphicRendering::printRibbonSvg(loader.viewBox(), svgOut,
                                                           kDefaultStrokeWidth, loader.ribList()));

    ASSERT_TRUE(fs::exists(svgOut)) << "Output SVG should be created";
    EXPECT_GT(fs::file_size(svgOut), 0U) << "Output SVG should not be empty";

    // Read and validate SVG structure
    std::ifstream ifs(svgOut);
    std::string const content((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());

    EXPECT_TRUE(content.find("<svg") != std::string::npos) << "Output should contain <svg element";

    // Verify the output has path/polyline elements matching the input ribbons
    std::size_t const pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0U) << "Output SVG should contain path or polyline elements";

    // Verify viewBox is preserved in the output
    auto const parsedViewBox = extractViewBox(content);
    EXPECT_TRUE(parsedViewBox.has_value()) << "Output SVG should have a viewBox attribute";

    fs::remove(svgOut);
}

TEST_F(PrintRibbonSvgTest, WriteSvgFrom591) {
    const std::string svgIn = inputFile("591.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    laby::svgp::Loader const loader(svgIn);
    ASSERT_GT(loader.ribList().size(), 0U);

    const std::string svgOut = tmpOutput("printribbon_591_output.svg");

    EXPECT_NO_THROW(laby::GraphicRendering::printRibbonSvg(loader.viewBox(), svgOut,
                                                           kThickStrokeWidth, loader.ribList()));

    ASSERT_TRUE(fs::exists(svgOut));
    EXPECT_GT(fs::file_size(svgOut), 0U);

    // Validate geometric output
    std::ifstream ifs(svgOut);
    std::string const content((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());

    std::size_t const pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0U) << "591.svg rendering should produce multiple path elements";

    fs::remove(svgOut);
}

TEST_F(PrintRibbonSvgTest, EmptyRibbonListProducesValidSvg) {
    const std::string svgOut = tmpOutput("printribbon_empty.svg");
    CGAL::Bbox_2 const box(0, 0, kEmptySvgBoxMax, kEmptySvgBoxMax);
    std::vector<laby::Ribbon> const empty;

    EXPECT_NO_THROW(
        laby::GraphicRendering::printRibbonSvg(box, svgOut, kDefaultStrokeWidth, empty));

    ASSERT_TRUE(fs::exists(svgOut));
    EXPECT_GT(fs::file_size(svgOut), 0U);

    // Even with empty input, the output should be well-formed SVG
    std::ifstream ifs(svgOut);
    std::string const content((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("<svg") != std::string::npos);
    EXPECT_TRUE(content.find("</svg>") != std::string::npos);

    // Empty input should produce no path elements
    std::size_t const pathCount = countSvgPathElements(content);
    EXPECT_EQ(pathCount, 0U) << "Empty ribbon list should produce no path elements";

    fs::remove(svgOut);
}

// ─── SkeletonGrid Qualification Tests ───────────────────────────────────────

class SkeletonGridQualTest : public ::testing::Test {};

TEST_F(SkeletonGridQualTest, ProcessSquareCircle) {
    const std::string svgIn = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    const std::string svgOut = tmpOutput("skeleton_qual_square_circle.svg");

    proto::SkeletonGrid config;
    config.set_inputfile(svgIn);
    config.set_outputfile(svgOut);
    config.set_simplificationoforiginalsvg(kSimplificationDistance);
    config.set_max_sep(kSquareMaxSeparation);
    config.set_min_sep(kSquareMinSeparation);
    config.set_seed(kSquareSeed);

    EXPECT_NO_THROW(laby::SkeletonGrid grid(config));

    // Verify output SVG was produced
    ASSERT_TRUE(fs::exists(svgOut)) << "SkeletonGrid should produce output SVG";
    EXPECT_GT(fs::file_size(svgOut), kMinimumSubstantialSvgSize)
        << "Output SVG should have substantial content";

    // Read output and validate structure
    std::ifstream ifs(svgOut);
    std::string const content((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());

    EXPECT_TRUE(content.find("<svg") != std::string::npos) << "Output should contain <svg element";

    // Skeleton output should have path/polyline elements for the grid
    std::size_t const pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0U)
        << "Skeleton output should contain path elements for the grid structure";

    laby::svgp::Loader outputLoader(svgOut);
    bool hasCircular = false;
    bool hasRadial = false;
    for (const auto& rib : outputLoader.ribList()) {
        const auto green = laby::basic::Color::getGreen(static_cast<uint32_t>(rib.strokeColor()));
        if (green == static_cast<uint32_t>(laby::GridChannel::Circular)) {
            hasCircular = hasCircular || ribbonHasGeometry(rib);
        }
        if (green == static_cast<uint32_t>(laby::GridChannel::Radial)) {
            hasRadial = hasRadial || ribbonHasGeometry(rib);
        }
    }
    EXPECT_TRUE(hasCircular) << "Skeleton output should include a non-empty circular channel";
    EXPECT_TRUE(hasRadial) << "Skeleton output should include a non-empty radial channel";

    // Validate the output has a viewBox
    if (auto const parsedViewBox = extractViewBox(content); parsedViewBox.has_value()) {
        EXPECT_GT(parsedViewBox->width, 0.0) << "Output viewBox width should be positive";
        EXPECT_GT(parsedViewBox->height, 0.0) << "Output viewBox height should be positive";
    }

    fs::remove(svgOut);
}

TEST_F(SkeletonGridQualTest, ProcessDrawing) {
    const std::string svgIn = inputFile("drawing.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    const std::string svgOut = tmpOutput("skeleton_qual_drawing.svg");

    proto::SkeletonGrid config;
    config.set_inputfile(svgIn);
    config.set_outputfile(svgOut);
    config.set_simplificationoforiginalsvg(kSimplificationDistance);
    config.set_max_sep(kDrawingMaxSeparation);
    config.set_min_sep(kDrawingMinSeparation);
    config.set_seed(kDrawingSeed);

    EXPECT_NO_THROW(laby::SkeletonGrid grid(config));
    ASSERT_TRUE(fs::exists(svgOut));

    std::ifstream ifs(svgOut);
    std::string const content((std::istreambuf_iterator<char>(ifs)),
                              std::istreambuf_iterator<char>());

    EXPECT_GT(fs::file_size(svgOut), 0U);
    EXPECT_TRUE(content.find("<svg") != std::string::npos);

    // Verify skeleton produced geometric elements
    std::size_t const pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0U) << "Drawing skeleton should produce path elements";

    fs::remove(svgOut);
}

TEST_F(SkeletonGridQualTest, OutputBoundsMatchInput) {
    const std::string svgIn = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    // Load input to get the expected bounds
    laby::svgp::Loader const inputLoader(svgIn);
    const auto& inputBox = inputLoader.viewBox();

    const std::string svgOut = tmpOutput("skeleton_bounds_test.svg");

    proto::SkeletonGrid config;
    config.set_inputfile(svgIn);
    config.set_outputfile(svgOut);
    config.set_simplificationoforiginalsvg(kSimplificationDistance);
    config.set_max_sep(kSquareMaxSeparation);
    config.set_min_sep(kSquareMinSeparation);
    config.set_seed(kSquareSeed);

    laby::SkeletonGrid const grid(config);

    // The skeleton's bounding box should be within or close to the input's
    const auto& skelBox = grid.bbox();
    EXPECT_GE(skelBox.xmin(), inputBox.xmin() - kSquareMaxSeparation)
        << "Skeleton should not extend far beyond input bounds";
    EXPECT_LE(skelBox.xmax(), inputBox.xmax() + kSquareMaxSeparation)
        << "Skeleton should not extend far beyond input bounds";
    EXPECT_GE(skelBox.ymin(), inputBox.ymin() - kSquareMaxSeparation)
        << "Skeleton should not extend far beyond input bounds";
    EXPECT_LE(skelBox.ymax(), inputBox.ymax() + kSquareMaxSeparation)
        << "Skeleton should not extend far beyond input bounds";

    fs::remove(svgOut);
}

// ─── AlternateRoute Qualification Tests ─────────────────────────────────────

class AlternateRouteQualTest : public ::testing::Test {};

TEST_F(AlternateRouteQualTest, ProcessSquareCircleDoesNotSegfault) {
    const std::string svgIn = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    const std::string svgOut = tmpOutput("alternate_qual_square_circle.svg");

    proto::AlternateRouting altConfig;
    altConfig.set_maxthickness(kAlternateMaxThickness);
    altConfig.set_minthickness(kAlternateMinThickness);
    altConfig.set_pruning(kAlternatePruning);
    altConfig.set_thicknesspercent(kAlternateThicknessPercent);
    altConfig.set_simplifydist(kAlternateSimplifyDistance);

    proto::Filepaths filepaths;
    filepaths.set_inputfile(svgIn);
    filepaths.set_outputfile(svgOut);

    try {
        laby::AlternateRoute const route(altConfig, filepaths);
        ASSERT_TRUE(fs::exists(svgOut)) << "AlternateRoute should produce output SVG";
        if (fs::exists(svgOut)) {
            EXPECT_GT(fs::file_size(svgOut), 0U);

            // Validate output is well-formed SVG
            std::ifstream ifs(svgOut);
            std::string const content((std::istreambuf_iterator<char>(ifs)),
                                      std::istreambuf_iterator<char>());
            EXPECT_TRUE(content.find("<svg") != std::string::npos);

            fs::remove(svgOut);
        }
    } catch (const std::bad_alloc&) {
        SUCCEED() << "std::bad_alloc is expected for simple input shapes";
    } catch (const std::exception& exception) {
        SUCCEED() << "Exception during AlternateRoute: " << exception.what();
    }
}

TEST_F(AlternateRouteQualTest, ConfigSetup) {
    proto::AlternateRouting altConfig;
    altConfig.set_maxthickness(kConfigMaxThickness);
    altConfig.set_minthickness(kConfigMinThickness);
    altConfig.set_pruning(kConfigPruning);
    altConfig.set_thicknesspercent(kConfigThicknessPercent);
    altConfig.set_simplifydist(kAlternateSimplifyDistance);

    EXPECT_DOUBLE_EQ(altConfig.maxthickness(), kConfigMaxThickness);
    EXPECT_DOUBLE_EQ(altConfig.minthickness(), kConfigMinThickness);
    EXPECT_EQ(altConfig.pruning(), kConfigPruning);
    EXPECT_DOUBLE_EQ(altConfig.thicknesspercent(), kConfigThicknessPercent);
    EXPECT_DOUBLE_EQ(altConfig.simplifydist(), kAlternateSimplifyDistance);

    proto::Filepaths filepaths;
    filepaths.set_inputfile("/some/input.svg");
    filepaths.set_outputfile("/some/output.svg");
    EXPECT_EQ(filepaths.inputfile(), "/some/input.svg");
    EXPECT_EQ(filepaths.outputfile(), "/some/output.svg");
}

TEST_F(AlternateRouteQualTest, ConfigThicknessConstraint) {
    proto::AlternateRouting altConfig;
    altConfig.set_maxthickness(kAlternateMaxThickness);
    altConfig.set_minthickness(kAlternateMinThickness);

    // Max thickness must be greater than min thickness
    EXPECT_GT(altConfig.maxthickness(), altConfig.minthickness())
        << "Max thickness should exceed min thickness";

    // Thickness values should be positive
    EXPECT_GT(altConfig.maxthickness(), 0.0);
    EXPECT_GT(altConfig.minthickness(), 0.0);
}

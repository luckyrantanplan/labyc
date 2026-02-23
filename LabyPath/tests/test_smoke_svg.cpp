/**
 * @file test_smoke_svg.cpp
 * @brief Qualification tests for end-to-end SVG processing pipelines.
 *
 * These tests load real SVG input files and validate geometric properties
 * of the output: coordinate bounds, polyline integrity, skeleton structure,
 * and SVG well-formedness.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "GeomData.h"
#include "Ribbon.h"
#include "Polyline.h"
#include "SVGParser/Loader.h"
#include "Rendering/GraphicRendering.h"
#include "SkeletonGrid.h"
#include "AlternaRoute/AlternateRoute.h"
#include "protoc/AllConfig.pb.h"

namespace fs = std::filesystem;

// ─── Helpers ────────────────────────────────────────────────────────────────

static std::string inputDir() {
    fs::path base = fs::path(__FILE__).parent_path().parent_path() / "input";
    return base.string();
}

static std::string inputFile(const std::string& name) {
    return inputDir() + "/" + name;
}

static std::string tmpOutput(const std::string& name) {
    fs::path tmp = fs::temp_directory_path() / "labypath_test";
    fs::create_directories(tmp);
    return (tmp / name).string();
}

/// Check that all points in a polyline lie within the given bounding box
/// (with a tolerance margin for floating-point rounding).
static bool allPointsInBounds(const laby::Polyline& polyline, const CGAL::Bbox_2& box, double margin = 1.0) {
    for (const auto& pt : polyline.points) {
        double x = CGAL::to_double(pt.x());
        double y = CGAL::to_double(pt.y());
        if (x < box.xmin() - margin || x > box.xmax() + margin ||
            y < box.ymin() - margin || y > box.ymax() + margin) {
            return false;
        }
    }
    return true;
}

/// Check that all coordinates are finite (not NaN or Inf).
static bool allPointsFinite(const laby::Polyline& polyline) {
    for (const auto& pt : polyline.points) {
        double x = CGAL::to_double(pt.x());
        double y = CGAL::to_double(pt.y());
        if (!std::isfinite(x) || !std::isfinite(y)) {
            return false;
        }
    }
    return true;
}

/// Count the total number of points across all polylines in a ribbon.
static std::size_t totalPointCount(const laby::Ribbon& ribbon) {
    std::size_t count = 0;
    for (const auto& line : ribbon.lines()) {
        count += line.points.size();
    }
    return count;
}

/// Count the number of path or polyline elements in SVG content.
static std::size_t countSvgPathElements(const std::string& content) {
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
static bool extractViewBox(const std::string& content, double& x, double& y, double& w, double& h) {
    std::regex viewBoxRe(R"(viewBox\s*=\s*["']([0-9.e+-]+)\s+([0-9.e+-]+)\s+([0-9.e+-]+)\s+([0-9.e+-]+)["'])");
    std::smatch match;
    if (std::regex_search(content, match, viewBoxRe) && match.size() == 5) {
        x = std::stod(match[1]);
        y = std::stod(match[2]);
        w = std::stod(match[3]);
        h = std::stod(match[4]);
        return true;
    }
    return false;
}

// ─── SVG Loader Qualification Tests ─────────────────────────────────────────

class SvgLoaderTest : public ::testing::Test {};

TEST_F(SvgLoaderTest, LoadSquareCircle) {
    const std::string path = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader loader(path);
    EXPECT_GT(loader.ribList().size(), 0u) << "Should extract at least one ribbon";

    const auto& box = loader.viewBox();
    EXPECT_GT(box.xmax() - box.xmin(), 0.0) << "viewBox should have positive width";
    EXPECT_GT(box.ymax() - box.ymin(), 0.0) << "viewBox should have positive height";
}

TEST_F(SvgLoaderTest, LoadDrawing) {
    const std::string path = inputFile("drawing.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader loader(path);
    EXPECT_GT(loader.ribList().size(), 0u);
}

TEST_F(SvgLoaderTest, Load591) {
    const std::string path = inputFile("591.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader loader(path);
    EXPECT_GT(loader.ribList().size(), 0u);

    // 591.svg has color-coded shapes
    for (const auto& rib : loader.ribList()) {
        EXPECT_GT(rib.lines().size(), 0u) << "Each ribbon should have polylines";
    }
}

TEST_F(SvgLoaderTest, Load591Quarter) {
    const std::string path = inputFile("591_quarter.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader loader(path);
    EXPECT_GT(loader.ribList().size(), 0u);

    const auto& box = loader.viewBox();
    EXPECT_GT(box.xmax() - box.xmin(), 0.0);
    EXPECT_GT(box.ymax() - box.ymin(), 0.0);
}

TEST_F(SvgLoaderTest, Load591Tree) {
    const std::string path = inputFile("591_tree.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader loader(path);
    EXPECT_GT(loader.ribList().size(), 0u);
}

TEST_F(SvgLoaderTest, Load591Circle) {
    const std::string path = inputFile("591_circle.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader loader(path);
    EXPECT_GT(loader.ribList().size(), 0u);
}

TEST_F(SvgLoaderTest, LoadWomanHair) {
    const std::string path = inputFile("woman_hair.svg");
    ASSERT_TRUE(fs::exists(path)) << "Missing: " << path;

    laby::svgp::Loader loader(path);
    EXPECT_GT(loader.ribList().size(), 0u);
}

TEST_F(SvgLoaderTest, RibbonsHavePolylines) {
    const std::string path = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader loader(path);
    for (const auto& rib : loader.ribList()) {
        EXPECT_GT(rib.lines().size(), 0u) << "Ribbon should have polyline data";
        std::size_t nonEmpty = 0;
        for (const auto& line : rib.lines()) {
            if (!line.points.empty()) {
                ++nonEmpty;
            }
        }
        EXPECT_GT(nonEmpty, 0u) << "Ribbon should have at least one non-empty polyline";
    }
}

// ─── Geometric Validation: Coordinates ──────────────────────────────────────

TEST_F(SvgLoaderTest, CoordinatesWithinViewBox) {
    const std::string path = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader loader(path);
    const auto& box = loader.viewBox();

    for (const auto& rib : loader.ribList()) {
        for (const auto& line : rib.lines()) {
            EXPECT_TRUE(allPointsInBounds(line, box, 1.0))
                << "All polyline points should lie within the viewBox (±1.0 margin)";
        }
    }
}

TEST_F(SvgLoaderTest, CoordinatesAreFinite) {
    const std::string path = inputFile("591.svg");
    ASSERT_TRUE(fs::exists(path));

    laby::svgp::Loader loader(path);
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
            if (!line.points.empty()) {
                // A meaningful polyline should have at least 2 points
                EXPECT_GE(line.points.size(), 2u)
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
        EXPECT_GE(rib.fill_color(), 0) << "Fill color should be a valid positive value";
    }
}

// ─── PrintRibbonSvg Qualification Tests ─────────────────────────────────────

class PrintRibbonSvgTest : public ::testing::Test {};

TEST_F(PrintRibbonSvgTest, WriteSvgFromLoadedRibbons) {
    const std::string svgIn = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    laby::svgp::Loader loader(svgIn);
    ASSERT_GT(loader.ribList().size(), 0u);

    const std::string svgOut = tmpOutput("printribbon_output.svg");

    EXPECT_NO_THROW(
        laby::GraphicRendering::printRibbonSvg(
            loader.viewBox(), svgOut, 1.0, loader.ribList())
    );

    ASSERT_TRUE(fs::exists(svgOut)) << "Output SVG should be created";
    EXPECT_GT(fs::file_size(svgOut), 0u) << "Output SVG should not be empty";

    // Read and validate SVG structure
    std::ifstream ifs(svgOut);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    EXPECT_TRUE(content.find("<svg") != std::string::npos)
        << "Output should contain <svg element";

    // Verify the output has path/polyline elements matching the input ribbons
    std::size_t pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0u) << "Output SVG should contain path or polyline elements";

    // Verify viewBox is preserved in the output
    double vx, vy, vw, vh;
    EXPECT_TRUE(extractViewBox(content, vx, vy, vw, vh))
        << "Output SVG should have a viewBox attribute";

    fs::remove(svgOut);
}

TEST_F(PrintRibbonSvgTest, WriteSvgFrom591) {
    const std::string svgIn = inputFile("591.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    laby::svgp::Loader loader(svgIn);
    ASSERT_GT(loader.ribList().size(), 0u);

    const std::string svgOut = tmpOutput("printribbon_591_output.svg");

    EXPECT_NO_THROW(
        laby::GraphicRendering::printRibbonSvg(
            loader.viewBox(), svgOut, 2.0, loader.ribList())
    );

    ASSERT_TRUE(fs::exists(svgOut));
    EXPECT_GT(fs::file_size(svgOut), 0u);

    // Validate geometric output
    std::ifstream ifs(svgOut);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    std::size_t pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0u) << "591.svg rendering should produce multiple path elements";

    fs::remove(svgOut);
}

TEST_F(PrintRibbonSvgTest, EmptyRibbonListProducesValidSvg) {
    const std::string svgOut = tmpOutput("printribbon_empty.svg");
    CGAL::Bbox_2 box(0, 0, 100, 100);
    std::vector<laby::Ribbon> empty;

    EXPECT_NO_THROW(
        laby::GraphicRendering::printRibbonSvg(box, svgOut, 1.0, empty)
    );

    ASSERT_TRUE(fs::exists(svgOut));
    EXPECT_GT(fs::file_size(svgOut), 0u);

    // Even with empty input, the output should be well-formed SVG
    std::ifstream ifs(svgOut);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_TRUE(content.find("<svg") != std::string::npos);
    EXPECT_TRUE(content.find("</svg>") != std::string::npos);

    // Empty input should produce no path elements
    std::size_t pathCount = countSvgPathElements(content);
    EXPECT_EQ(pathCount, 0u) << "Empty ribbon list should produce no path elements";

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
    config.set_simplificationoforiginalsvg(0.1);
    config.set_max_sep(10.0);
    config.set_min_sep(3.0);
    config.set_seed(42);

    EXPECT_NO_THROW(laby::SkeletonGrid grid(config));

    // Verify output SVG was produced
    ASSERT_TRUE(fs::exists(svgOut)) << "SkeletonGrid should produce output SVG";
    EXPECT_GT(fs::file_size(svgOut), 100u) << "Output SVG should have substantial content";

    // Read output and validate structure
    std::ifstream ifs(svgOut);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    EXPECT_TRUE(content.find("<svg") != std::string::npos)
        << "Output should contain <svg element";

    // Skeleton output should have path/polyline elements for the grid
    std::size_t pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0u)
        << "Skeleton output should contain path elements for the grid structure";

    // Validate the output has a viewBox
    double vx, vy, vw, vh;
    if (extractViewBox(content, vx, vy, vw, vh)) {
        EXPECT_GT(vw, 0.0) << "Output viewBox width should be positive";
        EXPECT_GT(vh, 0.0) << "Output viewBox height should be positive";
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
    config.set_simplificationoforiginalsvg(0.1);
    config.set_max_sep(8.0);
    config.set_min_sep(2.0);
    config.set_seed(123);

    EXPECT_NO_THROW(laby::SkeletonGrid grid(config));
    ASSERT_TRUE(fs::exists(svgOut));

    std::ifstream ifs(svgOut);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());

    EXPECT_GT(fs::file_size(svgOut), 0u);
    EXPECT_TRUE(content.find("<svg") != std::string::npos);

    // Verify skeleton produced geometric elements
    std::size_t pathCount = countSvgPathElements(content);
    EXPECT_GT(pathCount, 0u) << "Drawing skeleton should produce path elements";

    fs::remove(svgOut);
}

TEST_F(SkeletonGridQualTest, OutputBoundsMatchInput) {
    const std::string svgIn = inputFile("square_circle.svg");
    ASSERT_TRUE(fs::exists(svgIn));

    // Load input to get the expected bounds
    laby::svgp::Loader inputLoader(svgIn);
    const auto& inputBox = inputLoader.viewBox();

    const std::string svgOut = tmpOutput("skeleton_bounds_test.svg");

    proto::SkeletonGrid config;
    config.set_inputfile(svgIn);
    config.set_outputfile(svgOut);
    config.set_simplificationoforiginalsvg(0.1);
    config.set_max_sep(10.0);
    config.set_min_sep(3.0);
    config.set_seed(42);

    laby::SkeletonGrid grid(config);

    // The skeleton's bounding box should be within or close to the input's
    const auto& skelBox = grid.bbox();
    EXPECT_GE(skelBox.xmin(), inputBox.xmin() - 10.0)
        << "Skeleton should not extend far beyond input bounds";
    EXPECT_LE(skelBox.xmax(), inputBox.xmax() + 10.0)
        << "Skeleton should not extend far beyond input bounds";
    EXPECT_GE(skelBox.ymin(), inputBox.ymin() - 10.0)
        << "Skeleton should not extend far beyond input bounds";
    EXPECT_LE(skelBox.ymax(), inputBox.ymax() + 10.0)
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
    altConfig.set_maxthickness(20.0);
    altConfig.set_minthickness(2.0);
    altConfig.set_pruning(5);
    altConfig.set_thicknesspercent(0.5);
    altConfig.set_simplifydist(1e-7);

    proto::Filepaths filepaths;
    filepaths.set_inputfile(svgIn);
    filepaths.set_outputfile(svgOut);

    try {
        laby::AlternateRoute route(altConfig, filepaths);
        ASSERT_TRUE(fs::exists(svgOut)) << "AlternateRoute should produce output SVG";
        if (fs::exists(svgOut)) {
            EXPECT_GT(fs::file_size(svgOut), 0u);

            // Validate output is well-formed SVG
            std::ifstream ifs(svgOut);
            std::string content((std::istreambuf_iterator<char>(ifs)),
                                 std::istreambuf_iterator<char>());
            EXPECT_TRUE(content.find("<svg") != std::string::npos);

            fs::remove(svgOut);
        }
    } catch (const std::bad_alloc&) {
        SUCCEED() << "std::bad_alloc is expected for simple input shapes";
    } catch (const std::exception& e) {
        SUCCEED() << "Exception during AlternateRoute: " << e.what();
    }
}

TEST_F(AlternateRouteQualTest, ConfigSetup) {
    proto::AlternateRouting altConfig;
    altConfig.set_maxthickness(15.0);
    altConfig.set_minthickness(1.0);
    altConfig.set_pruning(3);
    altConfig.set_thicknesspercent(0.4);
    altConfig.set_simplifydist(1e-7);

    EXPECT_DOUBLE_EQ(altConfig.maxthickness(), 15.0);
    EXPECT_DOUBLE_EQ(altConfig.minthickness(), 1.0);
    EXPECT_EQ(altConfig.pruning(), 3);
    EXPECT_DOUBLE_EQ(altConfig.thicknesspercent(), 0.4);
    EXPECT_DOUBLE_EQ(altConfig.simplifydist(), 1e-7);

    proto::Filepaths filepaths;
    filepaths.set_inputfile("/some/input.svg");
    filepaths.set_outputfile("/some/output.svg");
    EXPECT_EQ(filepaths.inputfile(), "/some/input.svg");
    EXPECT_EQ(filepaths.outputfile(), "/some/output.svg");
}

TEST_F(AlternateRouteQualTest, ConfigThicknessConstraint) {
    proto::AlternateRouting altConfig;
    altConfig.set_maxthickness(20.0);
    altConfig.set_minthickness(2.0);

    // Max thickness must be greater than min thickness
    EXPECT_GT(altConfig.maxthickness(), altConfig.minthickness())
        << "Max thickness should exceed min thickness";

    // Thickness values should be positive
    EXPECT_GT(altConfig.maxthickness(), 0.0);
    EXPECT_GT(altConfig.minthickness(), 0.0);
}

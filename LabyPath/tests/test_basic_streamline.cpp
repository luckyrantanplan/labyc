/**
 * @file test_basic_streamline.cpp
 * @brief Smoke tests for the HqNoise-to-StreamLine field-driven pipeline.
 */

#include <gtest/gtest.h>

#include <chrono>
#include <complex>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "MessageIO.h"
#include "Rendering/GraphicRendering.h"
#include "generator/ComplexField2D.h"
#include "generator/HqNoise2DSampler.h"
#include "generator/StreamLine.h"
#include "protoc/AllConfig.pb.h"

namespace fs = std::filesystem;

namespace {

auto makeTempDir() -> fs::path {
    const auto uniqueValue =
        std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const fs::path tempDir =
        fs::temp_directory_path() / ("labypath-basic-streamline-" + uniqueValue);
    fs::create_directories(tempDir);
    return tempDir;
}

auto readTextFile(const fs::path& path) -> std::string {
    std::ifstream input(path);
    EXPECT_TRUE(input.is_open());
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}

auto createNoiseConfig(const fs::path& fieldPath, const fs::path& previewPath, uint32_t width = 32,
                       uint32_t height = 32) -> proto::HqNoise {
    proto::HqNoise config;
    config.mutable_filepaths()->set_outputfile(fieldPath.string());
    config.set_maxn(16);
    config.set_accuracy(8);
    config.set_amplitude(1.0);
    config.set_seed(7.0);
    config.set_gaussianfrequency(2.5);
    config.set_powerlawfrequency(1.2);
    config.set_powerlawpower(2.0);
    config.set_complex(true);
    config.set_width(width);
    config.set_height(height);
    config.set_scale(0.25);
    config.set_previewmode(proto::HqNoise_PreviewMode_ARROWS);
    config.set_previewfile(previewPath.string());
    config.set_previewstride(2);
    return config;
}

} // namespace

TEST(BasicStreamLineTest, DirectFieldSamplingRendersPreviewAndStreamLines) {
    const fs::path tempDir = makeTempDir();
    const fs::path fieldPath = tempDir / "noise.field";
    const fs::path previewPath = tempDir / "preview.svg";
    const fs::path streamPath = tempDir / "stream.svg";

    const proto::HqNoise noiseConfig = createNoiseConfig(fieldPath, previewPath);
    const laby::generator::ComplexField2D field =
        laby::generator::HqNoise2DSampler::sample(noiseConfig);

    ASSERT_EQ(field.meta.width, 32U);
    ASSERT_EQ(field.meta.height, 32U);
    EXPECT_GT(std::abs(field.values[0][0]), 0.0);

    laby::generator::writeComplexField(fieldPath.string(), field);
    const laby::generator::ComplexField2D restoredField =
        laby::generator::readComplexField(fieldPath.string());
    EXPECT_EQ(restoredField.meta.width, field.meta.width);
    EXPECT_EQ(restoredField.meta.height, field.meta.height);
    EXPECT_NEAR(restoredField.values[5][7].real(), field.values[5][7].real(), 1e-9);
    EXPECT_NEAR(restoredField.values[5][7].imag(), field.values[5][7].imag(), 1e-9);

    laby::generator::HqNoise2DSampler::writePreviewSvg(previewPath.string(), restoredField,
                                                       noiseConfig);
    ASSERT_TRUE(fs::exists(previewPath));
    const std::string previewSvg = readTextFile(previewPath);
    EXPECT_NE(previewSvg.find("<svg"), std::string::npos);
    EXPECT_NE(previewSvg.find("<line"), std::string::npos);

    laby::generator::StreamLine::Config streamConfig{};
    streamConfig.resolution = 0;
    streamConfig.simplify_distance = 0.05;
    streamConfig.dRat = 1.0;
    streamConfig.epsilon = 0.01;
    streamConfig.size = static_cast<double>(restoredField.meta.width) * restoredField.meta.scale;
    streamConfig.divisor = 0.45;
    streamConfig.sample_scale = restoredField.meta.scale;
    streamConfig.old_RegularGrid = true;

    laby::generator::StreamLine streamLine(streamConfig, restoredField.values);
    streamLine.render();

    EXPECT_FALSE(streamLine.circularList().lines().empty());
    EXPECT_FALSE(streamLine.radialList().lines().empty());

    laby::GraphicRendering::printRibbonSvg(
        CGAL::Bbox_2(0.0, 0.0,
                     static_cast<double>(restoredField.meta.width) * restoredField.meta.scale,
                     static_cast<double>(restoredField.meta.height) * restoredField.meta.scale),
        streamPath.string(), 0.1, streamLine.ribbons());

    ASSERT_TRUE(fs::exists(streamPath));
    const std::string streamSvg = readTextFile(streamPath);
    EXPECT_NE(streamSvg.find("<svg"), std::string::npos);
    EXPECT_NE(streamSvg.find("<path"), std::string::npos);
}

TEST(BasicStreamLineTest, MessageIORunsHqNoiseAndStreamLineStagesSequentially) {
    const fs::path tempDir = makeTempDir();
    const fs::path fieldPath = tempDir / "pipeline.field";
    const fs::path previewPath = tempDir / "pipeline-preview.svg";
    const fs::path streamPath = tempDir / "pipeline-stream.svg";
    const fs::path configPath = tempDir / "pipeline.json";

    const std::string configJson =
        std::string("{\n") + "  \"hqNoise\": {\n" + "    \"filepaths\": { \"outputfile\": \"" +
        fieldPath.string() + "\" },\n" + "    \"maxN\": 16,\n" + "    \"accuracy\": 8,\n" +
        "    \"amplitude\": 1.0,\n" + "    \"seed\": 7.0,\n" + "    \"gaussianFrequency\": 2.5,\n" +
        "    \"powerlawFrequency\": 1.2,\n" + "    \"powerlawPower\": 2.0,\n" +
        "    \"complex\": true,\n" + "    \"width\": 32,\n" + "    \"height\": 32,\n" +
        "    \"scale\": 0.25,\n" + "    \"previewMode\": \"ARROWS\",\n" +
        "    \"previewFile\": \"" + previewPath.string() + "\",\n" + "    \"previewStride\": 2\n" +
        "  },\n" + "  \"streamLine\": {\n" + "    \"filepaths\": { \"inputfile\": \"" +
        fieldPath.string() + "\", \"outputfile\": \"" + streamPath.string() + "\" },\n" +
        "    \"resolution\": 0,\n" + "    \"simplifyDistance\": 0.05,\n" + "    \"dRat\": 1.0,\n" +
        "    \"epsilon\": 0.01,\n" + "    \"size\": 8.0,\n" + "    \"divisor\": 0.45,\n" +
        "    \"strokeThickness\": 0.1\n" + "  }\n" + "}\n";

    {
        std::ofstream output(configPath);
        ASSERT_TRUE(output.is_open());
        output << configJson;
    }

    const std::vector<std::string> storage = {"labypath", configPath.string()};
    std::vector<std::string_view> arguments;
    arguments.reserve(storage.size());
    for (const std::string& argument : storage) {
        arguments.emplace_back(argument);
    }

    EXPECT_EQ(laby::MessageIO::parseMessage(arguments), 0);
    ASSERT_TRUE(fs::exists(fieldPath));
    ASSERT_TRUE(fs::exists(previewPath));
    ASSERT_TRUE(fs::exists(streamPath));

    const std::string previewSvg = readTextFile(previewPath);
    const std::string streamSvg = readTextFile(streamPath);
    EXPECT_NE(previewSvg.find("<line"), std::string::npos);
    EXPECT_NE(streamSvg.find("<path"), std::string::npos);
}

TEST(BasicStreamLineTest, StreamLineAcceptsLargerFieldThanInitialSizeGuess) {
    const fs::path tempDir = makeTempDir();
    const fs::path fieldPath = tempDir / "large-noise.field";
    const fs::path previewPath = tempDir / "large-preview.svg";

    const proto::HqNoise noiseConfig = createNoiseConfig(fieldPath, previewPath, 64, 64);
    const laby::generator::ComplexField2D field =
        laby::generator::HqNoise2DSampler::sample(noiseConfig);

    ASSERT_EQ(field.meta.width, 64U);
    ASSERT_EQ(field.meta.height, 64U);

    laby::generator::StreamLine::Config streamConfig{};
    streamConfig.resolution = 0;
    streamConfig.simplify_distance = 0.05;
    streamConfig.dRat = 1.0;
    streamConfig.epsilon = 0.01;
    streamConfig.size = 8.0;
    streamConfig.divisor = 0.45;
    streamConfig.sample_scale = field.meta.scale;
    streamConfig.old_RegularGrid = true;

    laby::generator::StreamLine streamLine(streamConfig, field.values);
    EXPECT_NO_THROW(streamLine.render());
    EXPECT_FALSE(streamLine.circularList().lines().empty());
    EXPECT_FALSE(streamLine.radialList().lines().empty());
}
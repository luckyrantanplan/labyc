#include "HqNoise2DSampler.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <stdexcept>

#include "../SVGWriter/DocumentSVG.h"

namespace laby::generator {

namespace {

constexpr double kDefaultPreviewScale = 1.0;
constexpr double kArrowCoverageRatio = 0.45;
constexpr double kMinimumStrokeWidth = 0.05;
constexpr double kMaximumColorValue = 255.0;

auto buildNoiseConfig(const proto::HqNoise& config) -> HqNoiseConfig {
    if (config.maxn() == 0 || config.accuracy() == 0 || config.width() == 0 ||
        config.height() == 0) {
        throw std::runtime_error("hqNoise requires positive maxN, accuracy, width, and height");
    }
    if (config.scale() <= 0.0) {
        throw std::runtime_error("hqNoise scale must be greater than zero");
    }

    const double maxX = static_cast<double>(config.width() - 1) * config.scale();
    const double maxY = static_cast<double>(config.height() - 1) * config.scale();
    if (maxX > static_cast<double>(config.maxn()) || maxY > static_cast<double>(config.maxn())) {
        throw std::runtime_error("hqNoise sampling domain exceeds maxN support");
    }

    HqNoiseConfig noiseConfig{};
    noiseConfig.maxN = config.maxn();
    noiseConfig.accuracy = config.accuracy();
    noiseConfig.amplitude = config.amplitude();
    noiseConfig.seed = config.seed();
    noiseConfig.gaussian.frequency = config.gaussianfrequency();
    noiseConfig.powerlaw.frequency = config.powerlawfrequency();
    noiseConfig.powerlaw.power = config.powerlawpower();
    noiseConfig.complex = config.complex();
    return noiseConfig;
}

auto mixColor(const svg::Color::Rgb& startColor, const svg::Color::Rgb& endColor,
              double position) -> svg::Color::Rgb {
    const double clampedPosition = std::clamp(position, 0.0, 1.0);
    const auto lerp = [clampedPosition](int32_t startValue, int32_t endValue) {
        return static_cast<int32_t>(std::lround(static_cast<double>(startValue) +
                                                clampedPosition * static_cast<double>(endValue - startValue)));
    };
    return {lerp(startColor.red, endColor.red), lerp(startColor.green, endColor.green),
            lerp(startColor.blue, endColor.blue)};
}

auto rainbowColor(double normalizedMagnitude) -> svg::Color {
    const double t = std::clamp(normalizedMagnitude, 0.0, 1.0);
    if (t < (1.0 / 3.0)) {
        return svg::Color(mixColor({0, 0, static_cast<int32_t>(kMaximumColorValue)},
                                   {0, static_cast<int32_t>(kMaximumColorValue), 0}, t * 3.0));
    }
    if (t < (2.0 / 3.0)) {
        return svg::Color(mixColor({0, static_cast<int32_t>(kMaximumColorValue), 0},
                                   {static_cast<int32_t>(kMaximumColorValue),
                                    static_cast<int32_t>(kMaximumColorValue), 0},
                                   (t - (1.0 / 3.0)) * 3.0));
    }
    return svg::Color(mixColor({static_cast<int32_t>(kMaximumColorValue),
                                static_cast<int32_t>(kMaximumColorValue), 0},
                               {static_cast<int32_t>(kMaximumColorValue), 0, 0},
                               (t - (2.0 / 3.0)) * 3.0));
}

auto resolveStride(const proto::HqNoise& config, const ComplexField2D& field) -> uint32_t {
    if (config.previewstride() > 0) {
        return config.previewstride();
    }
    return std::max<uint32_t>(1, std::max(field.meta.width, field.meta.height) / 32U);
}

auto maxMagnitude(const ComplexField2D& field) -> double {
    double maxValue = 0.0;
    for (uint32_t xIndex = 0; xIndex < field.meta.width; ++xIndex) {
        for (uint32_t yIndex = 0; yIndex < field.meta.height; ++yIndex) {
            maxValue = std::max(maxValue, std::abs(field.values[xIndex][yIndex]));
        }
    }
    return maxValue;
}

} // namespace

auto HqNoise2DSampler::sample(const proto::HqNoise& config) -> ComplexField2D {
    const HqNoiseConfig noiseConfig = buildNoiseConfig(config);
    const HqNoise2D noise(noiseConfig);

    ComplexField2D field;
    field.meta.width = config.width();
    field.meta.height = config.height();
    field.meta.originX = 0.0;
    field.meta.originY = 0.0;
    field.meta.scale = config.scale();

    using FieldIndex = boost::multi_array<std::complex<double>, 2>::index;
    field.values.resize(boost::extents[static_cast<FieldIndex>(field.meta.width)]
                                     [static_cast<FieldIndex>(field.meta.height)]);

    for (uint32_t xIndex = 0; xIndex < field.meta.width; ++xIndex) {
        for (uint32_t yIndex = 0; yIndex < field.meta.height; ++yIndex) {
            const double xCoordinate = field.meta.originX + static_cast<double>(xIndex) * field.meta.scale;
            const double yCoordinate = field.meta.originY + static_cast<double>(yIndex) * field.meta.scale;
            field.values[xIndex][yIndex] = noise.getComplex(xCoordinate, yCoordinate);
        }
    }

    return field;
}

auto HqNoise2DSampler::writePreviewSvg(const std::string& path, const ComplexField2D& field,
                                       const proto::HqNoise& config) -> void {
    if (path.empty() || config.previewmode() == proto::HqNoise_PreviewMode_NONE) {
        return;
    }

    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    const svg::Dimensions dimensions(
        svg::Dimensions::Size{static_cast<double>(field.meta.width) * field.meta.scale,
                              static_cast<double>(field.meta.height) * field.meta.scale});
    svg::DocumentSVG document(path, svg::Layout(dimensions, svg::Layout::Origin::TopLeft,
                                                kDefaultPreviewScale));

    const uint32_t stride = resolveStride(config, field);
    const double strideScale = static_cast<double>(stride) * field.meta.scale;
    const double magnitudeUpperBound = std::max(maxMagnitude(field), std::numeric_limits<double>::epsilon());

    for (uint32_t xIndex = 0; xIndex < field.meta.width; xIndex += stride) {
        for (uint32_t yIndex = 0; yIndex < field.meta.height; yIndex += stride) {
            const std::complex<double> value = field.values[xIndex][yIndex];
            const double magnitude = std::abs(value);
            const double normalizedMagnitude = magnitude / magnitudeUpperBound;
            const svg::Color color = rainbowColor(normalizedMagnitude);
            const double centerX = field.meta.originX + (static_cast<double>(xIndex) + 0.5) * field.meta.scale;
            const double centerY = field.meta.originY + (static_cast<double>(yIndex) + 0.5) * field.meta.scale;

            if (config.previewmode() == proto::HqNoise_PreviewMode_MAGNITUDE) {
                const double cellWidth = std::min(strideScale,
                                                  static_cast<double>(field.meta.width - xIndex) * field.meta.scale);
                const double cellHeight = std::min(strideScale,
                                                   static_cast<double>(field.meta.height - yIndex) * field.meta.scale);
                document << svg::Rectangle(laby::Point_2(centerX - cellWidth * 0.5,
                                                         centerY - cellHeight * 0.5),
                                           cellWidth, cellHeight, svg::Fill(color));
                continue;
            }

            const double vectorLength = normalizedMagnitude * strideScale * kArrowCoverageRatio;
            const std::complex<double> direction =
                magnitude > 0.0 ? value / magnitude : std::complex<double>(1.0, 0.0);
            const std::complex<double> offset = direction * (vectorLength * 0.5);
            const laby::Point_2 startPoint(centerX - offset.real(), centerY - offset.imag());
            const laby::Point_2 endPoint(centerX + offset.real(), centerY + offset.imag());
            document << svg::Line(startPoint, endPoint,
                                  svg::Stroke(std::max(field.meta.scale * 0.15, kMinimumStrokeWidth), color));
        }
    }

    static_cast<void>(document.save());
}

} // namespace laby::generator
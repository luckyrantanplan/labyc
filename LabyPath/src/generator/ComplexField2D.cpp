#include "ComplexField2D.h"

#include <boost/multi_array/extent_gen.hpp>
#include <boost/multi_array.hpp>
#include <cstddef>
#include <complex>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <ios>
#include <stdexcept>
#include <string>

#include "../protoc/AllConfig.pb.h"

namespace laby::generator {

namespace {

constexpr int kComplexComponentCount = 2;

auto validateMeta(const ComplexField2DMeta& meta) -> void {
    if (meta.width == 0 || meta.height == 0) {
        throw std::runtime_error("complex field dimensions must be greater than zero");
    }
    if (meta.scale <= 0.0) {
        throw std::runtime_error("complex field scale must be greater than zero");
    }
}

auto validateValueCount(std::size_t actualCount, const ComplexField2DMeta& meta) -> void {
    const auto expectedCount =
        static_cast<std::size_t>(meta.width) * static_cast<std::size_t>(meta.height) *
        static_cast<std::size_t>(kComplexComponentCount);
    if (actualCount != expectedCount) {
        throw std::runtime_error("complex field serialized value count does not match dimensions");
    }
}

} // namespace

auto writeComplexField(const std::string& path, const ComplexField2D& field) -> void {
    validateMeta(field.meta);
    validateValueCount(field.values.num_elements() * static_cast<std::size_t>(kComplexComponentCount),
                       field.meta);

    proto::ComplexField2DData message;
    message.set_width(field.meta.width);
    message.set_height(field.meta.height);
    message.set_originx(field.meta.originX);
    message.set_originy(field.meta.originY);
    message.set_scale(field.meta.scale);
    message.mutable_values()->Reserve(static_cast<int>(field.values.num_elements() *
                                                       static_cast<std::size_t>(kComplexComponentCount)));

    for (uint32_t xIndex = 0; xIndex < field.meta.width; ++xIndex) {
        for (uint32_t yIndex = 0; yIndex < field.meta.height; ++yIndex) {
            const std::complex<double> value = field.values[xIndex][yIndex];
            message.add_values(value.real());
            message.add_values(value.imag());
        }
    }

    const std::filesystem::path outputPath(path);
    if (outputPath.has_parent_path()) {
        std::filesystem::create_directories(outputPath.parent_path());
    }

    std::ofstream outputStream(path, std::ios::binary);
    if (!outputStream.is_open()) {
        throw std::runtime_error("failed to open complex field output file: " + path);
    }
    if (!message.SerializeToOstream(&outputStream)) {
        throw std::runtime_error("failed to serialize complex field output file: " + path);
    }
}

auto readComplexField(const std::string& path) -> ComplexField2D {
    std::ifstream inputStream(path, std::ios::binary);
    if (!inputStream.is_open()) {
        throw std::runtime_error("failed to open complex field input file: " + path);
    }

    proto::ComplexField2DData message;
    if (!message.ParseFromIstream(&inputStream)) {
        throw std::runtime_error("failed to parse complex field input file: " + path);
    }

    ComplexField2D field;
    field.meta.width = message.width();
    field.meta.height = message.height();
    field.meta.originX = message.originx();
    field.meta.originY = message.originy();
    field.meta.scale = message.scale();
    validateMeta(field.meta);
    validateValueCount(static_cast<std::size_t>(message.values_size()), field.meta);

    using FieldIndex = boost::multi_array<std::complex<double>, 2>::index;
    field.values.resize(boost::extents[static_cast<FieldIndex>(field.meta.width)]
                                     [static_cast<FieldIndex>(field.meta.height)]);

    int valueIndex = 0;
    for (uint32_t xIndex = 0; xIndex < field.meta.width; ++xIndex) {
        for (uint32_t yIndex = 0; yIndex < field.meta.height; ++yIndex) {
            field.values[xIndex][yIndex] =
                std::complex<double>(message.values(valueIndex), message.values(valueIndex + 1));
            valueIndex += kComplexComponentCount;
        }
    }
    return field;
}

} // namespace laby::generator
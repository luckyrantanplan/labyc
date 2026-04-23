#pragma once

#include <boost/multi_array.hpp>
#include <complex>
#include <cstdint>
#include <string>

namespace laby::generator {

struct ComplexField2DMeta {
    uint32_t width = 0;
    uint32_t height = 0;
    double originX = 0.0;
    double originY = 0.0;
    double scale = 1.0;
};

struct ComplexField2D {
    ComplexField2DMeta meta;
    boost::multi_array<std::complex<double>, 2> values;
};

auto writeComplexField(const std::string& path, const ComplexField2D& field) -> void;
auto readComplexField(const std::string& path) -> ComplexField2D;

} // namespace laby::generator
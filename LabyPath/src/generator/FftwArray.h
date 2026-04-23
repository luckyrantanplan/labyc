/**
 * @file FftwArray.h
 * @brief Minimal FFTW3-backed arrays and plan wrappers for noise generation.
 *
 * Replaces the vendored fftw++ / Array.h library with direct FFTW3 usage.
 * Provides RAII wrappers for fftw_malloc/fftw_free and fftw_plan.
 */

#pragma once

#include <cassert>
#include <complex>
#include <cstddef>
#include <cstdint>
#include <fftw3.h>
#include <gsl/span>
#include <stdexcept>

namespace laby::fft {

class ComplexRef {
    fftw_complex* _value = nullptr;

  public:
    explicit ComplexRef(fftw_complex& value) : _value(&value) {}

    auto operator=(const std::complex<double>& complexValue) -> ComplexRef& {
        (*_value)[0] = complexValue.real();
        (*_value)[1] = complexValue.imag();
        return *this;
    }

    auto operator=(double realValue) -> ComplexRef& {
        (*_value)[0] = realValue;
        (*_value)[1] = 0.0;
        return *this;
    }

    [[nodiscard]] auto real() const -> double {
        return (*_value)[0];
    }

    void real(double realValue) {
        (*_value)[0] = realValue;
    }

    [[nodiscard]] auto imag() const -> double {
        return (*_value)[1];
    }

    void imag(double imaginaryValue) {
        (*_value)[1] = imaginaryValue;
    }

    auto operator*=(double scale) -> ComplexRef& {
        (*_value)[0] *= scale;
        (*_value)[1] *= scale;
        return *this;
    }

    [[nodiscard]] auto toComplex() const -> std::complex<double> {
        return {(*_value)[0], (*_value)[1]};
    }

    // NOLINTNEXTLINE(google-explicit-constructor)
    operator std::complex<double>() const {
        return toComplex();
    }
};

/**
 * @brief 1-D complex array backed by fftw_malloc (SIMD-aligned).
 */
class Array1D {
    fftw_complex* _data = nullptr;
    uint32_t _nx = 0;

  public:
    explicit Array1D(uint32_t sampleCount)
        : _data{static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * sampleCount))},
          _nx{sampleCount} {
        if (_data == nullptr) {
            throw std::bad_alloc();
        }
    }

    ~Array1D() {
        if (_data != nullptr) {
            fftw_free(_data);
        }
    }

    Array1D(const Array1D&) = delete;
    auto operator=(const Array1D&) -> Array1D& = delete;

    Array1D(Array1D&& other) noexcept : _data{other._data}, _nx{other._nx} {
        other._data = nullptr;
        other._nx = 0;
    }

    auto operator=(Array1D&& other) noexcept -> Array1D& {
        if (this != &other) {
            if (_data != nullptr) {
                fftw_free(_data);
            }
            _data = other._data;
            _nx = other._nx;
            other._data = nullptr;
            other._nx = 0;
        }
        return *this;
    }

    [[nodiscard]] auto size() const -> uint32_t {
        return _nx;
    }
    auto raw() -> fftw_complex* {
        return _data;
    }
    [[nodiscard]] auto raw() const -> const fftw_complex* {
        return _data;
    }

    auto operator()(uint32_t sampleIndex) const -> ComplexRef {
        assert(sampleIndex < _nx);
        return ComplexRef{_data[sampleIndex]};
    }

    auto operator()(int32_t sampleIndex) const -> ComplexRef {
        assert(sampleIndex >= 0);
        return (*this)(static_cast<uint32_t>(sampleIndex));
    }

  private:
    [[nodiscard]] auto elements() -> gsl::span<fftw_complex> {
        return {_data, static_cast<std::size_t>(_nx)};
    }

    [[nodiscard]] auto constElements() const -> gsl::span<const fftw_complex> {
        return {_data, static_cast<std::size_t>(_nx)};
    }
};

/**
 * @brief 2-D complex array backed by fftw_malloc (SIMD-aligned, row-major).
 */
class Array2D {
    fftw_complex* _data = nullptr;
    uint32_t _nx = 0;
    uint32_t _ny = 0;

  public:
    Array2D(uint32_t xSampleCount, uint32_t ySampleCount)
        : _data{static_cast<fftw_complex*>(
              fftw_malloc(sizeof(fftw_complex) * xSampleCount * ySampleCount))},
          _nx{xSampleCount}, _ny{ySampleCount} {
        if (_data == nullptr) {
            throw std::bad_alloc();
        }
    }

    ~Array2D() {
        if (_data != nullptr) {
            fftw_free(_data);
        }
    }

    Array2D(const Array2D&) = delete;
    auto operator=(const Array2D&) -> Array2D& = delete;

    Array2D(Array2D&& other) noexcept : _data{other._data}, _nx{other._nx}, _ny{other._ny} {
        other._data = nullptr;
        other._nx = 0;
        other._ny = 0;
    }

    auto operator=(Array2D&& other) noexcept -> Array2D& {
        if (this != &other) {
            if (_data != nullptr) {
                fftw_free(_data);
            }
            _data = other._data;
            _nx = other._nx;
            _ny = other._ny;
            other._data = nullptr;
            other._nx = 0;
            other._ny = 0;
        }
        return *this;
    }

    [[nodiscard]] auto nx() const -> uint32_t {
        return _nx;
    }
    [[nodiscard]] auto ny() const -> uint32_t {
        return _ny;
    }
    auto raw() -> fftw_complex* {
        return _data;
    }
    [[nodiscard]] auto raw() const -> const fftw_complex* {
        return _data;
    }

    auto operator()(uint32_t xIndex, uint32_t yIndex) const -> ComplexRef {
        assert(xIndex < _nx && yIndex < _ny);
        return ComplexRef{_data[flatIndex(xIndex, yIndex)]};
    }

    auto operator()(int32_t xIndex, int32_t yIndex) const -> ComplexRef {
        assert(xIndex >= 0 && yIndex >= 0);
        return (*this)(static_cast<uint32_t>(xIndex), static_cast<uint32_t>(yIndex));
    }

  private:
    [[nodiscard]] auto elements() -> gsl::span<fftw_complex> {
        return {_data, static_cast<std::size_t>(_nx) * static_cast<std::size_t>(_ny)};
    }

    [[nodiscard]] auto constElements() const -> gsl::span<const fftw_complex> {
        return {_data, static_cast<std::size_t>(_nx) * static_cast<std::size_t>(_ny)};
    }

    [[nodiscard]] auto flatIndex(uint32_t xIndex, uint32_t yIndex) const -> std::size_t {
        return static_cast<std::size_t>(xIndex) * _ny + yIndex;
    }
};

/**
 * @brief RAII wrapper for fftw_plan.
 */
class Plan {
    fftw_plan _plan = nullptr;

  public:
    Plan() = default;

    explicit Plan(fftw_plan rawPlan) : _plan{rawPlan} {
        if (_plan == nullptr) {
            throw std::runtime_error("fftw_plan creation failed");
        }
    }

    ~Plan() {
        if (_plan != nullptr) {
            fftw_destroy_plan(_plan);
        }
    }

    Plan(const Plan&) = delete;
    auto operator=(const Plan&) -> Plan& = delete;

    Plan(Plan&& other) noexcept : _plan{other._plan} {
        other._plan = nullptr;
    }

    auto operator=(Plan&& other) noexcept -> Plan& {
        if (this != &other) {
            if (_plan != nullptr) {
                fftw_destroy_plan(_plan);
            }
            _plan = other._plan;
            other._plan = nullptr;
        }
        return *this;
    }

    void execute() {
        if (_plan != nullptr) {
            fftw_execute(_plan);
        }
    }
};

} // namespace laby::fft

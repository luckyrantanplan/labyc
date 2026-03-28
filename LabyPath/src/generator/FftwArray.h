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
#include <cstdint>
#include <fftw3.h>
#include <stdexcept>


namespace laby::fft {

/**
 * @brief 1-D complex array backed by fftw_malloc (SIMD-aligned).
 */
class Array1D {
    fftw_complex* _data = nullptr;
    uint32_t _nx = 0;

  public:
    explicit Array1D(uint32_t nx)
        : _data{static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * nx))}, _nx{nx} {
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

    auto operator[](uint32_t i) -> std::complex<double>& {
        return reinterpret_cast<std::complex<double>*>(_data)[i];
    }
    auto operator[](uint32_t i) const -> const std::complex<double>& {
        return reinterpret_cast<const std::complex<double>*>(_data)[i];
    }
    auto operator[](int32_t i) -> std::complex<double>& {
        assert(i >= 0);
        return reinterpret_cast<std::complex<double>*>(_data)[i];
    }
    auto operator[](int32_t i) const -> const std::complex<double>& {
        assert(i >= 0);
        return reinterpret_cast<const std::complex<double>*>(_data)[i];
    }

    auto operator()(uint32_t i) -> std::complex<double>& {
        return (*this)[i];
    }
    auto operator()(uint32_t i) const -> const std::complex<double>& {
        return (*this)[i];
    }
    auto operator()(int32_t i) -> std::complex<double>& {
        assert(i >= 0);
        return (*this)[static_cast<uint32_t>(i)];
    }
    auto operator()(int32_t i) const -> const std::complex<double>& {
        assert(i >= 0);
        return (*this)[static_cast<uint32_t>(i)];
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
    Array2D(uint32_t nx, uint32_t ny)
        : _data{static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * nx * ny))}, _nx{nx},
          _ny{ny} {
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

    /** arr[i] returns pointer to row i, so arr[i][j] works. */
    auto operator[](uint32_t i) -> std::complex<double>* {
        return reinterpret_cast<std::complex<double>*>(_data) + static_cast<std::size_t>(i) * _ny;
    }
    auto operator[](uint32_t i) const -> const std::complex<double>* {
        return reinterpret_cast<const std::complex<double>*>(_data) +
               static_cast<std::size_t>(i) * _ny;
    }
    auto operator[](int32_t i) -> std::complex<double>* {
        assert(i >= 0);
        return reinterpret_cast<std::complex<double>*>(_data) +
               static_cast<std::size_t>(static_cast<uint32_t>(i)) * _ny;
    }
    auto operator[](int32_t i) const -> const std::complex<double>* {
        assert(i >= 0);
        return reinterpret_cast<const std::complex<double>*>(_data) +
               static_cast<std::size_t>(static_cast<uint32_t>(i)) * _ny;
    }

    auto operator()(uint32_t i, uint32_t j) -> std::complex<double>& {
        return reinterpret_cast<std::complex<double>*>(_data)[i * _ny + j];
    }
    auto operator()(uint32_t i, uint32_t j) const -> const std::complex<double>& {
        return reinterpret_cast<const std::complex<double>*>(_data)[i * _ny + j];
    }
    auto operator()(int32_t i, int32_t j) -> std::complex<double>& {
        assert(i >= 0 && j >= 0);
        return reinterpret_cast<std::complex<double>*>(
            _data)[static_cast<uint32_t>(i) * _ny + static_cast<uint32_t>(j)];
    }
    auto operator()(int32_t i, int32_t j) const -> const std::complex<double>& {
        assert(i >= 0 && j >= 0);
        return reinterpret_cast<const std::complex<double>*>(
            _data)[static_cast<uint32_t>(i) * _ny + static_cast<uint32_t>(j)];
    }
};

/**
 * @brief RAII wrapper for fftw_plan.
 */
class Plan {
    fftw_plan _plan = nullptr;

  public:
    Plan() = default;

    explicit Plan(fftw_plan p) : _plan{p} {
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


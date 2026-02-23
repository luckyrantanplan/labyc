/**
 * @file FftwArray.h
 * @brief Minimal FFTW3-backed arrays and plan wrappers for noise generation.
 *
 * Replaces the vendored fftw++ / Array.h library with direct FFTW3 usage.
 * Provides RAII wrappers for fftw_malloc/fftw_free and fftw_plan.
 */

#pragma once

#include <complex>
#include <cstdint>
#include <stdexcept>
#include <fftw3.h>

namespace laby {
namespace fft {

/**
 * @brief 1-D complex array backed by fftw_malloc (SIMD-aligned).
 */
class Array1D {
    fftw_complex* data_ = nullptr;
    uint32_t nx_ = 0;

public:
    explicit Array1D(uint32_t nx)
        : data_{static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * nx))}
        , nx_{nx} {
        if (!data_) {
            throw std::bad_alloc();
        }
    }

    ~Array1D() {
        if (data_) {
            fftw_free(data_);
        }
    }

    Array1D(const Array1D&) = delete;
    Array1D& operator=(const Array1D&) = delete;

    Array1D(Array1D&& other) noexcept
        : data_{other.data_}, nx_{other.nx_} {
        other.data_ = nullptr;
        other.nx_ = 0;
    }

    Array1D& operator=(Array1D&& other) noexcept {
        if (this != &other) {
            if (data_) {
                fftw_free(data_);
            }
            data_ = other.data_;
            nx_ = other.nx_;
            other.data_ = nullptr;
            other.nx_ = 0;
        }
        return *this;
    }

    uint32_t size() const { return nx_; }
    fftw_complex* raw() { return data_; }
    const fftw_complex* raw() const { return data_; }

    std::complex<double>& operator[](uint32_t i) {
        return reinterpret_cast<std::complex<double>*>(data_)[i];
    }
    const std::complex<double>& operator[](uint32_t i) const {
        return reinterpret_cast<const std::complex<double>*>(data_)[i];
    }
    std::complex<double>& operator[](int32_t i) {
        return reinterpret_cast<std::complex<double>*>(data_)[i];
    }
    const std::complex<double>& operator[](int32_t i) const {
        return reinterpret_cast<const std::complex<double>*>(data_)[i];
    }

    std::complex<double>& operator()(uint32_t i) { return (*this)[i]; }
    const std::complex<double>& operator()(uint32_t i) const { return (*this)[i]; }
    std::complex<double>& operator()(int32_t i) { return (*this)[i]; }
    const std::complex<double>& operator()(int32_t i) const { return (*this)[i]; }
};

/**
 * @brief 2-D complex array backed by fftw_malloc (SIMD-aligned, row-major).
 */
class Array2D {
    fftw_complex* data_ = nullptr;
    uint32_t nx_ = 0;
    uint32_t ny_ = 0;

public:
    Array2D(uint32_t nx, uint32_t ny)
        : data_{static_cast<fftw_complex*>(
              fftw_malloc(sizeof(fftw_complex) * nx * ny))}
        , nx_{nx}
        , ny_{ny} {
        if (!data_) {
            throw std::bad_alloc();
        }
    }

    ~Array2D() {
        if (data_) {
            fftw_free(data_);
        }
    }

    Array2D(const Array2D&) = delete;
    Array2D& operator=(const Array2D&) = delete;

    Array2D(Array2D&& other) noexcept
        : data_{other.data_}, nx_{other.nx_}, ny_{other.ny_} {
        other.data_ = nullptr;
        other.nx_ = 0;
        other.ny_ = 0;
    }

    Array2D& operator=(Array2D&& other) noexcept {
        if (this != &other) {
            if (data_) {
                fftw_free(data_);
            }
            data_ = other.data_;
            nx_ = other.nx_;
            ny_ = other.ny_;
            other.data_ = nullptr;
            other.nx_ = 0;
            other.ny_ = 0;
        }
        return *this;
    }

    uint32_t nx() const { return nx_; }
    uint32_t ny() const { return ny_; }
    fftw_complex* raw() { return data_; }
    const fftw_complex* raw() const { return data_; }

    /** arr[i] returns pointer to row i, so arr[i][j] works. */
    std::complex<double>* operator[](uint32_t i) {
        return reinterpret_cast<std::complex<double>*>(data_) + i * ny_;
    }
    const std::complex<double>* operator[](uint32_t i) const {
        return reinterpret_cast<const std::complex<double>*>(data_) + i * ny_;
    }
    std::complex<double>* operator[](int32_t i) {
        return reinterpret_cast<std::complex<double>*>(data_) + static_cast<uint32_t>(i) * ny_;
    }
    const std::complex<double>* operator[](int32_t i) const {
        return reinterpret_cast<const std::complex<double>*>(data_) + static_cast<uint32_t>(i) * ny_;
    }

    std::complex<double>& operator()(uint32_t i, uint32_t j) {
        return reinterpret_cast<std::complex<double>*>(data_)[i * ny_ + j];
    }
    const std::complex<double>& operator()(uint32_t i, uint32_t j) const {
        return reinterpret_cast<const std::complex<double>*>(data_)[i * ny_ + j];
    }
    std::complex<double>& operator()(int32_t i, int32_t j) {
        return reinterpret_cast<std::complex<double>*>(data_)[static_cast<uint32_t>(i) * ny_ + static_cast<uint32_t>(j)];
    }
    const std::complex<double>& operator()(int32_t i, int32_t j) const {
        return reinterpret_cast<const std::complex<double>*>(data_)[static_cast<uint32_t>(i) * ny_ + static_cast<uint32_t>(j)];
    }
};

/**
 * @brief RAII wrapper for fftw_plan.
 */
class Plan {
    fftw_plan plan_ = nullptr;

public:
    Plan() = default;

    explicit Plan(fftw_plan p) : plan_{p} {
        if (!plan_) {
            throw std::runtime_error("fftw_plan creation failed");
        }
    }

    ~Plan() {
        if (plan_) {
            fftw_destroy_plan(plan_);
        }
    }

    Plan(const Plan&) = delete;
    Plan& operator=(const Plan&) = delete;

    Plan(Plan&& other) noexcept : plan_{other.plan_} {
        other.plan_ = nullptr;
    }

    Plan& operator=(Plan&& other) noexcept {
        if (this != &other) {
            if (plan_) {
                fftw_destroy_plan(plan_);
            }
            plan_ = other.plan_;
            other.plan_ = nullptr;
        }
        return *this;
    }

    void execute() {
        if (plan_) {
            fftw_execute(plan_);
        }
    }
};

} // namespace fft
} // namespace laby

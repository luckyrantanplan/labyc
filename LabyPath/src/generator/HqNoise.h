/*
 * HqNoise.h
 *
 *  Created on: Nov 12, 2017
 *      Author: florian
 */

#ifndef HQNOISE_H_
#define HQNOISE_H_

#include <cstdint>
#include <algorithm>
#include <complex>
#include <random>

#include "FftwArray.h"


namespace laby::generator {
struct HqNoiseConfig {

    uint32_t maxN;
    uint32_t accuracy;
    double amplitude;
    double seed;
    struct Gaussian {
        double frequency;

    };
    Gaussian gaussian;
    struct PowerLaw {
        double frequency;
        double power;
    };
    PowerLaw powerlaw;
    bool complex;
};

struct HqNoiseUtils {

    static auto fastFloor(double f) -> int32_t;
    static auto interpLinear(double t) -> double;
    static auto lerp(double a, double b, double t) -> double;
    static auto lerpC(const std::complex<double>& a, const std::complex<double>& b, double t) -> std::complex<double>;
};

class HqNoise1D {
public:
    const HqNoiseConfig config;
    fft::Array1D arr;
    static auto sgn(uint32_t val) -> double;
    explicit HqNoise1D(const HqNoiseConfig& config);

    [[nodiscard]] auto get(double x) const -> double;
    [[nodiscard]] auto getComplex(double x) const -> std::complex<double>;

private:
    void normalize(uint32_t nx);
};

class HqNoise2D {
public:
    const HqNoiseConfig config;
    fft::Array2D arr;

    static auto sgn(uint32_t x, uint32_t y) -> double;

    explicit HqNoise2D(const HqNoiseConfig& config);

    [[nodiscard]] auto get(double x, double y) const -> double;
    [[nodiscard]] auto getComplex(double x, double y) const -> std::complex<double>;
    [[nodiscard]] auto getComplex(const std::complex<double>& c) const -> std::complex<double>;

private:
    void normalize(uint32_t nx, uint32_t ny);
};
} // namespace laby::generator


#endif /* HQNOISE_H_ */

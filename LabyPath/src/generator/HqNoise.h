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

    static auto fastFloor(double value) -> int32_t;
    static auto interpLinear(double position) -> double;
    static auto lerp(double startValue, double endValue, double position) -> double;
    static auto lerpC(const std::complex<double>& startValue,
                      const std::complex<double>& endValue,
                      double position) -> std::complex<double>;
};

class HqNoise1D {
  public:
    static auto sgn(uint32_t sampleIndex) -> double;
    explicit HqNoise1D(const HqNoiseConfig& config);

    [[nodiscard]] auto config() const -> const HqNoiseConfig& {
        return _config;
    }

    [[nodiscard]] auto get(double xCoordinate) const -> double;
    [[nodiscard]] auto getComplex(double xCoordinate) const -> std::complex<double>;

  private:
    void normalize(uint32_t sampleCount);

    HqNoiseConfig _config;
    fft::Array1D _array;
};

class HqNoise2D {
  public:
    static auto sgn(uint32_t xIndex, uint32_t yIndex) -> double;

    explicit HqNoise2D(const HqNoiseConfig& config);

    [[nodiscard]] auto config() const -> const HqNoiseConfig& {
        return _config;
    }

    [[nodiscard]] auto get(double xCoordinate, double yCoordinate) const -> double;
    [[nodiscard]] auto getComplex(double xCoordinate, double yCoordinate) const
        -> std::complex<double>;
    [[nodiscard]] auto getComplex(const std::complex<double>& complexPoint) const
        -> std::complex<double>;

  private:
    void normalize(uint32_t xSampleCount, uint32_t ySampleCount);

    HqNoiseConfig _config;
    fft::Array2D _array;
};
} // namespace laby::generator


#endif /* HQNOISE_H_ */

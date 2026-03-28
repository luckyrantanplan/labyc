/*
 * HqNoise.cpp
 *
 *  Created on: Nov 12, 2017
 *      Author: florian
 */

#include "HqNoise.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <cstdint>
#include <fftw3.h>
#include <limits>
#include <random>

#include "../basic/RandomUniDist.h"
#include "generator/FftwArray.h"


namespace laby::generator {
auto HqNoiseUtils::lerp(double startValue, double endValue, double position) -> double {
    return startValue + position * (endValue - startValue);
}

auto HqNoiseUtils::lerpC(const std::complex<double>& startValue,
                         const std::complex<double>& endValue,
                         double position) -> std::complex<double> {
    return startValue + position * (endValue - startValue);
}

auto HqNoiseUtils::fastFloor(double value) -> int32_t {
    auto integerValue = static_cast<int32_t>(value);
    return (value >= 0 || value == static_cast<double>(integerValue)) ? integerValue
                                                                      : integerValue - 1;
}

auto HqNoise2D::get(const double xCoordinate, const double yCoordinate) const -> double {
    double const xScaled = xCoordinate * _config.accuracy;
    double const yScaled = yCoordinate * _config.accuracy;
    int32_t const xBase = HqNoiseUtils::fastFloor(xScaled);
    int32_t const yBase = HqNoiseUtils::fastFloor(yScaled);
    int32_t const xNext = xBase + 1;
    int32_t const yNext = yBase + 1;

    double const xFraction = xScaled - static_cast<double>(xBase);
    double const yFraction = yScaled - static_cast<double>(yBase);

    double const xInterpolatedLow =
        HqNoiseUtils::lerp(_array[xBase][yBase].real(), _array[xNext][yBase].real(), xFraction);
    double const xInterpolatedHigh = HqNoiseUtils::lerp(_array[xBase][yNext].real(),
                                                        _array[xNext][yNext].real(), xFraction);

    return HqNoiseUtils::lerp(xInterpolatedLow, xInterpolatedHigh, yFraction);
}

auto HqNoise2D::getComplex(const std::complex<double>& complexPoint) const
    -> std::complex<double> {
    return getComplex(complexPoint.real(), complexPoint.imag());
}

auto HqNoise2D::getComplex(const double xCoordinate, const double yCoordinate) const
    -> std::complex<double> {
    double const xScaled = xCoordinate * _config.accuracy;
    double const yScaled = yCoordinate * _config.accuracy;

    int32_t const xBase = HqNoiseUtils::fastFloor(xScaled);
    int32_t const yBase = HqNoiseUtils::fastFloor(yScaled);
    int32_t const xNext = xBase + 1;
    int32_t const yNext = yBase + 1;

    double const xFraction = xScaled - static_cast<double>(xBase);
    double const yFraction = yScaled - static_cast<double>(yBase);

    std::complex<double> const xInterpolatedLow =
        HqNoiseUtils::lerpC(_array[xBase][yBase], _array[xNext][yBase], xFraction);
    std::complex<double> const xInterpolatedHigh =
        HqNoiseUtils::lerpC(_array[xBase][yNext], _array[xNext][yNext], xFraction);

    return HqNoiseUtils::lerpC(xInterpolatedLow, xInterpolatedHigh, yFraction);
}

auto HqNoise2D::sgn(uint32_t xIndex, uint32_t yIndex) -> double {
    return (static_cast<int32_t>((yIndex + xIndex) % 2)) * 2 - 1;
}

void HqNoise2D::normalize(uint32_t xSampleCount, uint32_t ySampleCount) {
    double maxVal = std::numeric_limits<double>::min();
    double minVal = std::numeric_limits<double>::max();
    for (uint32_t xIndex = 0; xIndex < xSampleCount; ++xIndex) {
        for (uint32_t yIndex = 0; yIndex < ySampleCount; ++yIndex) {
            double const value = sgn(xIndex, yIndex) * _array[xIndex][yIndex].real();
            maxVal = std::max(maxVal, value);
            minVal = std::min(minVal, value);
        }
    }
    double mid = (maxVal + minVal) / 2.;
    double maxAmpl = 2. * _config.amplitude / (maxVal - minVal);

    for (uint32_t yIndex = 0; yIndex < ySampleCount; ++yIndex) {
        for (uint32_t xIndex = 0; xIndex < xSampleCount; ++xIndex) {
            double const normalizedValue =
                _array[xIndex][yIndex].real() * sgn(xIndex, yIndex) - mid;
            _array[xIndex][yIndex].real(normalizedValue * maxAmpl);
        }
    }
    if (_config.complex) {
        // imaginary part
        maxVal = std::numeric_limits<double>::min();
        minVal = std::numeric_limits<double>::max();
        for (uint32_t xIndex = 0; xIndex < xSampleCount; ++xIndex) {
            for (uint32_t yIndex = 0; yIndex < ySampleCount; ++yIndex) {
                double const value = sgn(xIndex, yIndex) * _array[xIndex][yIndex].imag();
                maxVal = std::max(maxVal, value);
                minVal = std::min(minVal, value);
            }
        }

        mid = (maxVal + minVal) / 2.;
        maxAmpl = 2. * _config.amplitude / (maxVal - minVal);
        for (uint32_t yIndex = 0; yIndex < ySampleCount; ++yIndex) {
            for (uint32_t xIndex = 0; xIndex < xSampleCount; ++xIndex) {
                double const normalizedValue =
                    _array[xIndex][yIndex].imag() * sgn(xIndex, yIndex) - mid;
                _array[xIndex][yIndex].imag(normalizedValue * maxAmpl);
            }
        }
    }
}

HqNoise2D::HqNoise2D(const HqNoiseConfig& noiseConfig)
    : _config{noiseConfig}
    , _array{noiseConfig.maxN * noiseConfig.accuracy + 2,
             noiseConfig.maxN * noiseConfig.accuracy + 2} {

    uint32_t const xSampleCount = _config.maxN * _config.accuracy + 2;
    uint32_t const ySampleCount = _config.maxN * _config.accuracy + 2;
    basic::RandomUniDist distribution(0, 100.0, static_cast<uint32_t>(_config.seed));

    // Create FFTW plans before filling data (FFTW_ESTIMATE does not touch the array)
    fft::Plan forward{fftw_plan_dft_2d(
        static_cast<int>(xSampleCount), static_cast<int>(ySampleCount),
        _array.raw(), _array.raw(), FFTW_FORWARD, FFTW_ESTIMATE)};
    fft::Plan backward{fftw_plan_dft_2d(
        static_cast<int>(xSampleCount), static_cast<int>(ySampleCount),
        _array.raw(), _array.raw(), FFTW_BACKWARD, FFTW_ESTIMATE)};

    for (uint32_t xIndex = 0; xIndex < xSampleCount; ++xIndex) {
        for (uint32_t yIndex = 0; yIndex < ySampleCount; ++yIndex) {
            double const realSample = distribution.get();
            if (_config.complex) {
                _array(xIndex, yIndex) = std::complex<double>(realSample * sgn(xIndex, yIndex),
                                                              distribution.get() * sgn(xIndex, yIndex));
            } else {
                _array(xIndex, yIndex) = std::complex<double>(realSample * sgn(xIndex, yIndex), 0);
            }
        }
    }

    forward.execute();

    double const power = _config.powerlaw.power * .5;
    double const pfreq = _config.powerlaw.frequency;
    double const quot = pfreq * pfreq;

    double const gfreq = _config.gaussian.frequency;
    double const gaussVariance = 2. * gfreq * gfreq;

    // cut freq 0
    _array[xSampleCount / 2][ySampleCount / 2] = 0;

    for (uint32_t xIndex = 0; xIndex < xSampleCount; ++xIndex) {
        for (uint32_t yIndex = 0; yIndex < ySampleCount; ++yIndex) {
            double const xDistance = static_cast<double>(xIndex) - xSampleCount / 2.;
            double const yDistance = static_cast<double>(yIndex) - ySampleCount / 2.;
            double const squaredDistance = xDistance * xDistance + yDistance * yDistance;
            double const powerLawQuotient = squaredDistance / quot;
            if (powerLawQuotient >= 1.) {
                _array[xIndex][yIndex] *= std::pow(powerLawQuotient, -power);
            }
            if (gaussVariance > 0) {
                _array[xIndex][yIndex] *= std::exp(-squaredDistance / gaussVariance);
            }
        }
    }

    backward.execute();
    normalize(xSampleCount, ySampleCount);
}

auto HqNoise1D::sgn(uint32_t sampleIndex) -> double {
    return (static_cast<int32_t>(sampleIndex % 2)) * 2 - 1;
}

void HqNoise1D::normalize(uint32_t sampleCount) {
    double maxVal = std::numeric_limits<double>::min();
    double minVal = std::numeric_limits<double>::max();
    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        double const value = sgn(sampleIndex) * _array[sampleIndex].real();
        maxVal = std::max(maxVal, value);
        minVal = std::min(minVal, value);
    }
    double mid = (maxVal + minVal) / 2.;
    double maxAmpl = 2. * _config.amplitude / (maxVal - minVal);
    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        double const normalizedValue = _array[sampleIndex].real() * sgn(sampleIndex) - mid;
        _array[sampleIndex].real(normalizedValue * maxAmpl);
    }
    if (_config.complex) {
        maxVal = std::numeric_limits<double>::min();
        minVal = std::numeric_limits<double>::max();
        for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
            double const value = sgn(sampleIndex) * _array[sampleIndex].imag();
            maxVal = std::max(maxVal, value);
            minVal = std::min(minVal, value);
        }
        mid = (maxVal + minVal) / 2.;
        maxAmpl = 2. * _config.amplitude / (maxVal - minVal);
        for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
            double const normalizedValue = _array[sampleIndex].imag() * sgn(sampleIndex) - mid;
            _array[sampleIndex].imag(normalizedValue * maxAmpl);
        }
    }
}

HqNoise1D::HqNoise1D(const HqNoiseConfig& noiseConfig)
    : _config{noiseConfig}
    , _array{noiseConfig.maxN * noiseConfig.accuracy + 2} {

    uint32_t const sampleCount = _config.maxN * _config.accuracy + 2;

    std::default_random_engine generator(static_cast<uint32_t>(_config.seed));
    std::uniform_real_distribution<double> distribution(0, 100.0);

    // Create FFTW plans before filling data (FFTW_ESTIMATE does not touch the array)
    fft::Plan forward{fftw_plan_dft_1d(
        static_cast<int>(sampleCount),
        _array.raw(), _array.raw(), FFTW_FORWARD, FFTW_ESTIMATE)};
    fft::Plan backward{fftw_plan_dft_1d(
        static_cast<int>(sampleCount),
        _array.raw(), _array.raw(), FFTW_BACKWARD, FFTW_ESTIMATE)};

    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        double const realSample = distribution(generator);
        if (_config.complex) {
            _array(sampleIndex) =
                std::complex<double>(realSample * sgn(sampleIndex), distribution(generator) * sgn(sampleIndex));
        } else {
            _array(sampleIndex) = std::complex<double>(realSample * sgn(sampleIndex), 0);
        }
    }

    forward.execute();

    // cut freq 0
    _array[sampleCount / 2] = 0;

    double const power = _config.powerlaw.power;
    double const pfreq = _config.powerlaw.frequency;

    double const gfreq = _config.gaussian.frequency;
    double const gaussVariance = 2. * gfreq * gfreq;

    for (uint32_t sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
        double const distanceFromCenter = static_cast<double>(sampleIndex) - sampleCount / 2.;
        double const powerLawQuotient = std::abs(distanceFromCenter) / pfreq;
        if (powerLawQuotient >= 1.) {
            _array[sampleIndex] *= std::pow(powerLawQuotient, -power);
        }
        if (gaussVariance > 0) {
            _array[sampleIndex] *= std::exp(-(distanceFromCenter * distanceFromCenter) / gaussVariance);
        }
    }

    backward.execute();
    normalize(sampleCount);
}

auto HqNoise1D::get(double xCoordinate) const -> double {
    double const xScaled = xCoordinate * _config.accuracy;
    int32_t const xBase = HqNoiseUtils::fastFloor(xScaled);
    int32_t const xNext = xBase + 1;

    double const xFraction = xScaled - static_cast<double>(xBase);

    double const xInterpolated =
        HqNoiseUtils::lerp(_array[xBase].real(), _array[xNext].real(), xFraction);

    return xInterpolated;
}

auto HqNoise1D::getComplex(double xCoordinate) const -> std::complex<double> {
    double const xScaled = xCoordinate * _config.accuracy;
    int32_t const xBase = HqNoiseUtils::fastFloor(xScaled);
    int32_t const xNext = xBase + 1;

    double const xFraction = xScaled - static_cast<double>(xBase);

    return HqNoiseUtils::lerpC(_array[xBase], _array[xNext], xFraction);
}
} // namespace laby::generator


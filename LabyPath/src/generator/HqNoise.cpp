/*
 * HqNoise.cpp
 *
 *  Created on: Nov 12, 2017
 *      Author: florian
 */

#include "HqNoise.h"

#include <cmath>
#include <limits>

#include "../basic/RandomUniDist.h"

namespace laby {
namespace generator {
double HqNoiseUtils::lerp(double a, double b, double t) {
    return a + t * (b - a);
}

std::complex<double> HqNoiseUtils::lerpC(const std::complex<double>& a, const std::complex<double>& b, double t) {
    return a + t * (b - a);
}

int32_t HqNoiseUtils::fastFloor(double f) {
    auto i = static_cast<int32_t>(f);
    return (f >= 0 || f == static_cast<double>(i)) ? i : i - 1;
}

double HqNoise2D::get(const double x, const double y) const {
    double xx = x * config.accuracy;
    double yy = y * config.accuracy;
    int32_t x0 = HqNoiseUtils::fastFloor(xx);
    int32_t y0 = HqNoiseUtils::fastFloor(yy);
    int32_t x1 = x0 + 1;
    int32_t y1 = y0 + 1;

    double xs = xx - static_cast<double>(x0);
    double ys = yy - static_cast<double>(y0);

    double xf0 = HqNoiseUtils::lerp(arr[x0][y0].real(), arr[x1][y0].real(), xs);
    double xf1 = HqNoiseUtils::lerp(arr[x0][y1].real(), arr[x1][y1].real(), xs);

    return HqNoiseUtils::lerp(xf0, xf1, ys);
}

std::complex<double> HqNoise2D::getComplex(const std::complex<double> &c) const {
    return getComplex(c.real(), c.imag());
}

std::complex<double> HqNoise2D::getComplex(const double x, const double y) const {
    double xx = x * config.accuracy;
    double yy = y * config.accuracy;

    int32_t x0 = HqNoiseUtils::fastFloor(xx);
    int32_t y0 = HqNoiseUtils::fastFloor(yy);
    int32_t x1 = x0 + 1;
    int32_t y1 = y0 + 1;

    double xs = xx - static_cast<double>(x0);
    double ys = yy - static_cast<double>(y0);

    std::complex<double> xf0 = HqNoiseUtils::lerpC(arr[x0][y0], arr[x1][y0], xs);
    std::complex<double> xf1 = HqNoiseUtils::lerpC(arr[x0][y1], arr[x1][y1], xs);

    return HqNoiseUtils::lerpC(xf0, xf1, ys);
}

double HqNoise2D::sgn(uint32_t x, uint32_t y) {
    return (static_cast<int32_t>((y + x) % 2)) * 2 - 1;
}

void HqNoise2D::normalize(uint32_t nx, uint32_t ny) {
    double maxVal = std::numeric_limits<double>::min();
    double minVal = std::numeric_limits<double>::max();
    for (uint32_t i = 0; i < nx; i++) {
        for (uint32_t j = 0; j < ny; j++) {
            double value = sgn(i, j) * arr[i][j].real();
            maxVal = std::max(maxVal, value);
            minVal = std::min(minVal, value);
        }
    }
    double mid = (maxVal + minVal) / 2.;
    double maxAmpl = 2. * config.amplitude / (maxVal - minVal);

    for (uint32_t y = 0; y < ny; ++y) {
        for (uint32_t x = 0; x < nx; ++x) {
            double norm2 = arr[x][y].real() * sgn(x, y) - mid;
            arr[x][y].real(norm2 * maxAmpl);
        }
    }
    if (config.complex) {
        // imaginary part
        maxVal = std::numeric_limits<double>::min();
        minVal = std::numeric_limits<double>::max();
        for (uint32_t i = 0; i < nx; i++) {
            for (uint32_t j = 0; j < ny; j++) {
                double value = sgn(i, j) * arr[i][j].imag();
                maxVal = std::max(maxVal, value);
                minVal = std::min(minVal, value);
            }
        }

        mid = (maxVal + minVal) / 2.;
        maxAmpl = 2. * config.amplitude / (maxVal - minVal);
        for (uint32_t y = 0; y < ny; ++y) {
            for (uint32_t x = 0; x < nx; ++x) {
                double norm2 = arr[x][y].imag() * sgn(x, y) - mid;
                arr[x][y].imag(norm2 * maxAmpl);
            }
        }
    }
}

HqNoise2D::HqNoise2D(const HqNoiseConfig& iconfig)
    : config{iconfig}
    , arr{iconfig.maxN * iconfig.accuracy + 2, iconfig.maxN * iconfig.accuracy + 2} {

    uint32_t nx = config.maxN * config.accuracy + 2;
    uint32_t ny = config.maxN * config.accuracy + 2;
    basic::RandomUniDist distribution(0, 100.0, static_cast<uint32_t>(config.seed));

    // Create FFTW plans before filling data (FFTW_ESTIMATE does not touch the array)
    fft::Plan forward{fftw_plan_dft_2d(
        static_cast<int>(nx), static_cast<int>(ny),
        arr.raw(), arr.raw(), FFTW_FORWARD, FFTW_ESTIMATE)};
    fft::Plan backward{fftw_plan_dft_2d(
        static_cast<int>(nx), static_cast<int>(ny),
        arr.raw(), arr.raw(), FFTW_BACKWARD, FFTW_ESTIMATE)};

    for (uint32_t i = 0; i < nx; i++) {
        for (uint32_t j = 0; j < ny; j++) {
            double x = distribution.get();
            if (config.complex) {
                arr(i, j) = std::complex<double>(x * sgn(i, j), distribution.get() * sgn(i, j));
            } else {
                arr(i, j) = std::complex<double>(x * sgn(i, j), 0);
            }
        }
    }

    forward.execute();

    double power = config.powerlaw.power * .5;
    double pfreq = config.powerlaw.frequency;
    double quot = pfreq * pfreq;

    double gfreq = config.gaussian.frequency;
    double gaussVariance = 2. * gfreq * gfreq;

    // cut freq 0
    arr[nx / 2][ny / 2] = 0;

    for (uint32_t i = 0; i < nx; i++) {
        for (uint32_t j = 0; j < ny; j++) {
            double xx = static_cast<double>(i) - nx / 2.;
            double yy = static_cast<double>(j) - ny / 2.;
            double d = xx * xx + yy * yy;
            double dquot = d / quot;
            if (dquot >= 1.) {
                arr[i][j] *= pow(dquot, -power);
            }
            if (gaussVariance > 0) {
                arr[i][j] *= exp(-d / gaussVariance);
            }
        }
    }

    backward.execute();
    normalize(nx, ny);
}

double HqNoise1D::sgn(uint32_t val) {
    return (static_cast<int32_t>(val % 2)) * 2 - 1;
}

void HqNoise1D::normalize(uint32_t nx) {
    double maxVal = std::numeric_limits<double>::min();
    double minVal = std::numeric_limits<double>::max();
    for (uint32_t i = 0; i < nx; ++i) {
        double value = sgn(i) * arr[i].real();
        maxVal = std::max(maxVal, value);
        minVal = std::min(minVal, value);
    }
    double mid = (maxVal + minVal) / 2.;
    double maxAmpl = 2. * config.amplitude / (maxVal - minVal);
    for (uint32_t x = 0; x < nx; ++x) {
        double norm2 = arr[x].real() * sgn(x) - mid;
        arr[x].real(norm2 * maxAmpl);
    }
    if (config.complex) {
        maxVal = std::numeric_limits<double>::min();
        minVal = std::numeric_limits<double>::max();
        for (uint32_t i = 0; i < nx; ++i) {
            double value = sgn(i) * arr[i].imag();
            maxVal = std::max(maxVal, value);
            minVal = std::min(minVal, value);
        }
        mid = (maxVal + minVal) / 2.;
        maxAmpl = 2. * config.amplitude / (maxVal - minVal);
        for (uint32_t x = 0; x < nx; ++x) {
            double norm2 = arr[x].imag() * sgn(x) - mid;
            arr[x].imag(norm2 * maxAmpl);
        }
    }
}

HqNoise1D::HqNoise1D(const HqNoiseConfig& iconfig)
    : config{iconfig}
    , arr{config.maxN * config.accuracy + 2} {

    uint32_t nx = config.maxN * config.accuracy + 2;

    std::default_random_engine generator(static_cast<uint32_t>(config.seed));
    std::uniform_real_distribution<double> distribution(0, 100.0);

    // Create FFTW plans before filling data (FFTW_ESTIMATE does not touch the array)
    fft::Plan forward{fftw_plan_dft_1d(
        static_cast<int>(nx),
        arr.raw(), arr.raw(), FFTW_FORWARD, FFTW_ESTIMATE)};
    fft::Plan backward{fftw_plan_dft_1d(
        static_cast<int>(nx),
        arr.raw(), arr.raw(), FFTW_BACKWARD, FFTW_ESTIMATE)};

    for (uint32_t i = 0; i < nx; i++) {
        double x = distribution(generator);
        if (config.complex) {
            arr(i) = std::complex<double>(x * sgn(i), distribution(generator) * sgn(i));
        } else {
            arr(i) = std::complex<double>(x * sgn(i), 0);
        }
    }

    forward.execute();

    // cut freq 0
    arr[nx / 2] = 0;

    double power = config.powerlaw.power;
    double pfreq = config.powerlaw.frequency;

    double gfreq = config.gaussian.frequency;
    double gaussVariance = 2. * gfreq * gfreq;

    for (uint32_t i = 0; i < nx; i++) {
        double xx = static_cast<double>(i) - nx / 2.;
        double dquot = std::abs(xx) / pfreq;
        if (dquot >= 1.) {
            arr[i] *= pow(dquot, -power);
        }
        if (gaussVariance > 0) {
            arr[i] *= exp(-(xx * xx) / gaussVariance);
        }
    }

    backward.execute();
    normalize(nx);
}

double HqNoise1D::get(double x) const {
    double xx = x * config.accuracy;
    int32_t x0 = HqNoiseUtils::fastFloor(xx);
    int32_t x1 = x0 + 1;

    double xs = xx - static_cast<double>(x0);

    double xf0 = HqNoiseUtils::lerp(arr[x0].real(), arr[x1].real(), xs);

    return xf0;
}

std::complex<double> HqNoise1D::getComplex(double x) const {
    double xx = x * config.accuracy;
    int32_t x0 = HqNoiseUtils::fastFloor(xx);
    int32_t x1 = x0 + 1;

    double xs = xx - static_cast<double>(x0);

    return HqNoiseUtils::lerpC(arr[x0], arr[x1], xs);
}
} /* namespace generator */
} /* namespace laby */

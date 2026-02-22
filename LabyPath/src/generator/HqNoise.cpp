/*
 * HqNoise.cpp
 *
 *  Created on: Nov 12, 2017
 *      Author: florian
 */

#include "HqNoise.h"

#include <cmath>
#include <cmath>
#include <limits>

#include "../basic/RandomUniDist.h"
#include "../fft/fftw++.h"

namespace laby {
namespace generator {
double HqNoiseUtils::lerp(double a, double b, double t) {
    return a + t * (b - a);
}

std::complex<double> HqNoiseUtils::lerpC(std::complex<double>& a, std::complex<double>& b, double t) {
    return a + t * (b - a);
}

int HqNoiseUtils::fastFloor(double f) {
    return (f >= 0 ? (int) f : (int) f - 1);
}

double HqNoise2D::get(const double x, const double y) const {
    double xx = x * config.accuracy;
    double yy = y * config.accuracy;
    int x0 = HqNoiseUtils::fastFloor(xx);
    int y0 = HqNoiseUtils::fastFloor(yy);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    double xs, ys;

    xs = xx - (double) x0;
    ys = yy - (double) y0;

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

    int x0 = HqNoiseUtils::fastFloor(xx);
    int y0 = HqNoiseUtils::fastFloor(yy);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    double xs, ys;

    xs = xx - (double) x0;
    ys = yy - (double) y0;

    std::complex<double> xf0 = HqNoiseUtils::lerpC(arr[x0][y0], arr[x1][y0], xs);

    std::complex<double> xf1 = HqNoiseUtils::lerpC(arr[x0][y1], arr[x1][y1], xs);

    return HqNoiseUtils::lerpC(xf0, xf1, ys);

}

double HqNoise2D::sgn(int x, int y) {
    return ((y + x) % 2) * 2 - 1;
}

void HqNoise2D::normalize(unsigned int nx, unsigned int ny) {
    double max = std::numeric_limits<double>::min();
    double min = std::numeric_limits<double>::max();
    for (uint32_t i = 0; i < nx; i++) {
        for (uint32_t j = 0; j < ny; j++) {
            double value = sgn(i, j) * arr[i][j].real();
            max = std::max(max, value);
            min = std::min(min, value);
        }
    }
    double mid = (max + min) / 2.;
    double maxAmpl = 2. * config.amplitude / (max - min);

    for (uint32_t y = 0; y < ny; ++y) {
        for (uint32_t x = 0; x < nx; ++x) {
            double norm2 = arr[x][y].real() * sgn(x, y) - mid;
            arr[x][y].real(norm2 * maxAmpl);
        }
    }
    if (config.complex) {
        //imaginary part
        max = std::numeric_limits<double>::min();
        min = std::numeric_limits<double>::max();
        for (uint32_t i = 0; i < nx; i++) {
            for (uint32_t j = 0; j < ny; j++) {
                double value = sgn(i, j) * arr[i][j].imag();
                max = std::max(max, value);
                min = std::min(min, value);
            }
        }

        mid = (max + min) / 2.;
        maxAmpl = 2. * config.amplitude / (max - min);
        for (uint32_t y = 0; y < ny; ++y) {
            for (uint32_t x = 0; x < nx; ++x) {
                double norm2 = arr[x][y].imag() * sgn(x, y) - mid;
                arr[x][y].imag(norm2 * maxAmpl);
            }
        }
    }
}

HqNoise2D::HqNoise2D(const HqNoiseConfig& iconfig) :
        config { iconfig }, arr { iconfig.maxN * iconfig.accuracy + 2, iconfig.maxN * iconfig.accuracy + 2, sizeof(std::complex<double>) } {

    unsigned int nx = config.maxN * config.accuracy + 2;
    unsigned int ny = config.maxN * config.accuracy + 2;
    basic::RandomUniDist distribution(0, 100.0, config.seed);

// we should construct plan before initialization of array, otherwise it is wipe out during probing testing wisdom
    fftwpp::fft2d Forward(-1, arr);
    fftwpp::fft2d Backward(1, arr);

    for (unsigned int i = 0; i < nx; i++) {
        for (unsigned int j = 0; j < ny; j++) {
            double x = distribution.get();
            if (config.complex) {
                arr(i, j) = std::complex<double>(x * sgn(i, j), distribution.get() * sgn(i, j));
            } else {
                arr(i, j) = std::complex<double>(x * sgn(i, j), 0);

            }
        }
    }

    Forward.fft(arr);

    double power = config.powerlaw.power * .5;
    double pfreq = config.powerlaw.frequency;
    double quot = pfreq * pfreq;

    double gfreq = config.gaussian.frequency;
    double gaussVariance = 2. * gfreq * gfreq;

    //cut freq 0
    arr[nx / 2][ny / 2] = 0;

    for (unsigned int i = 0; i < nx; i++) {
        for (unsigned int j = 0; j < ny; j++) {
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
    Backward.fft(arr);
    normalize(nx, ny);
}

double HqNoise1D::sgn(int val) {
    return ((val) % 2) * 2 - 1;
}

void HqNoise1D::normalize(unsigned int nx) {
    double max = std::numeric_limits<double>::min();
    double min = std::numeric_limits<double>::max();
    for (uint32_t i = 0; i < nx; ++i) {
        double value = sgn(i) * arr[i].real();
        max = std::max(max, value);
        min = std::min(min, value);
    }
    double mid = (max + min) / 2.;
    double maxAmpl = 2. * config.amplitude / (max - min);
    for (uint32_t x = 0; x < nx; ++x) {
        double norm2 = arr[x].real() * sgn(x) - mid;
        arr[x].real(norm2 * maxAmpl);
    }
    if (config.complex) {
        double max = std::numeric_limits<double>::min();
        double min = std::numeric_limits<double>::max();
        for (uint32_t i = 0; i < nx; ++i) {
            double value = sgn(i) * arr[i].imag();
            max = std::max(max, value);
            min = std::min(min, value);
        }
        double mid = (max + min) / 2.;
        double maxAmpl = 2. * config.amplitude / (max - min);
        for (uint32_t x = 0; x < nx; ++x) {
            double norm2 = arr[x].imag() * sgn(x) - mid;
            arr[x].imag(norm2 * maxAmpl);
        }
    }
}

HqNoise1D::HqNoise1D(const HqNoiseConfig& iconfig) :
        config { iconfig }, arr { config.maxN * config.accuracy + 2, sizeof(std::complex<double>) } {

    unsigned int nx = config.maxN * config.accuracy + 2;

    std::default_random_engine generator(config.seed);
    std::uniform_real_distribution<double> distribution(0, 100.0);
// we should construct plan before initialization of array, otherwise it is wipe out during probing testing wisdom
    fftwpp::fft1d Forward(-1, arr);
    fftwpp::fft1d Backward(1, arr);

    for (unsigned int i = 0; i < nx; i++) {
        double x = distribution(generator);
        if (config.complex) {
            arr(i) = std::complex<double>(x * sgn(i), distribution(generator) * sgn(i));
        } else {
            arr(i) = std::complex<double>(x * sgn(i), 0);

        }
    }

    Forward.fft(arr);

    //cut freq 0
    arr[nx / 2.] = 0;

    double power = config.powerlaw.power;
    double pfreq = config.powerlaw.frequency;

    double gfreq = config.gaussian.frequency;
    double gaussVariance = 2. * gfreq * gfreq;

    for (unsigned int i = 0; i < nx; i++) {
        double xx = static_cast<double>(i) - nx / 2.;
        double dquot = std::abs(xx) / pfreq;
        if (dquot >= 1.) {
            arr[i] *= pow(dquot, -power);
        }
        if (gaussVariance > 0) {
            arr[i] *= exp(-(xx * xx) / gaussVariance);
        }

    }

    Backward.fft(arr);

    normalize(nx);
}
double HqNoise1D::get(double x) const {
    double xx = x * config.accuracy;
    int x0 = HqNoiseUtils::fastFloor(xx);
    int x1 = x0 + 1;

    double xs = xx - (double) x0;

    double xf0 = HqNoiseUtils::lerp(arr[x0].real(), arr[x1].real(), xs);

    return xf0;

}

std::complex<double> HqNoise1D::getComplex(double x) const {
    double xx = x * config.accuracy;
    int x0 = HqNoiseUtils::fastFloor(xx);
    int x1 = x0 + 1;

    double xs = xx - (double) x0;

    return HqNoiseUtils::lerpC(arr[x0], arr[x1], xs);

}
} /* namespace generator */
} /* namespace laby */

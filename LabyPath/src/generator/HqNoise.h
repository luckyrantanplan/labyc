/*
 * HqNoise.h
 *
 *  Created on: Nov 12, 2017
 *      Author: florian
 */

#ifndef HQNOISE_H_
#define HQNOISE_H_

#include <bits/stdint-uintn.h>
#include <algorithm>
#include <complex>
#include <random>

#include "../fft/Array.h"

namespace laby {
namespace generator {
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

    static int fastFloor(double f);
    static double interpLinear(double t);
    static double lerp(double a, double b, double t);
    static std::complex<double> lerpC(std::complex<double>& a, std::complex<double>& b, double t);
};

class HqNoise1D {
public:
    const HqNoiseConfig config;
    Array::array1<std::complex<double>> arr;
    static double sgn(int val);
    explicit HqNoise1D(const HqNoiseConfig& config);

    double get(double x) const;
    std::complex<double> getComplex(double x) const;

private:
    void normalize(unsigned int nx);
};

class HqNoise2D {
public:
    const HqNoiseConfig config;
    Array::array2<std::complex<double>> arr;

    static double sgn(int x, int y);

    explicit HqNoise2D(const HqNoiseConfig& config);

    double get(const double x, const double y) const;
    std::complex<double> getComplex(const double x, const double y) const;
    std::complex<double> getComplex(const std::complex<double>& c) const;

private:
    void normalize(unsigned int nx, unsigned int ny);
};
} /* namespace generator */
} /* namespace laby */

#endif /* HQNOISE_H_ */

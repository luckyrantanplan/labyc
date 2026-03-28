/*
 * ffttest.cpp
 *
 *  Created on: Nov 10, 2017
 *      Author: florian
 */

#include <cairomm/context.h>
#include <cairomm/enums.h>
#include <cairomm/refptr.h>
#include <cairomm/surface.h>
#include <algorithm>
#include <complex>
#include <cstdint>
#include <iostream>
#include <limits>
#include <random>

#include "Fenetre.h"
#include "HqNoise.h"

/* namespace laby */

Cairo::RefPtr<Cairo::ImageSurface> getNoiseSurface(const laby::HqNoise2D& hqNoise2D) {
    const laby::generator::HqNoiseConfig& config = hqNoise2D.config();

    Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::Format::FORMAT_RGB24, config.maxN, config.maxN);
    unsigned char* data = surface->get_data();
    int stride = surface->get_stride();
    for (uint32_t y = 0; y < config.maxN; ++y) {
        unsigned char* row = data + y * stride;
        for (uint32_t x = 0; x < config.maxN; ++x) {
            double valBlue = hqNoise2D.get(static_cast<double>(x), static_cast<double>(y)) + config.amplitude + 20;
            row[x * 4] = valBlue;
            row[x * 4 + 1] = valBlue;
            row[x * 4 + 2] = valBlue;
        }
    }
    return surface;
}

int graypenmain(int argc, char *argv[]) {

    laby::HqNoiseConfig config;

    config.maxN = 800;
    config.seed = 4;
    config.amplitude = 2.;
    config.accuracy = 1;
    config.gaussian.frequency = 800;
    config.powerlaw.frequency = 10;
    config.powerlaw.power = 2;
    config.complex = true;

    laby::HqNoise1D hqNoise1D(config);

    config.maxN = 800;
    config.seed = 4;
    config.amplitude = 40.;
    config.accuracy = 1;
    config.gaussian.frequency = 800;
    config.powerlaw.frequency = 50;
    config.powerlaw.power = 2;
    config.complex = false;

    laby::HqNoise2D hqNoise2D(config);
    return Fenetre::main(argc, argv, [& ](const Cairo::RefPtr<Cairo::Context>& cr) {

        cr->set_source( getNoiseSurface( hqNoise2D),0,0);
        cr->set_line_width(1);

        cr->scale(1., 1.);
        cr->move_to( 20 , 200 );
        for (uint32_t x=20;x<config.maxN-20; ++x) {

            std::complex<double> c = hqNoise1D.getComplex(x );

            cr->line_to( x , c.imag()+200.-(c.real()*0.5+5) );

        }

        for (uint32_t x=20;x<config.maxN-20; ++x) {

            uint32_t xx=config.maxN-x;
            std::complex<double> c = hqNoise1D.getComplex(xx );

            cr->line_to( xx , c.imag()+200.+(c.real()*0.5+5) );

        }
        cr->close_path();
        cr->fill();
        return true;
    });
}


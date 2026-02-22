/*
 * Goutte.cpp
 *
 *  Created on: Oct 27, 2017
 *      Author: florian
 */

#include "Goutte.h"

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <cmath>
#include <iostream>

#include "Fenetre.h"

Goutte::Goutte() {
    // TODO Auto-generated constructor stub

}

Goutte::~Goutte() {
    // TODO Auto-generated destructor stub
}

struct Pinceau {

    double theta, phi, vtheta, vphi;

    double g, l;

    Pinceau(double theta, double phi, double vtheta, double vphi, double g, double l) :
            theta { theta }, phi { phi }, vtheta { vtheta }, vphi { vphi }, g { g }, l { l } {

    }

    void nextPos(double deltaT) {

        double sinT = std::sin(theta);

        double acceltheta = -g / l * sinT + sinT * cos(theta) * vphi * vphi;
        vtheta += acceltheta * deltaT;

        vtheta *= (1 - 0.01 * deltaT);

        theta += vtheta * deltaT;
        vphi = sinT * sinT * vphi / (std::sin(theta) * std::sin(theta));

        vphi *= (1 - 0.01 * deltaT);
        phi += vphi * deltaT;

    }

    double getX() {
        return l * std::sin(theta) * std::cos(phi);
    }
    double getY() {
        return l * std::sin(theta) * std::sin(phi);
    }

};

int Gouttemain(int argc, char *argv[]) {
    return Fenetre::main(argc, argv, [ ](const Cairo::RefPtr<Cairo::Context>& cr) {

        cr->set_line_width(1);

        double sc=300.;

        cr->scale(sc, sc);
        cr->translate(1, 1);

        Pinceau p(1.3,0.7,-0.4,1.2,1,1);

        double deltaT=0.03;

        for (long i=0;i<600/deltaT;++i) {

            cr->move_to(p.getX(),p.getY());

            for (int j=0;j<2;++j) {
                p.nextPos(deltaT/2.);
            }
            cr->line_to(p.getX(),p.getY());
            //  std::cout << x << " " << y << '\n';
        }
        cr->scale(1./sc, 1./sc);
        cr->set_line_width(1);
        cr->stroke();

        return true;

    });
}

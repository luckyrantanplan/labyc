/*
 * ffttest.cpp
 *
 *  Created on: Nov 10, 2017
 *      Author: florian
 */

#include "GlobalRouter.h"

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <OutputGeneration.h>
#include <Route.h>
#include <src/spdlog/common.h>
#include <algorithm>
#include <string>

#include "Fenetre.h"

/* namespace laby */

inline double srgb_to_linear(double srgb) {
    auto linear = srgb;
    if (linear <= 0.4045) {
        linear = linear / 12.92;
    } else {
        linear = std::pow((linear + 0.055) / 1.055, 2.4);
    }
    return linear;
}

// Converts a linear value in the range [0, 1] to an sRGB value in
// the range [0, 255].
inline double linear_to_srgb(double linear) {
    double srgb;
    if (linear <= 0.0031308) {
        srgb = linear * 12.92;
    } else {
        srgb = 1.055 * std::pow(linear, 1.0 / 2.4) - 0.055;
    }
    return srgb;
}

void addMultiplePin(NTHUR::RoutingRegion& rr, int xstart, int xend, int ymin, int ymax, int space) {

    rr.setNetNumber(ymax - ymin);

    int serial = 0;
    for (int y = ymin; y <= ymax; y += space) {
        rr.beginAddANet(std::to_string(serial), serial, 2, 1);
        rr.addPin(xstart, y, 1);
        rr.addPin(y, xend, 1);
        rr.endAddANet();
        ++serial;
    }

}

void addMultiplePin(NTHUR::RoutingRegion& rr, const std::vector<NTHUR::Coordinate_3d>& v) {

    rr.setNetNumber(v.size() / 2);

    int serial = 0;
    for (uint32_t i = 0; i + 1 < v.size(); i += 2) {
        rr.beginAddANet(std::to_string(serial), serial, 2, 1);
        rr.addPin(v[i].x, v[i].y, v[i].z);
        rr.addPin(v[i + 1].x, v[i + 1].y, v[i + 1].z);
        rr.endAddANet();
        ++serial;
    }

}

void addRect(NTHUR::RoutingRegion& rr, int x, int y, std::vector<NTHUR::Coordinate_3d>& pins) {
    Rectangle rect(x, 5, y, 5, 0);
    std::vector<NTHUR::Coordinate_3d> v = rect.getPin(1);
    pins.insert(pins.end(), v.begin(), v.end());
    rect.adjustEdgeCapacity(rr, 0, 1, 0);
    rect.adjustEdgeCapacity(rr, 0, 1, 1);
    rect.adjustEdgeCapacity(rr, 0, 1, 2);
}

int mainGlobalRouter(int argc, char *argv[]) {

    std::random_device rd { };
    std::mt19937 gen { rd() };
    std::uniform_real_distribution<> dis { 0.0, 1.0 };

    return Fenetre::main(argc, argv, [& ](const Cairo::RefPtr<Cairo::Context>& cr) {

        NTHUR::Route router;
        int max=60;
        NTHUR::RoutingRegion rr(max+1, max+1, 3);
        rr.setVerticalCapacity(0, 2);
        rr.setHorizontalCapacity(0, 2);
        rr.setVerticalCapacity(1, 2);
        rr.setHorizontalCapacity(1, 2);
        rr.setVerticalCapacity(2, 2);
        rr.setHorizontalCapacity(2, 2);

        std::vector<NTHUR::Coordinate_3d > v;

        int space=12;
        for (int x=5;x<max-space; x+=space) {
            for (int y=5;y<max-space;y+=space) {
                addRect(rr,x,y,v);
            }
        }

        //Rectangle r (0, 10, 0, 10, 0);
            std::shuffle(v.begin(),v.end(),gen);

            addMultiplePin(rr,v);

            NTHUR::OutputGeneration output(router.process(rr,spdlog::level::info));

            NTHUR::OutputGeneration::Comb comb(output.combAllNet());

            double zoom=22.;
            cr->scale(zoom, zoom);
            cr->translate(1,1);

            double layer_dist=0.3;

            for (const std::vector<NTHUR::Segment3d>& v : comb) {

                double r=dis(gen)*0.6;
                double b=dis(gen)*0.6;
                double g=(1.1)-r-b;
                cr->set_source_rgb(linear_to_srgb(r),linear_to_srgb(g),linear_to_srgb(b));

                for (const NTHUR::Segment3d& s: v) {
                    cr->move_to( s.first.x+layer_dist*s.first.z , s.first.y +layer_dist*s.first.z);
                    cr->line_to( s.last .x +layer_dist*s.last.z, s.last.y +layer_dist*s.last.z);
                }cr->scale(1./zoom, 1./zoom);
                cr->set_line_width(1);
                cr->stroke();
                cr->scale(zoom, zoom);
            }

            return true;
        });
}


/*
 * bezier.cpp
 *
 *  Created on: Oct 5, 2017
 *      Author: florian
 */

// Constructing an arrangement of Bezier curves.
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <iostream>
#include <list>
#include <vector>
#include <utility>

#ifndef CGAL_USE_CORE

int main ()
{
    std::cout << "Sorry, this example needs CORE ..." << std::endl;
    return 0;
}
#else


#include <CGAL/Cartesian.h>
#include <CGAL/CORE_algebraic_number_traits.h>
#include <CGAL/Arr_Bezier_curve_traits_2.h>
#include <CGAL/Arrangement_2.h>
using Nt_traits = CGAL::CORE_algebraic_number_traits;
using NT = Nt_traits::Rational;
using Rational = Nt_traits::Rational;
using Algebraic = Nt_traits::Algebraic;
using Rat_kernel = CGAL::Cartesian<Rational>;
using Alg_kernel = CGAL::Cartesian<Algebraic>;
using Rat_point_2 = Rat_kernel::Point_2;
using Traits_2 = CGAL::Arr_Bezier_curve_traits_2<Rat_kernel, Alg_kernel, Nt_traits>;
using Point_2 = Traits_2::Point_2;
using Bezier_curve_2 = Traits_2::Curve_2;
using Arrangement_2 = CGAL::Arrangement_2<Traits_2>;


auto main3(int  /*argc*/, char * /*argv*/[]) -> int {

    std::list<Bezier_curve_2> curves;

    std::vector<Point_2> v { { 0, 0 }, { 200, 200 }, { 100, 0 }, { 0, 100 } };

    Bezier_curve_2 const b(v.begin(), v.end());

    curves.push_back(b);
    std::cout << "B = {" << b << "}" << '\n';

    // Construct the arrangement.
    Arrangement_2 arr;
    insert(arr, curves.begin(), curves.end());

    for (Arrangement_2::Face_const_iterator f = arr.faces_begin(); f != arr.faces_end(); ++f) {

        for (Arrangement_2::Inner_ccb_const_iterator ccbIt = f->inner_ccbs_begin(); ccbIt != f->inner_ccbs_end(); ++ccbIt) {
            Arrangement_2::Ccb_halfedge_const_circulator const ccb = *ccbIt;
            Arrangement_2::Ccb_halfedge_const_circulator current = ccb;

            do {
                Arrangement_2::Halfedge_const_handle const he =current;
                std::pair<double, double> const pair1= he->curve().parameter_range();


                std::cout << "pair " << pair1.first << ";" << pair1.second << "\n";

            } while (++current != ccb);

        }
    }

    // Print the arrangement size.
    std::cout << "The arrangement size:" << '\n' << "   V = " << arr.number_of_vertices() << ",  E = " << arr.number_of_edges() << ",  F = " << arr.number_of_faces() << '\n';
    return 0;
}
#endif


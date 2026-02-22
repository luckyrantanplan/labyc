/*
 * bezier.cpp
 *
 *  Created on: Oct 5, 2017
 *      Author: florian
 */

// Constructing an arrangement of Bezier curves.
#include <CGAL/basic.h>
#include <iostream>

#ifndef CGAL_USE_CORE

int main ()
{
    std::cout << "Sorry, this example needs CORE ..." << std::endl;
    return 0;
}
#else

#include <CGAL/CORE/Gmp.h>

#include <CGAL/Cartesian.h>
#include <CGAL/CORE_algebraic_number_traits.h>
#include <CGAL/Arr_Bezier_curve_traits_2.h>
#include <CGAL/Arrangement_2.h>
typedef CGAL::CORE_algebraic_number_traits Nt_traits;
typedef Nt_traits::Rational NT;
typedef Nt_traits::Rational Rational;
typedef Nt_traits::Algebraic Algebraic;
typedef CGAL::Cartesian<Rational> Rat_kernel;
typedef CGAL::Cartesian<Algebraic> Alg_kernel;
typedef Rat_kernel::Point_2 Rat_point_2;
typedef CGAL::Arr_Bezier_curve_traits_2<Rat_kernel, Alg_kernel, Nt_traits> Traits_2;
typedef Traits_2::Point_2 Point_2;
typedef Traits_2::Curve_2 Bezier_curve_2;
typedef CGAL::Arrangement_2<Traits_2> Arrangement_2;

#include <CGAL/Exact_rational.h>

int main3(int argc, char *argv[]) {

    std::list<Bezier_curve_2> curves;

    std::vector<Point_2> v { { 0, 0 }, { 200, 200 }, { 100, 0 }, { 0, 100 } };

    Bezier_curve_2 B(v.begin(), v.end());

    curves.push_back(B);
    std::cout << "B = {" << B << "}" << std::endl;

    // Construct the arrangement.
    Arrangement_2 arr;
    insert(arr, curves.begin(), curves.end());

    for (Arrangement_2::Face_const_iterator f = arr.faces_begin(); f != arr.faces_end(); ++f) {

        for (Arrangement_2::Inner_ccb_const_iterator ccbIt = f->inner_ccbs_begin(); ccbIt != f->inner_ccbs_end(); ++ccbIt) {
            Arrangement_2::Ccb_halfedge_const_circulator ccb = *ccbIt;
            Arrangement_2::Ccb_halfedge_const_circulator current = ccb;

            do {
                Arrangement_2::Halfedge_const_handle he =current;
                std::pair<double, double> pair1= he->curve().parameter_range();


                std::cout << "pair " << pair1.first << ";" << pair1.second << "\n";

            } while (++current != ccb);

        }
    }

    // Print the arrangement size.
    std::cout << "The arrangement size:" << std::endl << "   V = " << arr.number_of_vertices() << ",  E = " << arr.number_of_edges() << ",  F = " << arr.number_of_faces() << std::endl;
    return 0;
}
#endif


/*
 * ex_triangle.cpp
 *
 *  Created on: Oct 2, 2017
 *      Author: florian
 */

// File : ex_triangle . cpp
#include <CGAL/Arrangement_2/Arrangement_on_surface_2_global.h>
#include <CGAL/Cartesian.h>
#include <CGAL/Arr_non_caching_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <array>
#include <iostream>
using Number_type = int;
using Kernel = CGAL::Cartesian<Number_type>;
using Traits = CGAL::Arr_non_caching_segment_traits_2<Kernel>;
using Point = Traits::Point_2;
using Segment = Traits::X_monotone_curve_2;
using Arrangement = CGAL::Arrangement_2<Traits>;



template<typename Arrangement>
void printIncidentHalfedges(typename Arrangement::Vertex_const_handle& v) {
    if (v->is_isolated()) {
        std::cout << "The vertex ( " << v->point() << " ) is isolated " << std::endl;
        return;
    }
    std::cout << "The neighbors of the vertex ( " << v->point() << " ) are : ";
    typename Arrangement::Halfedge_around_vertex_const_circulator first;
    typename Arrangement::Halfedge_around_vertex_const_circulator curr;
    first = curr = v->incident_halfedges();

    do {
        std::cout << "( " << curr->source()->point() << " ) ";
    } while (++curr != first);
    std::cout << '\n';

}

template<typename Arrangement>
void printCcb(typename Arrangement::Ccb_halfedge_const_circulator circ) {
    std::cout << " (" << circ->source()->point() << " ) ";
    typename Arrangement::Ccb_halfedge_const_circulator curr = circ;
    do {
        typename Arrangement::Halfedge_const_handle const he = curr;
        std::cout << "␣␣␣ [ " << he->curve() << " ] ␣␣␣" << " ( " << he->target()->point() << " ) ";
    } while (++curr != circ);
    std::cout << '\n';
}

auto mainold() -> int{
    Point p1(1, 1);
    Point p2(1, 2);
    Point p3(2, 1);

    std::array<Segment, 3> cv = { Segment(p1, p2), Segment(p2, p3), Segment(p3, p1) };
    Arrangement arr;
    CGAL::insert(arr, cv.begin(), cv.end());
    std::cout << "Number of  faces : " << arr.number_of_faces() << '\n';

    std::cout << "Number of  vertices : " << arr.number_of_vertices() << '\n';

    std::cout << "Number of  halfedges : " << arr.number_of_halfedges() << '\n';

    for (Arrangement::Vertex_const_iterator it = arr.vertices_begin(); it != arr.vertices_end(); ++it) {
        printIncidentHalfedges<Arrangement>(it);
    }

    Arrangement::Ccb_halfedge_const_circulator const ccb = arr.halfedges_begin()->ccb();
    printCcb<Arrangement>(ccb);
    return 0;
}

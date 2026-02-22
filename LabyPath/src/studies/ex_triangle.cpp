/*
 * ex_triangle.cpp
 *
 *  Created on: Oct 2, 2017
 *      Author: florian
 */

// File : ex_triangle . cpp
#include <CGAL/Cartesian.h>
#include <CGAL/Arr_non_caching_segment_traits_2.h>
#include <CGAL/Arrangement_2.h>
#include <array>
typedef int Number_type;
typedef CGAL::Cartesian<Number_type> Kernel;
typedef CGAL::Arr_non_caching_segment_traits_2<Kernel> Traits;
typedef Traits::Point_2 Point;
typedef Traits::X_monotone_curve_2 Segment;
typedef CGAL::Arrangement_2<Traits> Arrangement;



template<typename Arrangement>
void print_incident_halfedges(typename Arrangement::Vertex_const_handle& v) {
    if (v->is_isolated()) {
        std::cout << "The vertex ( " << v->point() << " ) is isolated " << std::endl;
        return;
    }
    std::cout << "The neighbors of the vertex ( " << v->point() << " ) are : ";
    typename Arrangement::Halfedge_around_vertex_const_circulator first, curr;
    first = curr = v->incident_halfedges();

    do
        std::cout << "( " << curr->source()->point() << " ) ";
    while (++curr != first);
    std::cout << std::endl;

}

template<typename Arrangement>
void print_ccb(typename Arrangement::Ccb_halfedge_const_circulator circ) {
    std::cout << " (" << circ->source()->point() << " ) ";
    typename Arrangement::Ccb_halfedge_const_circulator curr = circ;
    do {
        typename Arrangement::Halfedge_const_handle he = curr;
        std::cout << "␣␣␣ [ " << he->curve() << " ] ␣␣␣" << " ( " << he->target()->point() << " ) ";
    } while (++curr != circ);
    std::cout << std::endl;
}

int mainold(){
    Point p1(1, 1), p2(1, 2), p3(2, 1);

    std::array<Segment, 3> cv = { Segment(p1, p2), Segment(p2, p3), Segment(p3, p1) };
    Arrangement arr;
    CGAL::insert(arr, cv.begin(), cv.end());
    std::cout << "Number of  faces : " << arr.number_of_faces() << std::endl;

    std::cout << "Number of  vertices : " << arr.number_of_vertices() << std::endl;

    std::cout << "Number of  halfedges : " << arr.number_of_halfedges() << std::endl;

    for (Arrangement::Vertex_const_iterator it = arr.vertices_begin(); it != arr.vertices_end(); ++it) {
        print_incident_halfedges<Arrangement>(it);
    }

    Arrangement::Ccb_halfedge_const_circulator ccb = arr.halfedges_begin()->ccb();
    print_ccb<Arrangement>(ccb);
    return 0;
}

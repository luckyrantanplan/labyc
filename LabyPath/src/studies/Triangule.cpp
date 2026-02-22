/*
 * triangule.cpp
 *
 *  Created on: Oct 16, 2017
 *      Author: florian
 */

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <CGAL/Constrained_Delaunay_triangulation_2.h>
#include <CGAL/Constrained_triangulation_2.h>
#include <CGAL/Delaunay_mesh_face_base_2.h>
#include <CGAL/Delaunay_mesh_size_criteria_2.h>
#include <CGAL/Delaunay_mesh_vertex_base_2.h>
#include "Delaunay_mesh_vertex_base_with_info_2.h"
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/Exact_predicates_inexact_constructions_kernel.h>
#include <CGAL/Triangulation_data_structure_2.h>

#include "Fenetre.h"
#include "Triangule.h"

typedef CGAL::Exact_predicates_inexact_constructions_kernel K;

typedef CGAL::Delaunay_mesh_vertex_base_with_info_2<VertexInfo, K> Vb;
typedef CGAL::Delaunay_mesh_face_base_2<K> Fb;
typedef CGAL::Triangulation_data_structure_2<Vb, Fb> TDS;
//typedef CGAL::Exact_predicates_tag Itag;
typedef CGAL::Constrained_Delaunay_triangulation_2<K, TDS/*, Itag*/> CDT;
typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Criteria;
typedef CGAL::Delaunay_mesher_2<CDT, Criteria> Mesher;

typedef CDT::Point Point;

int mainTriangule(int argc, char *argv[]) {

    Triangule<CDT> t;

    return Fenetre::main(argc, argv, [&t](const Cairo::RefPtr<Cairo::Context>& cr) {
        return t.on_draw(cr);});
}


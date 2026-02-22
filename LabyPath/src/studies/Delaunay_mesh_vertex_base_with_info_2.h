/*
 * Delaunay_mesh_vertex_base_with_info_2.h
 *
 *  Created on: Nov 23, 2017
 *      Author: florian
 */

#ifndef DELAUNAY_MESH_VERTEX_BASE_WITH_INFO_2_H_
#define DELAUNAY_MESH_VERTEX_BASE_WITH_INFO_2_H_

#include <CGAL/Triangulation_vertex_base_2.h>

namespace CGAL {

template<typename Info_, typename GT, typename Vb = Triangulation_vertex_base_2<GT> >
class Delaunay_mesh_vertex_base_with_info_2: public Vb {
    Info_ _info;

public:
    typedef typename GT::FT FT;
    typedef typename Vb::Face_handle Face_handle;
    typedef typename Vb::Point Point;
    typedef Info_ Info;

    template<typename TDS2>
    struct Rebind_TDS {
        typedef typename Vb::template Rebind_TDS<TDS2>::Other Vb2;
        typedef Delaunay_mesh_vertex_base_with_info_2<Info, GT, Vb2> Other;
    };
protected:
    FT sizing_info_;

public:
    Delaunay_mesh_vertex_base_with_info_2() :
            Vb(), sizing_info_(0.) {
    }

    Delaunay_mesh_vertex_base_with_info_2(const Point & p) :
            Vb(p), sizing_info_(0.) {
    }

    Delaunay_mesh_vertex_base_with_info_2(const Point & p, Face_handle c) :
            Vb(p, c), sizing_info_(0.) {
    }

    Delaunay_mesh_vertex_base_with_info_2(Face_handle c) :
            Vb(c), sizing_info_(0.) {
    }

    void set_sizing_info(const FT& s) {
        sizing_info_ = s;
    }
    const FT& sizing_info() const {
        return sizing_info_;
    }

    const Info& info() const {
        return _info;
    }
    Info& info() {
        return _info;
    }
};

} // namespace CGAL

#endif /* DELAUNAY_MESH_VERTEX_BASE_WITH_INFO_2_H_ */

/*
 * AlternateRoute.h
 *
 *  Created on: Aug 21, 2018
 *      Author: florian
 */

#ifndef ALTERNAROUTE_ALTERNATEROUTE_H_
#define ALTERNAROUTE_ALTERNATEROUTE_H_

#include "../GeomData.h"
#include "../Ribbon.h"
#include "../GridIndex.h"
#include "StrokeArrangement.h"
#include "../protoc/AllConfig.pb.h"

namespace laby {

class AlternateRoute {

    struct Offset_pair {
        Kernel::Point_2 origin;
        Kernel::Point_2 offset;

        void print(std::ostream& os) const {

            os << " origin " << origin;
            os << " offset " << offset;

        }
        static std::vector<Offset_pair> simplify(std::vector<Offset_pair>& list, const double& dist);
    };

public:

    AlternateRoute(const proto::AlternateRouting& configfilepaths, const proto::Filepaths& filepaths);

private:
    const proto::AlternateRouting _config;
    double sq_max_thickness;
    double sq_min_thickness;

    Arrangement_2 prune_arrangement(const Arrangement_2& arr);
    Arrangement_2 remove_antenna(const Arrangement_2& arr);
    void add_point(std::vector<Offset_pair>& left, const Kernel::Point_2& o1, const Kernel::Point_2& o2);
    std::vector<Offset_pair> couple_list(const Halfedge& he, int32_t direction);
    void add_triplet(alter::Offset_triplet& triplet, //
            const Offset_pair& a, const Kernel::Point_2& b, const Kernel::Point_2& c);

    Arrangement_2 voronoi_arr(const Arrangement_2& arr, const int32_t& direction, const CGAL::Bbox_2& viewBox, const Ribbon& ribLimit);
    std::vector<alter::Offset_triplet> create_triplet_list(const Halfedge& he, const int32_t& direction);
    void populateTrapeze(const GridIndex& gridIndex, const std::vector<Ribbon>& ribList, const CGAL::Bbox_2& viewBox, std::vector<alter::Segment_trapeze_info_2>& trapeze_vect);
    void ribToTrapeze(const Ribbon& rib, std::vector<alter::Segment_trapeze_info_2>& trapeze_vect, //
            const Arrangement_2& arr, const CGAL::Bbox_2& viewBox, const Ribbon& ribLimit);

};

} /* namespace laby */

#endif /* ALTERNAROUTE_ALTERNATEROUTE_H_ */

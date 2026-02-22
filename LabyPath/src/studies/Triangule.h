/*
 * Triangule.h
 *
 *  Created on: Oct 20, 2017
 *      Author: florian
 */

#ifndef TRIANGULE_H_
#define TRIANGULE_H_

#include <cairomm/context.h>
#include <cairomm/refptr.h>
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
#include <CGAL/Delaunay_mesher_2.h>
#include <CGAL/lloyd_optimize_mesh_2.h>
#include "Fenetre.h"
#include "PoissonGenerator.h"
#include "ortools/graph/min_cost_flow.h"

template<typename P>
struct Point2d {
    double _x;
    double _y;
    Point2d(double x, double y) :
            _x(x), _y(y) {
    }

    Point2d() :
            _x(), _y() {
    }
    void set(double x, double y) {
        _x = x;
        _y = y;
    }
    void set(Point2d p) {
        _x = p._x;
        _y = p._y;
    }
    P get() {
        return P(_x, _y);
    }
};

struct VertexInfo {

    enum VertexType {
        SOURCE, TARGET, NORMAL
    };

    VertexType type;
    int32_t number;

    VertexInfo() :
            type { NORMAL } {
        static u_int32_t number_counter = 0;
        number = number_counter;
        ++number_counter;
    }
};

template<typename CDT>
struct ArcInfo {
    typedef typename CDT::Vertex_handle Vh;
    const Vh first;
    const Vh second;
    const int32_t arcIndex;

    ArcInfo(const Vh first, const Vh second, const int32_t arcIndex) :
            first { first }, second { second }, arcIndex { arcIndex } {
    }

};

template<typename CDT>
class Triangule {
public:
    void draw_arr(std::vector<ArcInfo<CDT>> arr, const Cairo::RefPtr<Cairo::Context>& cr, operations_research::SimpleMinCostFlow& min_cost_flow) {

        for (const ArcInfo<CDT>& arc : arr) {

            if (min_cost_flow.Flow(arc.arcIndex) > 0) {

                cr->set_source_rgb(1., 0, 0);
            } else {

                cr->set_source_rgb(0.6, 0.6, 0.6);
            }
            typename CDT::Point p = arc.first->point();

            cr->move_to(static_cast<double>(p.x()), static_cast<double>(p.y()));
            typename CDT::Point p2 = arc.second->point();

            cr->line_to(static_cast<double>(p2.x()), static_cast<double>(p2.y()));
            cr->stroke();
        }
    }

public:
//Override default signal handler:
    bool on_draw(const Cairo::RefPtr<Cairo::Context>& cr) {

        CDT cdt;

//        Point2d<typename CDT::Point> a(0, 0);
//        Point2d<typename CDT::Point> b(800, 0);
//        Point2d<typename CDT::Point> c(800, 800);
//        Point2d<typename CDT::Point> d(0, 800);
//
//        cdt.insert_constraint(a.get(), b.get());
//        cdt.insert_constraint(b.get(), c.get());
//        cdt.insert_constraint(c.get(), d.get());
//        cdt.insert_constraint(d.get(), a.get());

        circle(cdt, 480., 400, 400);

        double radius = 40;

        circle(cdt, radius, 100, 100, VertexInfo::SOURCE);

        circle(cdt, radius, 500, 90, VertexInfo::SOURCE);

        circle(cdt, radius, 98, 483, VertexInfo::TARGET);
        circle(cdt, radius, 502, 504, VertexInfo::TARGET);

        assert(cdt.is_valid());
        std::cout << "Number of vertices before: " << cdt.number_of_vertices() << std::endl;
        typedef CGAL::Delaunay_mesh_size_criteria_2<CDT> Criteria;
        typedef CGAL::Delaunay_mesher_2<CDT, Criteria> Mesher;

        std::list<typename CDT::Point> list_of_seeds;
        list_of_seeds.push_back(typename CDT::Point(400, 10));

        Mesher mesher(cdt);
        mesher.set_criteria(Criteria(0.20, 50));
        mesher.set_seeds(list_of_seeds.begin(), list_of_seeds.end(), true);
        mesher.refine_mesh();

        std::cout << "Number of vertices: " << cdt.number_of_vertices() << std::endl;
//        std::cout << "Run Lloyd optimization...";
//        CGAL::lloyd_optimize_mesh_2(cdt, CGAL::parameters::max_iteration_number = 100);
//        std::cout << " done." << std::endl;
//        std::cout << "Number of vertices: " << cdt.number_of_vertices() << std::endl;

        operations_research::SimpleMinCostFlow min_cost_flow;

        for (typename CDT::Finite_vertices_iterator it = cdt.finite_vertices_begin(); it != cdt.finite_vertices_end(); ++it) {

            switch (it->info().type) {
            case VertexInfo::SOURCE:
                min_cost_flow.SetNodeSupply(it->info().number, 1);
                break;
            case VertexInfo::TARGET:
                min_cost_flow.SetNodeSupply(it->info().number, -1);
                break;
            case VertexInfo::NORMAL:
                break;
            }
        }

        std::vector<ArcInfo<CDT>> arcList;

        for (typename CDT::Finite_edges_iterator it = cdt.finite_edges_begin(); it != cdt.finite_edges_end(); ++it) {
            const typename CDT::Edge& edge = *it;
            int cost = 1;
            int capacity = 1;
            const typename CDT::Face_handle& fh = edge.first;
            if (fh->is_in_domain()) {
                int i = edge.second;
                typename CDT::Vertex_handle firstVertex = fh->vertex(CDT::cw(i));
                u_int32_t first = firstVertex->info().number;
                typename CDT::Vertex_handle secondVertex = fh->vertex(CDT::ccw(i));
                u_int32_t second = secondVertex->info().number;

                operations_research::ArcIndex ai = min_cost_flow.AddArcWithCapacityAndUnitCost(first, second, capacity, cost);
                arcList.push_back(ArcInfo<CDT>(firstVertex, secondVertex, ai));
                operations_research::ArcIndex bi = min_cost_flow.AddArcWithCapacityAndUnitCost(second, first, capacity, cost);
                arcList.push_back(ArcInfo<CDT>(secondVertex, firstVertex, bi));

            }
        }

        operations_research::MinCostFlowBase::Status solve = min_cost_flow.Solve();
        int64_t cost = min_cost_flow.OptimalCost();

        std::cout << solve << " cost is " << cost << std::endl;
        cr->set_line_width(1);

        cr->scale(1., 1.);
        cr->translate(80., 80.);

        draw_arr(arcList, cr, min_cost_flow);
        cr->scale(1, 1);
        cr->set_line_width(1);
        cr->stroke();

        return true;
    }

private:
    void circle(CDT& cdt, double r, double x, double y, VertexInfo::VertexType vtype = VertexInfo::NORMAL) {

        int max = static_cast<int>(std::trunc(.5 * r));

        double constante = 2. * M_PI / static_cast<double>(max);

        typedef typename CDT::Point Point2D;

        std::vector<typename CDT::Vertex_handle> listPoint;
        double angle;
        for (int i = 0; i < max; ++i) {
            angle = constante * (static_cast<double>(i) + prn.RandomFloat() / 2.);

            typename CDT::Vertex_handle vh = cdt.insert(Point2D(r * std::cos(angle) + x, r * std::sin(angle) + y));
            vh->info().type = vtype;
            listPoint.push_back(vh);

        }

        for (int i = 0; i < max; ++i) {

            cdt.insert_constraint(listPoint.at(i), listPoint.at((i + 1) % max));
        }
    }

    PoissonGenerator::DefaultPRNG prn;
}
;

#endif /* TRIANGULE_H_ */

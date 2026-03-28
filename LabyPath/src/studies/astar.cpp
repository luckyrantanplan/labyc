/*
 * astar.cpp
 *
 *  Created on: Nov 14, 2017
 *      Author: florian
 */

//
//=======================================================================
// Copyright (c) 2004 Kristopher Beevers
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//=======================================================================
//
#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/graph_selectors.hpp>
#include <boost/graph/properties.hpp>
#include <boost/graph/detail/adjacency_list.hpp>
#include <boost/graph/named_function_params.hpp>
#include <boost/graph/random.hpp>
#include <boost/pending/property.hpp>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/graphviz.hpp>
#include <cstddef>
#include <boost/tuple/tuple.hpp>
#include <boost/random/mersenne_twister.hpp>
#include <ctime>
#include <utility>
#include <vector>
#include <list>
#include <iostream>
#include <fstream>
#include <cmath>    // for sqrt

using namespace boost;
using namespace std;

// auxiliary types
struct Location {
    float y, x; // lat, long
};
using cost = float;

template<class Name, class LocMap>
class CityWriter {
public:
    CityWriter(Name n, LocMap l, float _minx, float _maxx, float _miny, float _maxy, unsigned int _ptx, unsigned int _pty) :
            _name(n), _loc(l), _minx(_minx), _maxx(_maxx), _miny(_miny), _maxy(_maxy), _ptx(_ptx), _pty(_pty) {
    }
    template<class Vertex>
    void operator()(ostream& out, const Vertex& v) const {
        float const px = 1 - (_loc[v].x - _minx) / (_maxx - _minx);
        float const py = (_loc[v].y - _miny) / (_maxy - _miny);
        out << "[label=\"" << _name[v] << "\", pos=\"" << static_cast<unsigned int>(_ptx * px) << "," << static_cast<unsigned int>(_pty * py) << R"(", fontsize="11"])";
    }
private:
    Name _name;
    LocMap _loc;
    float _minx, _maxx, _miny, _maxy;
    unsigned int _ptx, _pty;
};

template<class WeightMap>
class TimeWriter {
public:
    explicit TimeWriter(WeightMap w) :
            _wm(w) {
    }
    template<class Edge>
    void operator()(ostream &out, const Edge& e) const {
        out << "[label=\"" << _wm[e] << R"(", fontsize="11"])";
    }
private:
    WeightMap _wm;
};

// euclidean distance heuristic
template<class Graph, class CostType, class LocMap>
class DistanceHeuristic: public astar_heuristic<Graph, CostType> {
public:
    using Vertex = typename graph_traits<Graph>::vertex_descriptor;
    DistanceHeuristic(LocMap l, Vertex goal) :
            _m_location(l), _m_goal(goal) {
    }
    auto operator()(Vertex u) -> CostType {
        CostType dx = _m_location[_m_goal].x - _m_location[u].x;
        CostType dy = _m_location[_m_goal].y - _m_location[u].y;
        return ::sqrt(dx * dx + dy * dy);
    }
private:
    LocMap _m_location;
    Vertex _m_goal;
};

struct FoundGoal {
};
// exception for termination

// visitor that terminates when we find the goal
template<class Vertex>
class AstarGoalVisitor: public boost::default_astar_visitor {
public:
    explicit AstarGoalVisitor(Vertex goal) :
            _m_goal(goal) {
    }
    template<class Graph>
    void examine_vertex(Vertex u, Graph&  /*g*/) {
        if (u == _m_goal) {
            throw FoundGoal();
}
    }
private:
    Vertex _m_goal;
};

auto mainAStar(int  /*argc*/, char ** /*argv*/) -> int {

    // specify some types
    typedef adjacency_list<listS, vecS, undirectedS, no_property, property<edge_weight_t, cost> > mygraph_t;
    typedef property_map<mygraph_t, edge_weight_t>::type WeightMap;
    typedef mygraph_t::vertex_descriptor vertex;
    typedef mygraph_t::edge_descriptor edge_descriptor;
    typedef std::pair<int, int> edge;

    // specify data
    enum nodes {
        Troy, LakePlacid, Plattsburgh, Massena, Watertown, Utica, Syracuse, Rochester, Buffalo, Ithaca, Binghamton, Woodstock, NewYork, N
    };
    const char *name[] = { "Troy", "Lake Placid", "Plattsburgh", "Massena", "Watertown", "Utica", "Syracuse", "Rochester", "Buffalo", "Ithaca", "Binghamton", "Woodstock", "New York" };
    Location locations[] = { // lat/long
            { 42.73, 73.68 }, { 44.28, 73.99 }, { 44.70, 73.46 }, { 44.93, 74.89 }, { 43.97, 75.91 }, { 43.10, 75.23 }, { 43.04, 76.14 }, { 43.17, 77.61 }, { 42.89, 78.86 }, { 42.44, 76.50 }, { 42.10,
                    75.91 }, { 42.04, 74.11 }, { 40.67, 73.94 } };
    edge const edgeArray[] = { edge(Troy, Utica), edge(Troy, LakePlacid), edge(Troy, Plattsburgh), edge(LakePlacid, Plattsburgh), edge(Plattsburgh, Massena), edge(LakePlacid, Massena), edge(Massena,
            Watertown), edge(Watertown, Utica), edge(Watertown, Syracuse), edge(Utica, Syracuse), edge(Syracuse, Rochester), edge(Rochester, Buffalo), edge(Syracuse, Ithaca), edge(Ithaca, Binghamton),
            edge(Ithaca, Rochester), edge(Binghamton, Troy), edge(Binghamton, Woodstock), edge(Binghamton, NewYork), edge(Syracuse, Binghamton), edge(Woodstock, Troy), edge(Woodstock, NewYork) };
    unsigned int const numEdges = sizeof(edgeArray) / sizeof(edge);
    cost const weights[] = { // estimated travel time (mins)
            96, 134, 143, 65, 115, 133, 117, 116, 74, 56, 84, 73, 69, 70, 116, 147, 173, 183, 74, 71, 124 };

    // create graph
    mygraph_t g(N);
    WeightMap const weightmap = get(edge_weight, g);
    for (std::size_t j = 0; j < numEdges; ++j) {
        edge_descriptor e;
        bool inserted = false;
        boost::tie(e, inserted) = add_edge(edgeArray[j].first, edgeArray[j].second, g);
        weightmap[e] = weights[j];
    }

    // pick random start/goal
    boost::mt19937 gen(time(nullptr));
    vertex const start = random_vertex(g, gen);
    vertex const goal = random_vertex(g, gen);

    cout << "Start vertex: " << name[start] << '\n';
    cout << "Goal vertex: " << name[goal] << '\n';

    ofstream dotfile;
    dotfile.open("test-astar-cities.dot");
    write_graphviz(dotfile, g, CityWriter<const char **, Location*>(name, locations, 73.46, 78.86, 40.67, 44.93, 480, 400), TimeWriter<WeightMap>(weightmap));

    vector<mygraph_t::vertex_descriptor> p(num_vertices(g));
    vector<cost> d(num_vertices(g));
    try {
        // call astar named parameter interface
        astar_search_tree(g, start, DistanceHeuristic<mygraph_t, cost, Location*>(locations, goal),
                predecessor_map(make_iterator_property_map(p.begin(), get(vertex_index, g))).distance_map(make_iterator_property_map(d.begin(), get(vertex_index, g))).visitor(
                        AstarGoalVisitor<vertex>(goal)));

    } catch (FoundGoal& fg) { // found a path to the goal
        list<vertex> shortestPath;
        for (vertex v = goal;; v = p[v]) {
            shortestPath.push_front(v);
            if (p[v] == v) {
                break;
}
        }
        cout << "Shortest path from " << name[start] << " to " << name[goal] << ": ";
        auto spi = shortestPath.begin();
        cout << name[start];
        for (++spi; spi != shortestPath.end(); ++spi) {
            cout << " -> " << name[*spi];
}
        cout << '\n' << "Total travel time: " << d[goal] << '\n';
        return 0;
    }

    cout << "Didn't find a path from " << name[start] << "to" << name[goal] << "!" << '\n';
    return 0;

}


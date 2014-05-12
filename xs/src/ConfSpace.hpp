#ifndef slic3r_PointSpace_hpp
#define slic3r_PointSpace_hpp

#include <vector>
#include <set>
#include <utility>
#include "Point.hpp"
#include "Line.hpp"

namespace Slic3r {

bool zorder_compare(const Point& r, const Point& l);

class Polyline;

class ConfSpace {
public:
    typedef int idx_type;
    typedef float weight_type;
     
    struct Vertex {
        Point point;
        typedef std::map<idx_type, weight_type> edges_type;
        edges_type edges;
        // modified during search
        weight_type distance;
        idx_type parent;
        enum {White,Gray,Black} color;

        Vertex(const Point& p) : point(p) {}

        bool add_edge(idx_type to, weight_type w);
    };

    struct VertexIdxComparator
    {
        bool operator ()(const Point& a, const Point& b) { return zorder_compare(a,b); }
    };

    std::vector<Vertex> vertices;
    typedef std::map<Point, idx_type, VertexIdxComparator> index_type;
    index_type index;
    weight_type infinity;
public:
    ConfSpace();
    Vertex& operator[](idx_type idx) {return vertices[idx]; };
    std::pair<idx_type,bool> insert(const Point& val);
    idx_type find(const Point& val);

    bool insert_edge(idx_type from, idx_type to, weight_type weight);
    bool insert_edge(Point& from, Point& to, weight_type weight, bool create=true);
    
    void points(Points* p);
    void edge_lines(Lines* l);

    void points_in_range(Point& from, float range, Points* p);
    idx_type nearest_point(Point& from);

    bool path_dijkstra(idx_type from, idx_type to, Polyline* ret);
    bool path_dijkstra(const Point& from, const Point& to, Polyline* ret);

protected:
    void path_init();
};


}

#endif

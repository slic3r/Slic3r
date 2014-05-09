#ifndef slic3r_PointSpace_hpp
#define slic3r_PointSpace_hpp

#include <vector>
#include <set>
#include <utility>
#include "Point.hpp"

namespace Slic3r {

bool zorder_compare(const Point& r, const Point& l);

class ConfSpace {
public:
    typedef int idx_type;
    typedef float weight_type;
    typedef std::pair<idx_type, weight_type> edge_t;
     
    struct Vertex {
        Point point;
        std::vector<edge_t> edges;
        // modified during search
        weight_type distance;
        idx_type parrent;
        enum {White,Gray,Black} color;

        Vertex(const Point& p) : point(p) {}
    };

    struct VertexIdxComparator
    {
        bool operator ()(const Point& a, const Point& b) { return zorder_compare(a,b); }
    };

    std::vector<Vertex> vertices;
    typedef std::map<Point, idx_type, VertexIdxComparator> index_type;
    index_type index;
    
public:
    ConfSpace();
    const Vertex& operator[](idx_type idx) const {return vertices[idx]; };
    std::pair<idx_type,bool> insert (const Point& val);
    idx_type find(const Point& val);

    bool insert_edge(idx_type from, idx_type to, weight_type weight);
    bool insert_edge(Point& from, Point& to, weight_type weight, bool create=true);
    

};

}

#endif

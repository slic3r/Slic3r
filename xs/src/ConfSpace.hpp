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

static const int VERTS_PER_POLYGON=6;

class ConfSpace {
public:
    typedef int idx_type;
    typedef float weight_type;
    
    struct Poly {
        unsigned verts[VERTS_PER_POLYGON];  // vertices of polygon
        unsigned neis[VERTS_PER_POLYGON];   // neighbor indexes
        unsigned char vertCount;
        unsigned type;                      // used for costs etc.
        // pathfinding state
        enum {White,Gray,Black} color;
        Point entry_point;                  // midpoint of side where poly vas entered
        Poly* parent;
        weight_type cost;                   // cost from first point
        weight_type total;                  // cost including heuristic
    };

    struct Vertex {
        Point point;
        Vertex(const Point& p) : point(p) {}
        std::vector<idx_type> polys;        // link back to polygons
    };

    struct VertexIdxComparator
    {
        bool operator ()(const Point& a, const Point& b) { return zorder_compare(a,b); }
    };
    
    std::vector<Vertex> vertices;
    typedef std::map<Point, idx_type, VertexIdxComparator> index_type;
    index_type index;

    std::vector<Poly> polys;
    
    weight_type infinity;
public:
    ConfSpace();
    Vertex& operator[](idx_type idx) {return vertices[idx]; };
    std::pair<idx_type,bool> insert(const Point& val);
    idx_type find(const Point& val);

    void add_polygon(const Poly& poly, weight_type cost_inside);    
    void delaunay();

    void points(Points* p);
    void edge_lines(Lines* l);

    void points_in_range(Point& from, float range, Points* p);
    idx_type nearest_point(Point& from);

    idx_type find_poly(const Point& val);

    bool path_dijkstra(idx_type from, idx_type to, Polyline* ret);
    bool path_dijkstra(const Point& from, const Point& to, Polyline* ret);

protected:
    void path_init();
};


}

#endif

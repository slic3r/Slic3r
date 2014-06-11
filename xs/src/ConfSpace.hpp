#ifndef slic3r_ConfSpace_hpp
#define slic3r_ConfSpace_hpp

#include <vector>
#include <set>
#include <utility>

#include "Point.hpp"
#include "SVG.hpp"

namespace Slic3r {

class Polyline;
class P2tPoint;
class Line;

class ConfSpace {
public:
    typedef float cost_t;
    class Edge;
    class Vertex;

    struct Poly {
        Edge* edge;                    // first edge for this polygon, start of linked list
        int type;                      // polygon type, used for costs etc.
        // pathfinding state
        enum {Open,Visited,Closed,
              Path                     // for debugging
        } color;
        Point entry_point;             // midpoint of side where poly vas entered
        Poly* parent;
        cost_t cost;                   // cost from first point
        cost_t total;                  // cost including heuristic
        
        Poly() : edge(NULL), type(-1) {}
    };

    struct Edge {
        Edge* mirror;                  // edge in oposite direction, must exist even for boundary
        Edge* next;                    // next counterclockwise edge in polygon, filled even for boundary edges
        Vertex* end;                   // endpoint of this edge (start is mirror->end)
        Poly* poly;                    // polygon this edge belongs to, NULL for boundary
        double angle;                  // angle of this edge, used to manage linked lists now
        bool constrained;
        Edge(Vertex* end, double angle) : next(NULL), end(end), poly(NULL), angle(angle), constrained(false) {}
        Vertex* start() { return mirror->end; }
        Edge* prev();
        static void free_loop(Edge* start);
    };
    
    struct Vertex {
        Point point;
        Edge* edge;                    // first edge
        Vertex(const Point& p) : point(p), edge(NULL) {}
    };

    struct VertexComparator
    {
        bool operator ()(const Point& a, const Point& b);
    };
    
    typedef std::map<Point, Vertex*, VertexComparator> vertices_index_type;
    vertices_index_type vertices;

    std::vector<Poly*> polys;

    Edge* outside_loop;  // pointer to fist outer edge. We assume that vertices graph is connected

    cost_t infinity;
    // store input polygons so that all are available for triangulation (and for coloring)
    struct input_polygon_type {
        Polygon poly;
        int left, right;
        input_polygon_type(Polygon p, int left, int right) : poly(p), left(left), right(right) {};
    };
    std::vector<input_polygon_type> input_polygons;   // temporary storage of polygons for triangulation, with cost

    SVG* svg;
public:
    ConfSpace();
    ~ConfSpace();
    std::pair<Vertex*, bool> point_insert(const Point& val);
    Vertex* point_find(const Point& val);

    // store polygon for triangulation. Outer polygon must be added first
    void add_Polygon(const Polygon poly, int left=-1, int right=-1);
    void triangulate();

    // debug functions to pass usefull data to slicer
    void points(Points* p);
    void edge_lines(Lines* l);
    Vertex* vertex_nearest(const Point& from);
    Vertex* vertex_near(const Point& from);

    // find path, sraighten it and return as Polyline
    bool path(const Point& from, const Point& to, Polyline*  ret);
    // start storing data into SVG file
    void SVG_dump(const char* fname);    
    void SVG_dump_path(const char* fname, Point& from, Point& to, const Polyline& straight_path);
protected:
    Poly* poly_find_left(const Vertex* v1, const Point& v2);
    Poly* poly_find_left(const Point& p1, const Point& p2);
    std::pair<Edge*, bool> edge_insert(Vertex* from, Vertex* to);
    Edge* edge_find(const Vertex* v1, const Vertex* v2);
    void vertex_insert_edge(Edge* edge);
    // flood fill polygons with type
    void poly_fill_type(Poly* first, int type);
    
    Poly* poly_find(const Point& val);
    
    void poly_get_portal(Poly* from, Poly* to, Point* left, Point* right);

    void path_init();                    // initialize confspace state
    bool path_dijkstra(const Point& from, const Point& to, std::vector<Poly*>* ret);
    void path_straight(const Point& from, const Point& to, const std::vector<Poly*>& path, Polyline* ret);
};


}

#endif

 #ifndef slic3r_NonplanarSurface_hpp_
#define slic3r_NonplanarSurface_hpp_

#include "libslic3r.h"
#include "NonplanarFacet.hpp"
#include "Point.hpp"
#include "Polygon.hpp"
#include "ExPolygon.hpp"
#include "Geometry.hpp"
#include "ClipperUtils.hpp"

namespace Slic3r {

typedef struct {
  float x;
  float y;
  float z;
} mesh_vertex;

typedef struct {
  mesh_vertex    max;
  mesh_vertex    min;
} mesh_stats;

class NonplanarSurface;

typedef std::vector<NonplanarSurface> NonplanarSurfaces;

class NonplanarSurface
{
    public:
    std::map<int, NonplanarFacet> mesh;
    mesh_stats stats;
    NonplanarSurface() {};
    ~NonplanarSurface() {};
    NonplanarSurface(std::map<int, NonplanarFacet> &_mesh);
    bool operator==(const NonplanarSurface& other) const;
    void calculate_stats();
    void translate(float x, float y, float z);
    void scale(float factor);
    void scale(float versor[3]);
    void rotate_z(float angle);
    void debug_output();
    NonplanarSurfaces group_surfaces();
    void mark_neighbor_surfaces(int id);
    bool check_max_printing_height(float height);
    void check_printable_surfaces(float max_angle);
    bool check_surface_area(float minimal_area);
    ExPolygons horizontal_projection() const;

};
};

#endif

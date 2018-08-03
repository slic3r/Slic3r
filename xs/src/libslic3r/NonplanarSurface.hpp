 #ifndef slic3r_NonplanarSurface_hpp_
#define slic3r_NonplanarSurface_hpp_

#include "libslic3r.h"
#include "NonplanarFacet.hpp"

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
    void calculate_stats();
    void translate(float x, float y, float z);
    void scale(float factor);
    void scale(float versor[3]);
    void rotate_z(float angle);
    void debug_output();
    NonplanarSurfaces group_surfaces();
    void mark_neighbour_surfaces(int id);
    void check_max_printing_height(float height);
    void check_printable_surfaces(float max_angle);

};
};

#endif

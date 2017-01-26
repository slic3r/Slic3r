#ifndef slic3r_Surface_hpp_
#define slic3r_Surface_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"

namespace Slic3r {

enum SurfaceType { stTop, stBottom, stBottomBridge, stInternal, stInternalSolid, stInternalBridge, stInternalVoid };

class Surface
{
    public:
    SurfaceType     surface_type;
    ExPolygon       expolygon;
    double          thickness;          // in mm
    unsigned short  thickness_layers;   // in layers
    double          bridge_angle;       // in radians, ccw, 0 = East, only 0+ (negative means undefined)
    unsigned short  extra_perimeters;
    
    Surface(SurfaceType _surface_type, const ExPolygon &_expolygon)
        : surface_type(_surface_type), expolygon(_expolygon),
            thickness(-1), thickness_layers(1), bridge_angle(-1), extra_perimeters(0)
        {};
    operator Polygons() const;
    double area() const;
    bool is_solid() const;
    bool is_external() const;
    bool is_internal() const;
    bool is_bottom() const;
    bool is_bridge() const;
};

typedef std::vector<Surface> Surfaces;
typedef std::vector<Surface*> SurfacesPtr;
typedef std::vector<const Surface*> SurfacesConstPtr;

inline Polygons
to_polygons(const Surfaces &surfaces)
{
    Slic3r::Polygons pp;
    for (Surfaces::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)*s);
    return pp;
}

inline Polygons
to_polygons(const SurfacesPtr &surfaces)
{
    Slic3r::Polygons pp;
    for (SurfacesPtr::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)**s);
    return pp;
}

inline Polygons
to_polygons(const SurfacesConstPtr &surfaces)
{
    Slic3r::Polygons pp;
    for (SurfacesConstPtr::const_iterator s = surfaces.begin(); s != surfaces.end(); ++s)
        append_to(pp, (Polygons)**s);
    return pp;
}

}

#endif

#ifndef slic3r_SurfaceCollection_hpp_
#define slic3r_SurfaceCollection_hpp_

#include "libslic3r.h"
#include "Surface.hpp"
#include <vector>

namespace Slic3r {

class SurfaceCollection
{
    public:
    Surfaces surfaces;
    
    SurfaceCollection() {};
    SurfaceCollection(const Surfaces &_surfaces)
        : surfaces(_surfaces) {};
    operator Polygons() const;
    operator ExPolygons() const;
    void simplify(double tolerance);
    void group(std::vector<SurfacesConstPtr> *retval) const;
    template <class T> bool any_internal_contains(const T &item) const;
    template <class T> bool any_bottom_contains(const T &item) const;
    SurfacesPtr filter_by_type(SurfaceType type);
    void filter_by_type(SurfaceType type, Polygons* polygons);
    void append(const SurfaceCollection &coll);
    void append(const Surfaces &surfaces);
    void append(const ExPolygons &src, const Surface &templ);
    void append(const ExPolygons &src, SurfaceType surfaceType);
    size_t polygons_count() const;
    bool empty() const { return this->surfaces.empty(); };
    void clear() { this->surfaces.clear(); };
};

}

#endif

#ifndef slic3r_FillRectilinear2_hpp_
#define slic3r_FillRectilinear2_hpp_

#include "../libslic3r.h"

#include "Fill.hpp"

namespace Slic3r {

class Surface;

class FillRectilinear2 : public Fill
{
public:
    virtual Fill* clone() const { return new FillRectilinear2(*this); };
    virtual ~FillRectilinear2() {}
    virtual Polylines fill_surface(const Surface &surface);

protected:
	bool fill_surface_by_lines(const Surface *surface, float angleBase, float pattern_shift, Polylines &polylines_out);
};

class FillGrid2 : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillGrid2(*this); };
    virtual ~FillGrid2() {}
    virtual Polylines fill_surface(const Surface &surface);

protected:
	// The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};

class FillTriangles : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillTriangles(*this); };
    virtual ~FillTriangles() {}
    virtual Polylines fill_surface(const Surface &surface);

protected:
	// The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};

class FillStars : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillStars(*this); };
    virtual ~FillStars() {}
    virtual Polylines fill_surface(const Surface &surface);

protected:
    // The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};

class FillCubic : public FillRectilinear2
{
public:
    virtual Fill* clone() const { return new FillCubic(*this); };
    virtual ~FillCubic() {}
    virtual Polylines fill_surface(const Surface &surface);

protected:
	// The grid fill will keep the angle constant between the layers, see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
};


// Remove sticks (tentacles with zero area) from the polygon.
extern bool remove_sticks(Polygon &poly);
extern bool remove_sticks(Polygons &polys);
extern bool remove_sticks(ExPolygon &poly);
extern bool remove_small(Polygons &polys, double min_area);


}; // namespace Slic3r

#endif // slic3r_FillRectilinear2_hpp_

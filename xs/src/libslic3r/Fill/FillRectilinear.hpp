#ifndef slic3r_FillRectilinear_hpp_
#define slic3r_FillRectilinear_hpp_

#include "../libslic3r.h"

#include "Fill.hpp"

namespace Slic3r {

class FillRectilinear : public Fill
{
public:
    virtual Fill* clone() const { return new FillRectilinear(*this); };
    virtual ~FillRectilinear() {}
    virtual bool can_solid() const { return true; };

protected:
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const direction_t               &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
    
	void _fill_single_direction(ExPolygon expolygon, const direction_t &direction,
	    coord_t x_shift, Polylines* out);
};

class FillAlignedRectilinear : public FillRectilinear
{
public:
    virtual Fill* clone() const { return new FillAlignedRectilinear(*this); };
    virtual ~FillAlignedRectilinear() {};
    virtual bool can_solid() const { return false; };

protected:
	// Keep the angle constant in all layers.
    virtual float _layer_angle(size_t idx) const { return 0.f; };
};

class FillGrid : public FillRectilinear
{
public:
    virtual Fill* clone() const { return new FillGrid(*this); };
    virtual ~FillGrid() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
};

class FillTriangles : public FillRectilinear
{
public:
    virtual Fill* clone() const { return new FillTriangles(*this); };
    virtual ~FillTriangles() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
};

class FillStars : public FillRectilinear
{
public:
    virtual Fill* clone() const { return new FillStars(*this); };
    virtual ~FillStars() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
};

class FillCubic : public FillRectilinear
{
public:
    virtual Fill* clone() const { return new FillCubic(*this); };
    virtual ~FillCubic() {}
    virtual bool can_solid() const { return false; };

protected:
	// The grid fill will keep the angle constant between the layers,; see the implementation of Slic3r::Fill.
    virtual float _layer_angle(size_t idx) const { return 0.f; }
	
	virtual void _fill_surface_single(
	    unsigned int                     thickness_layers,
	    const std::pair<float, Point>   &direction, 
	    ExPolygon                       &expolygon, 
	    Polylines*                      polylines_out);
};

}; // namespace Slic3r

#endif // slic3r_FillRectilinear_hpp_

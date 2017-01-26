#ifndef slic3r_Fill_hpp_
#define slic3r_Fill_hpp_

#include <assert.h>
#include <memory.h>
#include <float.h>
#include <stdint.h>

#include "../libslic3r.h"
#include "../BoundingBox.hpp"
#include "../ExPolygon.hpp"
#include "../Polyline.hpp"
#include "../PrintConfig.hpp"

namespace Slic3r {

class Surface;

// Abstract base class for the infill generators.
class Fill
{
public:
    // Index of the layer.
    size_t      layer_id;
    
    // Z coordinate of the top print surface, in unscaled coordinates
    coordf_t    z;
    
    // in unscaled coordinates
    coordf_t    min_spacing;
    
    // overlap over spacing for extrusion endpoints
    float       endpoints_overlap;
    
    // in radians, ccw, 0 = East
    float       angle;
    
    // In scaled coordinates. Maximum lenght of a perimeter segment connecting two infill lines.
    // Used by the FillRectilinear2, FillGrid2, FillTriangles, FillStars and FillCubic.
    // If left to zero, the links will not be limited.
    coord_t     link_max_length;
    
    // In scaled coordinates. Used by the concentric infill pattern to clip the loops to create extrusion paths.
    coord_t     loop_clipping;
    
    // In scaled coordinates. Bounding box of the 2D projection of the object.
    // If not defined, the bounding box of each single expolygon is used.
    BoundingBox bounding_box;
    
    // Fill density, fraction in <0, 1>
    float       density;

    // Don't connect the fill lines around the inner perimeter.
    bool        dont_connect;

    // Don't adjust spacing to fill the space evenly.
    bool        dont_adjust;

    // For Honeycomb.
    // we were requested to complete each loop;
    // in this case we don't try to make more continuous paths
    bool        complete;

public:
    static Fill* new_from_type(const InfillPattern type);
    static Fill* new_from_type(const std::string &type);
    static coord_t adjust_solid_spacing(const coord_t width, const coord_t distance);
    virtual Fill* clone() const = 0;
    virtual ~Fill() {};
    
    // Implementations can override the following virtual methods:
    // Use bridge flow for the fill?
    virtual bool use_bridge_flow() const { return false; };

    // Do not sort the fill lines to optimize the print head path?
    virtual bool no_sort() const { return false; };

    // Can this pattern be used for solid infill?
    virtual bool can_solid() const { return false; };

    // Perform the fill.
    virtual Polylines fill_surface(const Surface &surface);
    
    coordf_t spacing() const { return this->_spacing; };
    
protected:
    // the actual one in unscaled coordinates, we fill this while generating paths
    coordf_t _spacing;
    
    Fill() :
        layer_id(size_t(-1)),
        z(0.f),
        min_spacing(0.f),
        endpoints_overlap(0.3f),
        angle(0),
        link_max_length(0),
        loop_clipping(0),
        density(0),
        dont_connect(false),
        dont_adjust(false),
        complete(false),
        _spacing(0.f)
        {};
    
    typedef std::pair<float, Point> direction_t;
    
    // The expolygon may be modified by the method to avoid a copy.
    virtual void _fill_surface_single(
        unsigned int                    thickness_layers,
        const direction_t               &direction, 
        ExPolygon                       &expolygon, 
        Polylines*                      polylines_out) {};
    
    // Implementations can override the following virtual method:
    virtual float _layer_angle(size_t idx) const {
        return (idx % 2) == 0 ? (M_PI/2.) : 0;
    };

    direction_t _infill_direction(const Surface &surface) const;
};

} // namespace Slic3r

#endif // slic3r_Fill_hpp_

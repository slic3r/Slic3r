#include <cassert>
#include <math.h>
#include <stdio.h>

#include "../ClipperUtils.hpp"
#include "../Geometry.hpp"
#include "../Surface.hpp"
#include "../PrintConfig.hpp"

#include "Fill.hpp"
#include "FillConcentric.hpp"
#include "FillHoneycomb.hpp"
#include "Fill3DHoneycomb.hpp"
#include "FillPlanePath.hpp"
#include "FillRectilinear.hpp"

namespace Slic3r {

Fill*
Fill::new_from_type(const InfillPattern type)
{
    switch (type) {
        case ipConcentric:          return new FillConcentric();
        case ipHoneycomb:           return new FillHoneycomb();
        case ip3DHoneycomb:         return new Fill3DHoneycomb();
        
        case ipRectilinear:         return new FillRectilinear();
        case ipAlignedRectilinear:  return new FillAlignedRectilinear();
        case ipGrid:                return new FillGrid();
        
        case ipTriangles:           return new FillTriangles();
        case ipStars:               return new FillStars();
        case ipCubic:               return new FillCubic();
        
        case ipArchimedeanChords:   return new FillArchimedeanChords();
        case ipHilbertCurve:        return new FillHilbertCurve();
        case ipOctagramSpiral:      return new FillOctagramSpiral();
        
        default: CONFESS("unknown type"); return NULL;
    }
}

Fill*
Fill::new_from_type(const std::string &type)
{
    static t_config_enum_values enum_keys_map = ConfigOptionEnum<InfillPattern>::get_enum_values();
    t_config_enum_values::const_iterator it = enum_keys_map.find(type);
    return (it == enum_keys_map.end()) ? NULL : new_from_type(InfillPattern(it->second));
}

Polylines
Fill::fill_surface(const Surface &surface)
{
    if (this->density == 0) return Polylines();
    
    // Perform offset.
    ExPolygons expp = offset_ex(surface.expolygon, -scale_(this->min_spacing)/2);
    
    // Implementations can change this if they adjust the flow.
    this->_spacing = this->min_spacing;
    
    // Create the infills for each of the regions.
    Polylines polylines_out;
    for (size_t i = 0; i < expp.size(); ++i)
        this->_fill_surface_single(
            surface.thickness_layers,
            this->_infill_direction(surface),
            expp[i],
            &polylines_out
        );
    return polylines_out;
}

// Returns orientation of the infill and the reference point of the infill pattern.
// For a normal print, the reference point is the center of a bounding box of the STL.
Fill::direction_t
Fill::_infill_direction(const Surface &surface) const
{
    // set infill angle
    float out_angle = this->angle;

    // Bounding box is the bounding box of a Slic3r::PrintObject
    // The bounding box is only undefined in unit tests.
    Point out_shift = this->bounding_box.defined
        ? this->bounding_box.center()
    	: surface.expolygon.contour.bounding_box().center();

    #if 0
        if (!this->bounding_box.defined) {
            printf("Fill::_infill_direction: empty bounding box!");
        } else {
            printf("Fill::_infill_direction: reference point %d, %d\n", out_shift.x, out_shift.y);
        }
    #endif

    if (surface.bridge_angle >= 0) {
	    // use bridge angle
        #ifdef SLIC3R_DEBUG
        printf("Filling bridge with angle %f\n", Slic3r::Geometry::rad2deg(surface.bridge_angle));
        #endif
        out_angle = surface.bridge_angle;
    } else if (this->layer_id != size_t(-1)) {
        // alternate fill direction
        out_angle += this->_layer_angle(this->layer_id / surface.thickness_layers);
    }

    out_angle += float(M_PI/2.);
    return direction_t(out_angle, out_shift);
}

} // namespace Slic3r

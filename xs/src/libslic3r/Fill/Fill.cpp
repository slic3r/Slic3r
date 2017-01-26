#include <math.h>
#include <stdio.h>

#include "../ClipperUtils.hpp"
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

// Calculate a new spacing to fill width with possibly integer number of lines,
// the first and last line being centered at the interval ends.
// This function possibly increases the spacing, never decreases, 
// and for a narrow width the increase in spacing may become severe,
// therefore the adjustment is limited to 20% increase.
coord_t
Fill::adjust_solid_spacing(const coord_t width, const coord_t distance)
{
    assert(width >= 0);
    assert(distance > 0);
    const int number_of_intervals = floor(width / distance);
    if (number_of_intervals == 0) return distance;
    
    coord_t distance_new = (width / number_of_intervals);
    
    const coordf_t factor = coordf_t(distance_new) / coordf_t(distance);
    assert(factor > 1. - 1e-5);
    
    // How much could the extrusion width be increased? By 20%.
    // Because of this limit, this method is not idempotent: each run
    // will increment distance by 20%.
    const coordf_t factor_max = 1.2;
    if (factor > factor_max)
        distance_new = floor((double)distance * factor_max + 0.5);
    
    assert((distance_new * number_of_intervals) <= width);
    
    return distance_new;
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
		//FIXME Vojtech: Add a debugf?
        // Slic3r::debugf "Filling bridge with angle %d\n", rad2deg($surface->bridge_angle);
        #ifdef SLIC3R_DEBUG
        printf("Filling bridge with angle %f\n", surface.bridge_angle);
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

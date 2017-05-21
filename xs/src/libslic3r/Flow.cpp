#include "Flow.hpp"
#include <cmath>
#include <assert.h>

namespace Slic3r {

/* This constructor builds a Flow object from an extrusion width config setting
   and other context properties. */
Flow
Flow::new_from_config_width(FlowRole role, const ConfigOptionFloatOrPercent &width, float nozzle_diameter, float height, float bridge_flow_ratio) {
    // we need layer height unless it's a bridge
    if (height <= 0 && bridge_flow_ratio == 0) CONFESS("Invalid flow height supplied to new_from_config_width()");
    
    float w;
    if (bridge_flow_ratio > 0) {
        // if bridge flow was requested, calculate bridge width
        height = w = Flow::_bridge_width(nozzle_diameter, bridge_flow_ratio);
    } else if (!width.percent && width.value == 0) {
        // if user left option to 0, calculate a sane default width
        w = Flow::_auto_width(role, nozzle_diameter, height);
    } else {
        // if user set a manual value, use it
        w = width.get_abs_value(height);
    }
    
    return Flow(w, height, nozzle_diameter, bridge_flow_ratio > 0);
}

/* This constructor builds a Flow object from a given centerline spacing. */
Flow
Flow::new_from_spacing(float spacing, float nozzle_diameter, float height, bool bridge) {
    // we need layer height unless it's a bridge
    if (height <= 0 && !bridge) CONFESS("Invalid flow height supplied to new_from_spacing()");

    float w = Flow::_width_from_spacing(spacing, nozzle_diameter, height, bridge);
    if (bridge) height = w;
    return Flow(w, height, nozzle_diameter, bridge);
}

/* This method returns the centerline spacing between two adjacent extrusions 
   having the same extrusion width (and other properties). */
float
Flow::spacing() const {
    if (this->bridge) {
        return this->width + BRIDGE_EXTRA_SPACING;
    }
    
    // rectangle with semicircles at the ends
    float min_flow_spacing = this->width - this->height * (1 - PI/4.0);
    return this->width - OVERLAP_FACTOR * (this->width - min_flow_spacing);
}

void
Flow::set_spacing(float spacing) {
    this->width = Flow::_width_from_spacing(spacing, this->nozzle_diameter, this->height, this->bridge);
}

/* This method returns the centerline spacing between an extrusion using this
   flow and another one using another flow.
   this->spacing(other) shall return the same value as other.spacing(*this) */
float
Flow::spacing(const Flow &other) const {
    assert(this->height == other.height);
    assert(this->bridge == other.bridge);
    
    if (this->bridge) {
        return this->width/2 + other.width/2 + BRIDGE_EXTRA_SPACING;
    }
    
    return this->spacing()/2 + other.spacing()/2;
}

/* This method returns extrusion volume per head move unit. */
double
Flow::mm3_per_mm() const {
    if (this->bridge) {
        return (this->width * this->width) * PI/4.0;
    }
    
    // rectangle with semicircles at the ends
    return this->width * this->height + (this->height*this->height) / 4.0 * (PI-4.0);
}

/* This static method returns bridge width for a given nozzle diameter. */
float
Flow::_bridge_width(float nozzle_diameter, float bridge_flow_ratio) {
    if (bridge_flow_ratio == 1) return nozzle_diameter;  // optimization to avoid sqrt()
    return sqrt(bridge_flow_ratio * (nozzle_diameter*nozzle_diameter));
}

/* This static method returns a sane extrusion width default. */
float
Flow::_auto_width(FlowRole role, float nozzle_diameter, float height) {
    // here we calculate a sane default by matching the flow speed (at the nozzle) and the feed rate
    // shape: rectangle with semicircles at the ends
    float width = ((nozzle_diameter*nozzle_diameter) * PI + (height*height) * (4.0 - PI)) / (4.0 * height);
    
    float min = nozzle_diameter * 1.05;
    float max = nozzle_diameter * 3; // cap width to 3x nozzle diameter
    if (role == frExternalPerimeter || role == frSupportMaterial || role == frSupportMaterialInterface) {
        min = max = nozzle_diameter;
    } else if (role != frInfill) {
        // do not limit width for sparse infill so that we use full native flow for it
        max = nozzle_diameter * 1.7;
    }
    if (width > max) width = max;
    if (width < min) width = min;
    
    return width;
}

/* This static method returns the extrusion width value corresponding to the supplied centerline spacing. */
float
Flow::_width_from_spacing(float spacing, float nozzle_diameter, float height, bool bridge) {
    if (bridge) {
        return spacing - BRIDGE_EXTRA_SPACING;
    }
    
    // rectangle with semicircles at the ends
    return spacing + OVERLAP_FACTOR * height * (1 - PI/4.0);
}

/// Calculate a new spacing to fill width with possibly integer number of lines,
/// the first and last line being centered at the interval ends.
/// This function possibly increases the spacing, never decreases, 
/// and for a narrow width the increase in spacing may become severe,
/// therefore the adjustment is limited to 20% increase.
template <class T>
T
Flow::solid_spacing(const T total_width, const T spacing)
{
    assert(total_width >= 0);
    assert(spacing > 0);
    const int number_of_intervals = floor(total_width / spacing);
    if (number_of_intervals == 0) return spacing;
    
    T spacing_new = (total_width / number_of_intervals);
    
    const double factor = (double)spacing_new / (double)spacing;
    assert(factor > 1. - 1e-5);
    
    // How much could the extrusion width be increased? By 20%.
    // Because of this limit, this method is not idempotent: each run
    // will increment spacing by 20%.
    const double factor_max = 1.2;
    if (factor > factor_max)
        spacing_new = floor((double)spacing * factor_max + 0.5);
    
    assert((spacing_new * number_of_intervals) <= total_width);
    
    return spacing_new;
}
template coord_t Flow::solid_spacing<coord_t>(const coord_t total_width, const coord_t spacing);
template coordf_t Flow::solid_spacing<coordf_t>(const coordf_t total_width, const coordf_t spacing);

}

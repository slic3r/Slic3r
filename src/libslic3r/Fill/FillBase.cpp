#include <stdio.h>

#include "../ClipperUtils.hpp"
#include "../Surface.hpp"
#include "../PrintConfig.hpp"
#include "../ExtrusionEntityCollection.hpp"

#include "FillBase.hpp"
#include "FillConcentric.hpp"
#include "FillHoneycomb.hpp"
#include "Fill3DHoneycomb.hpp"
#include "FillGyroid.hpp"
#include "FillPlanePath.hpp"
#include "FillRectilinear.hpp"
#include "FillRectilinear2.hpp"
#include "FillRectilinear3.hpp"
#include "FillSmooth.hpp"

namespace Slic3r {

Fill* Fill::new_from_type(const InfillPattern type)
{
    switch (type) {
    case ipConcentric:          return new FillConcentric();
    case ipConcentricGapFill:   return new FillConcentricWGapFill();
    case ipHoneycomb:           return new FillHoneycomb();
    case ip3DHoneycomb:         return new Fill3DHoneycomb();
    case ipGyroid:              return new FillGyroid();
    case ipRectilinear:         return new FillRectilinear2();
//  case ipRectilinear:         return new FillRectilinear();
    case ipLine:                return new FillLine();
    case ipGrid:                return new FillGrid2();
    case ipTriangles:           return new FillTriangles();
    case ipStars:               return new FillStars();
    case ipCubic:               return new FillCubic();
//  case ipGrid:                return new FillGrid();
    case ipArchimedeanChords:   return new FillArchimedeanChords();
    case ipHilbertCurve:        return new FillHilbertCurve();
    case ipOctagramSpiral:      return new FillOctagramSpiral();
    case ipSmooth:              return new FillSmooth();
    case ipSmoothTriple:        return new FillSmoothTriple();
    case ipSmoothHilbert:       return new FillSmoothHilbert();
    case ipRectiWithPerimeter:  return new FillRectilinear2Peri();
    default: throw std::invalid_argument("unknown type");
    }
}

Fill* Fill::new_from_type(const std::string &type)
{
    const t_config_enum_values &enum_keys_map = ConfigOptionEnum<InfillPattern>::get_enum_values();
    t_config_enum_values::const_iterator it = enum_keys_map.find(type);
    return (it == enum_keys_map.end()) ? nullptr : new_from_type(InfillPattern(it->second));
}

Polylines Fill::fill_surface(const Surface *surface, const FillParams &params)
{
    // Perform offset.
    Slic3r::ExPolygons expp = offset_ex(surface->expolygon, float(scale_(0 - 0.5 * this->spacing)));
    // Create the infills for each of the regions.
    Polylines polylines_out;
    for (size_t i = 0; i < expp.size(); ++ i)
        _fill_surface_single(
            params,
            surface->thickness_layers,
            _infill_direction(surface),
            expp[i],
            polylines_out);
    return polylines_out;
}

// Calculate a new spacing to fill width with possibly integer number of lines,
// the first and last line being centered at the interval ends.
// This function possibly increases the spacing, never decreases, 
// and for a narrow width the increase in spacing may become severe,
// therefore the adjustment is limited to 20% increase.
coord_t Fill::_adjust_solid_spacing(const coord_t width, const coord_t distance)
{
    assert(width >= 0);
    assert(distance > 0);
    // floor(width / distance)
    coord_t number_of_intervals = (coord_t)((width - EPSILON) / distance);
    coord_t distance_new = (number_of_intervals == 0) ? 
        distance : 
        (coord_t)(((width - EPSILON) / number_of_intervals));
    const coordf_t factor = coordf_t(distance_new) / coordf_t(distance);
    assert(factor > 1. - 1e-5);
    // How much could the extrusion width be increased? By 20%.
    const coordf_t factor_max = 1.2;
    if (factor > factor_max)
        distance_new = coord_t(floor((coordf_t(distance) * factor_max + 0.5)));
    return distance_new;
}

// Returns orientation of the infill and the reference point of the infill pattern.
// For a normal print, the reference point is the center of a bounding box of the STL.
std::pair<float, Point> Fill::_infill_direction(const Surface *surface) const
{
    // set infill angle
    float out_angle = this->angle;

    if (out_angle == FLT_MAX) {
        //FIXME Vojtech: Add a warning?
        printf("Using undefined infill angle\n");
        out_angle = 0.f;
    }

    // Bounding box is the bounding box of a perl object Slic3r::Print::Object (c++ object Slic3r::PrintObject)
    // The bounding box is only undefined in unit tests.
    Point out_shift = empty(this->bounding_box) ? 
        surface->expolygon.contour.bounding_box().center() : 
        this->bounding_box.center();

#if 0
    if (empty(this->bounding_box)) {
        printf("Fill::_infill_direction: empty bounding box!");
    } else {
        printf("Fill::_infill_direction: reference point %d, %d\n", out_shift.x, out_shift.y);
    }
#endif

    if (surface->bridge_angle >= 0) {
        // use bridge angle
        //FIXME Vojtech: Add a debugf?
        // Slic3r::debugf "Filling bridge with angle %d\n", rad2deg($surface->bridge_angle);
#ifdef SLIC3R_DEBUG
        printf("Filling bridge with angle %f\n", surface->bridge_angle);
#endif /* SLIC3R_DEBUG */
        out_angle = (float)(surface->bridge_angle);
    } else if (this->layer_id != size_t(-1)) {
        // alternate fill direction
        out_angle += this->_layer_angle(this->layer_id / surface->thickness_layers);
    } else {
//        printf("Layer_ID undefined!\n");
    }

    out_angle += float(M_PI/2.);
    return std::pair<float, Point>(out_angle, out_shift);
}

void Fill::fill_surface_extrusion(const Surface *surface, const FillParams &params, const Flow &flow, const ExtrusionRole &role, ExtrusionEntitiesPtr &out) {
    //add overlap & call fill_surface
    Polylines polylines = this->fill_surface(surface, params);
    if (polylines.empty())
        return;
    // ensure it doesn't over or under-extrude
    double multFlow = 1;
    if (!params.dont_adjust && params.full_infill() && !flow.bridge && params.fill_exactly){
        // compute the path of the nozzle -> extruded volume
        double lengthTot = 0;
        int nbLines = 0;
        for (auto pline = polylines.begin(); pline != polylines.end(); ++pline){
            Lines lines = pline->lines();
            for (auto line = lines.begin(); line != lines.end(); ++line){
                lengthTot += unscaled(line->length());
                nbLines++;
            }
        }
        double extrudedVolume = flow.mm3_per_mm() * lengthTot;
        // compute real volume
        double poylineVolume = 0;
        for (auto poly = this->no_overlap_expolygons.begin(); poly != this->no_overlap_expolygons.end(); ++poly) {
            poylineVolume += flow.height*unscaled(unscaled(poly->area()));
            // add external "perimeter gap"
            double perimeterRoundGap = unscaled(poly->contour.length()) * flow.height * (1 - 0.25*PI) * 0.5;
            // add holes "perimeter gaps"
            double holesGaps = 0;
            for (auto hole = poly->holes.begin(); hole != poly->holes.end(); ++hole) {
                holesGaps += unscaled(hole->length()) * flow.height * (1 - 0.25*PI) * 0.5;
            }
            poylineVolume += perimeterRoundGap + holesGaps;
        }
        //printf("process want %f mm3 extruded for a volume of %f space : we mult by %f %i\n",
        //    extrudedVolume,
        //    (poylineVolume),
        //    (poylineVolume) / extrudedVolume,
        //    this->no_overlap_expolygons.size());
        if (extrudedVolume != 0 && poylineVolume != 0) multFlow = poylineVolume / extrudedVolume;
        //failsafe, it can happen
        if (multFlow > 1.3) multFlow = 1.3;
        if (multFlow < 0.8) multFlow = 0.8;
    }

    // Save into layer.
    auto *eec = new ExtrusionEntityCollection();
    /// pass the no_sort attribute to the extrusion path
    eec->no_sort = this->no_sort();
    /// add it into the collection
    out.push_back(eec);
    //get the role
    ExtrusionRole good_role = role;
    if (good_role == erNone || good_role == erCustom) {
        good_role = (flow.bridge ? erBridgeInfill :
            (surface->is_solid() ?
            ((surface->is_top()) ? erTopSolidInfill : erSolidInfill) :
            erInternalInfill));
    }
    /// push the path
    extrusion_entities_append_paths(
        eec->entities, std::move(polylines),
        good_role,
        flow.mm3_per_mm() * params.flow_mult * multFlow,
        flow.width * params.flow_mult * multFlow,
        flow.height);
    
}

} // namespace Slic3r

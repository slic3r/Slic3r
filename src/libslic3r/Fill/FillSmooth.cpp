#include "../ClipperUtils.hpp"
#include "../ExtrusionEntityCollection.hpp"
#include "../Surface.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

#include "FillSmooth.hpp"

namespace Slic3r {

    Polylines FillSmooth::fill_surface(const Surface *surface, const FillParams &params) const
    {
        //ERROR: you shouldn't call that. Default to the rectilinear one.
        printf("FillSmooth::fill_surface() : you call the wrong method (fill_surface instead of fill_surface_extrusion).\n");
        Polylines polylines_out;
        return polylines_out;
    }

    /// @idx: the index of the step (0 = first step, 1 = second step, ...) The first lay down the volume and the others smoothen the surface.
    void FillSmooth::perform_single_fill(const int idx, ExtrusionEntityCollection &eecroot, const Surface &srf_source,
        const FillParams &params, const double volume) const {
        if (srf_source.expolygon.empty()) return;
        
        // Save into layer smoothing path.
        ExtrusionEntityCollection *eec = new ExtrusionEntityCollection();
        eec->no_sort = params.monotonous;
        FillParams params_modifided = params;
        if (params.config != NULL && idx > 0) params_modifided.density /= (float)params.config->fill_smooth_width.get_abs_value(1);
        else if (params.config != NULL && idx == 0) params_modifided.density *= 1;
        else params_modifided.density *= (float)percentWidth[idx];
        // reduce flow for each increase in density
        params_modifided.flow_mult *= params.density;
        params_modifided.flow_mult /= params_modifided.density;
        // split the flow between steps
        if (params.config != NULL && idx > 0) params_modifided.flow_mult *= (float)params.config->fill_smooth_distribution.get_abs_value(1);
        else if (params.config != NULL && idx == 0) params_modifided.flow_mult *= (1.f - (float)params.config->fill_smooth_distribution.get_abs_value(1));
        else params_modifided.flow_mult *= (float)percentFlow[idx];

        //choose if we are going to extrude with or without overlap
        if ((params.flow->bridge && idx == 0) || has_overlap[idx] || this->no_overlap_expolygons.empty()){
            this->fill_expolygon(idx, *eec, srf_source, params_modifided, volume);
        }
        else{
            Surface surfaceNoOverlap(srf_source);
            for (const ExPolygon &poly : this->no_overlap_expolygons) {
                if (poly.empty()) continue;
                surfaceNoOverlap.expolygon = poly;
                this->fill_expolygon(idx, *eec, surfaceNoOverlap, params_modifided, volume);
            }
        }
        
        if (eec->entities.empty()) delete eec;
        else eecroot.entities.push_back(eec);
    }
    
    void FillSmooth::fill_expolygon(const int idx, ExtrusionEntityCollection &eec, const Surface &srf_to_fill, 
        const FillParams &params, const double volume) const {
        
        std::unique_ptr<Fill> f2 = std::unique_ptr<Fill>(Fill::new_from_type(fillPattern[idx]));
        f2->bounding_box = this->bounding_box;
        f2->init_spacing(this->get_spacing(),params);
        f2->layer_id = this->layer_id;
        f2->z = this->z;
        f2->angle = anglePass[idx] + this->angle;
        // Maximum length of the perimeter segment linking two infill lines.
        f2->link_max_length = this->link_max_length;
        // Used by the concentric infill pattern to clip the loops to create extrusion paths.
        f2->loop_clipping = this->loop_clipping;
        Polylines polylines_layer = f2->fill_surface(&srf_to_fill, params);

        if (!polylines_layer.empty()) {

            //compute the path of the nozzle
            double lengthTot = 0;
            int nbLines = 0;
            for (Polyline &pline : polylines_layer) {
                Lines lines = pline.lines();
                for (Line &line : lines) {
                    lengthTot += unscaled(line.length());
                    nbLines++;
                }
            }
            double extrudedVolume = params.flow->mm3_per_mm() * lengthTot / params.density;
            if (extrudedVolume == 0) extrudedVolume = volume;

            //get the role
            ExtrusionRole good_role = params.role;
            if (good_role == erNone || good_role == erCustom) {
                good_role = params.flow->bridge && idx == 0 ? erBridgeInfill : rolePass[idx];
            }
            // print
            float mult_flow = float(params.fill_exactly /*&& idx == 0*/ ? std::min(2., volume / extrudedVolume) : 1);
            extrusion_entities_append_paths(
                eec.entities, std::move(polylines_layer),
                good_role,
                params.flow->mm3_per_mm() * params.flow_mult * mult_flow,
                //min-reduced flow width for a better view (it's mostly a gui thing, but some support code can want to mess with it)
                (float)(params.flow->width * (params.flow_mult* mult_flow < 0.1 ? 0.1 : params.flow_mult * mult_flow)), (float)params.flow->height);
        }
    }


    void FillSmooth::fill_surface_extrusion(const Surface *surface, const FillParams &params, ExtrusionEntitiesPtr &out) const
    {
        coordf_t init_spacing = this->spacing;

        // compute the volume to extrude
        double volumeToOccupy = 0;
        for (auto poly = this->no_overlap_expolygons.begin(); poly != this->no_overlap_expolygons.end(); ++poly) {
            // add external "perimeter gap"
            double poylineVolume = params.flow->height*unscaled(unscaled(poly->area()));
            double perimeterRoundGap = unscaled(poly->contour.length()) * params.flow->height * (1 - 0.25*PI) * 0.5;
            // add holes "perimeter gaps"
            double holesGaps = 0;
            for (auto hole = poly->holes.begin(); hole != poly->holes.end(); ++hole) {
                holesGaps += unscaled(hole->length()) * params.flow->height * (1 - 0.25*PI) * 0.5;
            }
            poylineVolume += perimeterRoundGap + holesGaps;

            //extruded volume: see http://manual.slic3r.org/advanced/flow-math, and we need to remove a circle at an end (as the flow continue)
            volumeToOccupy += poylineVolume;
        }
        if (this->no_overlap_expolygons.empty()) {
            volumeToOccupy = unscaled(unscaled(surface->area())) * params.flow->height;
        }

        //create root node
        ExtrusionEntityCollection *eecroot = new ExtrusionEntityCollection();
        //you don't want to sort the extrusions: big infill first, small second
        eecroot->no_sort = true;

        // first infill
        perform_single_fill(0, *eecroot, *surface, params, volumeToOccupy);

        //use monotonous for ironing pass
        FillParams monotonous_params = params;
        monotonous_params.monotonous = true;

        //second infill
        if (nbPass > 1){
            perform_single_fill(1, *eecroot, *surface, monotonous_params, volumeToOccupy);
        }

        // third infill
        if (nbPass > 2){
            perform_single_fill(2, *eecroot, *surface, monotonous_params, volumeToOccupy);
        }
        
        if (!eecroot->entities.empty()) 
            out.push_back(eecroot);
        else delete eecroot;

    }

} // namespace Slic3r

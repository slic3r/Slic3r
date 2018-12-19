#include "../ClipperUtils.hpp"
#include "../PolylineCollection.hpp"
#include "../ExtrusionEntityCollection.hpp"
#include "../Surface.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

#include "FillSmooth.hpp"

namespace Slic3r {

    Polylines FillSmooth::fill_surface(const Surface *surface, const FillParams &params)
    {
        //ERROR: you shouldn't call that. Default to the rectilinear one.
        printf("FillSmooth::fill_surface() : you call the wrong method (fill_surface instead of fill_surface_extrusion).\n");
        Polylines polylines_out;
        return polylines_out;
    }


    void FillSmooth::fill_surface_extrusion(const Surface *surface, const FillParams &params,
        const Flow &flow, const ExtrusionRole &role, ExtrusionEntitiesPtr &out)
    {
        coordf_t init_spacing = this->spacing;

        //printf("FillSmooth::fill_surface() : you call the right method (fill_surface instead of fill_surface_extrusion).\n");
        //second pass with half layer width
        FillParams params1 = params;
        FillParams params2 = params;
        FillParams params3 = params;
        params1.density *= percentWidth[0];
        params2.density *= percentWidth[1];
        params3.density *= percentWidth[2];

        // compute the volume to extrude
        double volumeToOccupy = 0;
        for (auto poly = this->no_overlap_expolygons.begin(); poly != this->no_overlap_expolygons.end(); ++poly) {
            // add external "perimeter gap"
            double poylineVolume = flow.height*unscaled(unscaled(poly->area()));
            double perimeterRoundGap = unscaled(poly->contour.length()) * flow.height * (1 - 0.25*PI) * 0.5;
            // add holes "perimeter gaps"
            double holesGaps = 0;
            for (auto hole = poly->holes.begin(); hole != poly->holes.end(); ++hole) {
                holesGaps += unscaled(hole->length()) * flow.height * (1 - 0.25*PI) * 0.5;
            }
            poylineVolume += perimeterRoundGap + holesGaps;

            //extruded volume: see http://manual.slic3r.org/advanced/flow-math, and we need to remove a circle at an end (as the flow continue)
            volumeToOccupy += poylineVolume;
        }
        //if (polylines_layer1.empty() && polylines_layer2.empty() && polylines_layer3.empty())
        //    return;

        //create root node
        ExtrusionEntityCollection *eecroot = new ExtrusionEntityCollection();
        //you don't want to sort the extrusions: big infill first, small second
        eecroot->no_sort = true;

        ExtrusionEntityCollection *eec;


        // first infill
        std::unique_ptr<Fill> f1 = std::unique_ptr<Fill>(Fill::new_from_type(fillPattern[0]));
        f1->bounding_box = this->bounding_box;
        f1->spacing = init_spacing;
        f1->layer_id = this->layer_id;
        f1->z = this->z;
        f1->angle = anglePass[0];
        // Maximum length of the perimeter segment linking two infill lines.
        f1->link_max_length = this->link_max_length;
        // Used by the concentric infill pattern to clip the loops to create extrusion paths.
        f1->loop_clipping = this->loop_clipping;
        Surface surfaceNoOverlap(*surface);
        if (flow.bridge) {

            Polylines polylines_layer1 = f1->fill_surface(surface, params1);

            if (!polylines_layer1.empty()) {
                if (fillPattern[2] == InfillPattern::ipRectilinear && polylines_layer1[0].points.size() > 3) {
                    polylines_layer1[0].points.erase(polylines_layer1[0].points.begin());
                    polylines_layer1[polylines_layer1.size() - 1].points.pop_back();
                }

                //compute the path of the nozzle
                double lengthTot = 0;
                int nbLines = 0;
                for (auto pline = polylines_layer1.begin(); pline != polylines_layer1.end(); ++pline) {
                    Lines lines = pline->lines();
                    for (auto line = lines.begin(); line != lines.end(); ++line) {
                        lengthTot += unscaled(line->length());
                        nbLines++;
                    }
                }
                double extrudedVolume = flow.mm3_per_mm() * lengthTot;
                if (extrudedVolume == 0) extrudedVolume = 1;

                // Save into layer smoothing path.
                // print thin

                eec = new ExtrusionEntityCollection();
                eecroot->entities.push_back(eec);
                eec->no_sort = false; //can be sorted inside the pass
                extrusion_entities_append_paths(
                    eec->entities, std::move(polylines_layer1),
                    flow.bridge ? erBridgeInfill : rolePass[0],
                    //reduced flow height for a better view (it's only a gui thing)
                    params.flow_mult * flow.mm3_per_mm() * percentFlow[0] * (params.fill_exactly ? volumeToOccupy / extrudedVolume : 1),
                    (float)(flow.width*percentFlow[0] * (params.fill_exactly ? volumeToOccupy / extrudedVolume : 1)), (float)flow.height*0.8);
            } else {
                return;
            }

        } else {
            for (auto poly = this->no_overlap_expolygons.begin(); poly != this->no_overlap_expolygons.end(); ++poly) {
                surfaceNoOverlap.expolygon = *poly;
                Polylines polylines_layer1 = f1->fill_surface(&surfaceNoOverlap, params1);
                if (!polylines_layer1.empty()) {

                    double lengthTot = 0;
                    int nbLines = 0;
                    for (auto pline = polylines_layer1.begin(); pline != polylines_layer1.end(); ++pline) {
                        Lines lines = pline->lines();
                        for (auto line = lines.begin(); line != lines.end(); ++line) {
                            lengthTot += unscaled(line->length());
                            nbLines++;
                        }
                    }

                    // add external "perimeter gap"
                    double poylineVolume = flow.height*unscaled(unscaled(poly->area()));
                    double perimeterRoundGap = unscaled(poly->contour.length()) * flow.height * (1 - 0.25*PI) * 0.5;
                    // add holes "perimeter gaps"
                    double holesGaps = 0;
                    for (auto hole = poly->holes.begin(); hole != poly->holes.end(); ++hole) {
                        holesGaps += unscaled(hole->length()) * flow.height * (1 - 0.25*PI) * 0.5;
                    }
                    poylineVolume += perimeterRoundGap + holesGaps;

                    //extruded volume: see http://manual.slic3r.org/advanced/flow-math, and we need to remove a circle at an end (as the flow continue)
                    double extrudedVolume = flow.mm3_per_mm() * lengthTot;

                    eec = new ExtrusionEntityCollection();
                    eecroot->entities.push_back(eec);
                    eec->no_sort = false; //can be sorted inside the pass
                    //get the role
                    ExtrusionRole good_role = role;
                    if (good_role == erNone || good_role == erCustom) {
                        good_role = flow.bridge ? erBridgeInfill : rolePass[0];
                    }
                    extrusion_entities_append_paths(
                        eec->entities, std::move(polylines_layer1),
                        good_role,
                        //reduced flow height for a better view (it's only a gui thing)
                        params.flow_mult * flow.mm3_per_mm() * percentFlow[0] * (params.fill_exactly ? poylineVolume / extrudedVolume : 1),
                        (float)(flow.width*percentFlow[0] * (params.fill_exactly ? poylineVolume / extrudedVolume : 1)), (float)flow.height*0.8);
                } else {
                    return;
                }
            }
        }

        //second infill
        if (nbPass > 1){

            std::unique_ptr<Fill> f2 = std::unique_ptr<Fill>(Fill::new_from_type(fillPattern[1]));
            f2->bounding_box = this->bounding_box;
            f2->spacing = init_spacing;
            f2->layer_id = this->layer_id;
            f2->z = this->z;
            f2->angle = anglePass[1];
            // Maximum length of the perimeter segment linking two infill lines.
            f2->link_max_length = this->link_max_length;
            // Used by the concentric infill pattern to clip the loops to create extrusion paths.
            f2->loop_clipping = this->loop_clipping;
            Polylines polylines_layer2 = f2->fill_surface(surface, params2);

            if (!polylines_layer2.empty()){
                if (fillPattern[2] == InfillPattern::ipRectilinear && polylines_layer2[0].points.size() > 3){
                    polylines_layer2[0].points.erase(polylines_layer2[0].points.begin());
                    polylines_layer2[polylines_layer2.size() - 1].points.pop_back();
                }

                //compute the path of the nozzle
                double lengthTot = 0;
                int nbLines = 0;
                for (auto pline = polylines_layer2.begin(); pline != polylines_layer2.end(); ++pline){
                    Lines lines = pline->lines();
                    for (auto line = lines.begin(); line != lines.end(); ++line){
                        lengthTot += unscaled(line->length());
                        nbLines++;
                    }
                }
                double extrudedVolume = flow.mm3_per_mm() * lengthTot;
                if (extrudedVolume == 0) extrudedVolume = 1;

                // Save into layer smoothing path.
                eec = new ExtrusionEntityCollection();
                eecroot->entities.push_back(eec);
                eec->no_sort = false;
                //get the role
                ExtrusionRole good_role = role;
                if (good_role == erNone || good_role == erCustom) {
                    good_role = rolePass[1];
                }
                // print thin
                extrusion_entities_append_paths(
                    eec->entities, std::move(polylines_layer2),
                    good_role,
                    params.flow_mult * flow.mm3_per_mm() * percentFlow[1] * (params.fill_exactly ? volumeToOccupy / extrudedVolume : 1),
                    //min-reduced flow width for a better view (it's only a gui thing)
                        (float)(flow.width*(percentFlow[1] < 0.1 ? 0.1 : percentFlow[1])), (float)flow.height);
            }
            else{
                return;
            }
        }

        // third infill
        if (nbPass > 2){
            std::unique_ptr<Fill> f3 = std::unique_ptr<Fill>(Fill::new_from_type(fillPattern[0]));
            f3->bounding_box = this->bounding_box;
            f3->spacing = init_spacing;
            f3->layer_id = this->layer_id;
            f3->z = this->z;
            f3->angle = anglePass[2];
            // Maximum length of the perimeter segment linking two infill lines.
            f3->link_max_length = this->link_max_length;
            // Used by the concentric infill pattern to clip the loops to create extrusion paths.
            f3->loop_clipping = this->loop_clipping;
            Polylines polylines_layer3 = f3->fill_surface(surface, params1);

            if (!polylines_layer3.empty()){

                //remove some points that are not inside the area
                if (fillPattern[2] == InfillPattern::ipRectilinear && polylines_layer3[0].points.size() > 3){
                    polylines_layer3[0].points.erase(polylines_layer3[0].points.begin());
                    polylines_layer3[polylines_layer3.size() - 1].points.pop_back();
                }

                //compute the path of the nozzle
                double lengthTot = 0;
                int nbLines = 0;
                for (auto pline = polylines_layer3.begin(); pline != polylines_layer3.end(); ++pline){
                    Lines lines = pline->lines();
                    for (auto line = lines.begin(); line != lines.end(); ++line){
                        lengthTot += unscaled(line->length());
                        nbLines++;
                    }
                }
                double extrudedVolume = flow.mm3_per_mm() * lengthTot;
                
                // Save into layer smoothing path. layer 3
                eec = new ExtrusionEntityCollection();
                eecroot->entities.push_back(eec);
                eec->no_sort = false;
                //get the role
                ExtrusionRole good_role = role;
                if (good_role == erNone || good_role == erCustom) {
                    good_role = rolePass[2];
                }
                // print thin
                extrusion_entities_append_paths(
                    eec->entities, std::move(polylines_layer3),
                    good_role, //slow (if last)
                    //reduced flow width for a better view (it's only a gui thing)
                    params.flow_mult * flow.mm3_per_mm() * percentFlow[2] * (params.fill_exactly ? volumeToOccupy / extrudedVolume : 1),
                        (float)(flow.width*(percentFlow[2] < 0.1 ? 0.1 : percentFlow[2])), (float)flow.height);
            }
        }

        if (!eecroot->entities.empty()) 
            out.push_back(eecroot);
        else delete eecroot;

    }

} // namespace Slic3r

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


	void FillSmooth::fill_surface_extrusion(const Surface *surface, const FillParams &params, const Flow &flow, ExtrusionEntityCollection &out)
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

		//a small under-overlap to prevent over-extrudion on thin surfaces (i.e. remove the overlap)
		Surface surfaceNoOverlap(*surface);
		//remove the overlap (prevent over-extruding) if possible
        ExPolygons noOffsetPolys = offset2_ex(surfaceNoOverlap.expolygon, -scale_(this->overlap) * (flow.bridge?0:1), 0);
		//printf("FillSmooth::fill_surface() : overlap=%f->%f.\n", overlap, -scale_(this->overlap));
		//printf("FillSmooth::polys : 1->%i.\n", noOffsetPolys.size());
		//printf("FillSmooth::polys : %f  %f->%f.\n", surface->expolygon.area(), surfaceNoOverlap.expolygon.area(), noOffsetPolys[0].area());
		//if (offsetPolys.size() == 1) surfaceNoOverlap.expolygon = offsetPolys[0];

		//TODO: recursive if multiple polys instead of failing


		//if (polylines_layer1.empty() && polylines_layer2.empty() && polylines_layer3.empty())
		//	return;

		//create root node
		ExtrusionEntityCollection *eecroot = new ExtrusionEntityCollection();
		//you don't want to sort the extrusions: big infill first, small second
		eecroot->no_sort = true;

		ExtrusionEntityCollection *eec;

		double volumeToOccupy = 0;

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
		for (auto poly = noOffsetPolys.begin(); poly != noOffsetPolys.end(); ++poly){
			surfaceNoOverlap.expolygon = *poly;
			Polylines polylines_layer1 = f1->fill_surface(&surfaceNoOverlap, params1);
			if (!polylines_layer1.empty()){

				double lengthTot = 0;
				int nbLines = 0;
				for (auto pline = polylines_layer1.begin(); pline != polylines_layer1.end(); ++pline){
					Lines lines = pline->lines();
					for (auto line = lines.begin(); line != lines.end(); ++line){
						printf("line length = %f, scaled:%f, unscaled:%f \n", line->length(), scale_(line->length()), unscale(line->length()));
						lengthTot += unscale(line->length());
						nbLines++;
					}
				}

				// add external "perimeter gap"
				double poylineVolume = flow.height*unscale(unscale(poly->area()));
				double perimeterRoundGap = unscale(poly->contour.length()) * flow.height * (1 - 0.25*PI) * 0.5;
				// add holes "perimeter gaps"
				double holesGaps = 0;
				for (auto hole = poly->holes.begin(); hole != poly->holes.end(); ++hole){
					holesGaps += unscale(hole->length()) * flow.height * (1 - 0.25*PI) * 0.5;
				}
				poylineVolume += perimeterRoundGap + holesGaps;

				//extruded volume: see http://manual.slic3r.org/advanced/flow-math, and we need to remove a circle at an end (as the flow continue)
				double extrudedVolume = flow.mm3_per_mm() * lengthTot;
				volumeToOccupy += poylineVolume;

				printf("FillSmooth: request extruding of %f length, with mm3_per_mm of %f =volume=> %f / %f (%f) %s\n",
					lengthTot,
					flow.mm3_per_mm(),
					flow.mm3_per_mm() * lengthTot,
					flow.height*unscale(unscale(surfaceNoOverlap.area())),
					(flow.mm3_per_mm() * lengthTot) / (flow.height*unscale(unscale(surfaceNoOverlap.area()))),
                    params.fill_exactly ? "ready" : "only for info"
					);
				printf("FillSmooth: extruding flow mult %f vs old: %f\n", percentFlow[0] * poylineVolume / extrudedVolume, percentFlow[0] / percentWidth[0]);

				eec = new ExtrusionEntityCollection();
				eecroot->entities.push_back(eec);
				eec->no_sort = false; //can be sorted inside the pass
				extrusion_entities_append_paths(
					eec->entities, STDMOVE(polylines_layer1),
					flow.bridge ? erBridgeInfill : rolePass[0],
					//reduced flow height for a better view (it's only a gui thing)
                    params.flow_mult * flow.mm3_per_mm() * percentFlow[0] * (params.fill_exactly? poylineVolume / extrudedVolume : 1), 
                        (float)(flow.width*percentFlow[0] * (params.fill_exactly ? poylineVolume / extrudedVolume : 1)), (float)flow.height*0.8);
			}
			else{
				return;
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
						//printf("line length = %f, scaled:%f, unscaled:%f \n", line->length(), scale_(line->length()), unscale(line->length()));
						lengthTot += unscale(line->length());
						nbLines++;
					}
				}
				double extrudedVolume = flow.mm3_per_mm() * lengthTot;
				if (extrudedVolume != 0){
					printf("FillSmooth: extruding flow of 2nd layer increased by %f\n", volumeToOccupy / extrudedVolume); 
					printf("FillSmooth: extruding flow of 2nd layer mult %f vs old: %f\n", percentFlow[1] * volumeToOccupy / extrudedVolume, percentFlow[1] / percentWidth[1]);

				}
				else{
					printf("FillSmooth: extruding flow of 2nd layer increased by INFINITE \n");
					extrudedVolume = 1;
				}

				// Save into layer smoothing path.
				eec = new ExtrusionEntityCollection();
				eecroot->entities.push_back(eec);
				eec->no_sort = false;
				// print thin

				extrusion_entities_append_paths(
					eec->entities, STDMOVE(polylines_layer2),
					rolePass[1],
					//reduced flow width for a better view (it's only a gui thing)
                    params.flow_mult * flow.mm3_per_mm() * percentFlow[1] * (params.fill_exactly ? volumeToOccupy / extrudedVolume : 1), 
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
						printf("line length3");
						//printf("line length = %f, scaled:%f, unscaled:%f \n", line->length(), scale_(line->length()), unscale(line->length()));
						lengthTot += unscale(line->length());
						nbLines++;
					}
				}
				double extrudedVolume = flow.mm3_per_mm() * lengthTot;
				printf("FillSmooth: extruding flow of 3nd layer mult %f vs old: %f\n", percentFlow[2] * volumeToOccupy / extrudedVolume, percentFlow[2] / percentWidth[2]);

				// Save into layer smoothing path. layer 3
				eec = new ExtrusionEntityCollection();
				eecroot->entities.push_back(eec);
				eec->no_sort = false;
				// print thin
				extrusion_entities_append_paths(
					eec->entities, STDMOVE(polylines_layer3),
					rolePass[2], //slow (if last)
					//reduced flow width for a better view (it's only a gui thing)
                    params.flow_mult * flow.mm3_per_mm() * percentFlow[2] * (params.fill_exactly ? volumeToOccupy / extrudedVolume : 1),
                        (float)(flow.width*(percentFlow[2] < 0.1 ? 0.1 : percentFlow[2])), (float)flow.height);
			}
		}

		if (!eecroot->entities.empty())
			out.entities.push_back(eecroot);

	}

} // namespace Slic3r

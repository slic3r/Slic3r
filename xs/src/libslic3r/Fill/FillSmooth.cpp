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


void FillSmooth::fill_surface_extrusion(const Surface *surface, const FillParams &params, const Flow &flow, ExtrusionEntityCollection &out )
{
	coordf_t init_spacing = this->spacing;
	
    printf("FillSmooth::fill_surface() : you call the right method (fill_surface instead of fill_surface_extrusion).\n");
    //second pass with half layer width
    FillParams params1 = params;
    FillParams params2 = params;
    FillParams params3 = params;
	params1.density *= percentWidth[0];
	params2.density *= percentWidth[1];
	params3.density *= percentWidth[2];
//    Polylines polylines_layer1;
//    Polylines polylines_layer2; 
//    Polylines polylines_layer3; 
	
	 //a small under-overlap to prevent over-extrudion on thin surfaces (i.e. remove the overlap)
	Surface surfaceNoOverlap(*surface);
	//remove the overlap (prevent over-extruding)
	ExPolygons offsetPloys = offset2_ex(surfaceNoOverlap.expolygon, -scale_(this->overlap)*0.9, 0);
	
	if(offsetPloys.size() == 1) surfaceNoOverlap.expolygon = offsetPloys[0];
	//TODO: recursive if multiple polys instead of failing
	
	//if (! fill_surface_by_lines(&surfaceNoOverlap/* */, params1, anglePass[0], 0.f, polylines_layer1) ||
	//	(nbPass>1 && !fill_surface_by_lines(surface, params2, anglePass[1], 0.f, polylines_layer2)) ||
	//	(nbPass>2 && !fill_surface_by_lines(surface, params3, anglePass[2], 0.f, polylines_layer3))) {
	//	printf("FillSmooth::fill_surface() failed to fill a region.\n");
	//}
	/*
	std::cout<<"polylines_layer1 size ="<<polylines_layer1.size()<<" \n";
	std::cout<<"polylines_layer2 size ="<<polylines_layer2.size()<<" \n";
	std::cout<<"polylines_layer3 size ="<<polylines_layer3.size()<<" \n";*/

	//if (polylines_layer1.empty() && polylines_layer2.empty() && polylines_layer3.empty())
	//	return;
	
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
	Polylines polylines_layer1 = f1->fill_surface(surface, params1);

	if (!polylines_layer1.empty()){
		eec = new ExtrusionEntityCollection();
		eecroot->entities.push_back(eec);
		eec->no_sort = false; //can be sorted inside the pass
		extrusion_entities_append_paths(
			eec->entities, STDMOVE(polylines_layer1),
			flow.bridge ? erBridgeInfill : rolePass[0],
			flow.mm3_per_mm()*percentFlow[0], (float)flow.width*percentFlow[0], (float)flow.height*0.9);
			//reduced flow width & height for a better view (it's only a gui thing)
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
		Polylines polylines_layer2 = f2->fill_surface(&surfaceNoOverlap, params2);

		if (fillPattern[2] == InfillPattern::ipRectilinear && polylines_layer2[0].points.size() > 3){
			polylines_layer2[0].points.erase(polylines_layer2[0].points.begin());
			polylines_layer2[polylines_layer2.size() - 1].points.pop_back();
		}
		if (!polylines_layer2.empty()){

			// Save into layer smoothing path.
			eec = new ExtrusionEntityCollection();
			eecroot->entities.push_back(eec);
			eec->no_sort = false;
			// print thin
			extrusion_entities_append_paths(
				eec->entities, STDMOVE(polylines_layer2),
				rolePass[1],
				flow.mm3_per_mm()*percentFlow[1], (float)flow.width*(percentFlow[1] < 0.1 ? 0.1 : percentFlow[1]), (float)flow.height);
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
		Polylines polylines_layer3 = f3->fill_surface(&surfaceNoOverlap, params1);

		//remove some points that are not inside the area
		if (fillPattern[2] == InfillPattern::ipRectilinear && polylines_layer3[0].points.size() > 3){
			polylines_layer3[0].points.erase(polylines_layer3[0].points.begin());
			polylines_layer3[polylines_layer3.size() - 1].points.pop_back();
		}
		if (!polylines_layer3.empty()){
			// Save into layer smoothing path. layer 3
			eec = new ExtrusionEntityCollection();
			eecroot->entities.push_back(eec);
			eec->no_sort = false;
			// print thin
			extrusion_entities_append_paths(
				eec->entities, STDMOVE(polylines_layer3),
				rolePass[2], //slow (if last)
				flow.mm3_per_mm()*percentFlow[2], (float)flow.width*(percentFlow[2] < 0.1 ? 0.1 : percentFlow[2]), (float)flow.height);
		}
	}

	if (!eecroot->entities.empty())
		out.entities.push_back(eecroot);
    
}

} // namespace Slic3r

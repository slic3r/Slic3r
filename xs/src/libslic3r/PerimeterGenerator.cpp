#include "PerimeterGenerator.hpp"
#include "ClipperUtils.hpp"
#include "ExtrusionEntityCollection.hpp"
#include "BridgeDetector.hpp"
#include "Geometry.hpp"
#include <cmath>
#include <cassert>

namespace Slic3r {

void PerimeterGenerator::process()
{
    // other perimeters
    this->_mm3_per_mm               = this->perimeter_flow.mm3_per_mm();
    coord_t perimeter_width         = this->perimeter_flow.scaled_width();
    coord_t perimeter_spacing       = this->perimeter_flow.scaled_spacing();
    
    // external perimeters
    this->_ext_mm3_per_mm           = this->ext_perimeter_flow.mm3_per_mm();
    coord_t ext_perimeter_width     = this->ext_perimeter_flow.scaled_width();
    coord_t ext_perimeter_spacing   = this->ext_perimeter_flow.scaled_spacing();
    coord_t ext_perimeter_spacing2  = this->ext_perimeter_flow.scaled_spacing(this->perimeter_flow);
    
    // overhang perimeters
    this->_mm3_per_mm_overhang      = this->overhang_flow.mm3_per_mm();
    
    // solid infill
    coord_t solid_infill_spacing    = this->solid_infill_flow.scaled_spacing();
    
    // Calculate the minimum required spacing between two adjacent traces.
    // This should be equal to the nominal flow spacing but we experiment
    // with some tolerance in order to avoid triggering medial axis when
    // some squishing might work. Loops are still spaced by the entire
    // flow spacing; this only applies to collapsing parts.
    // For ext_min_spacing we use the ext_perimeter_spacing calculated for two adjacent
    // external loops (which is the correct way) instead of using ext_perimeter_spacing2
    // which is the spacing between external and internal, which is not correct
    // and would make the collapsing (thus the details resolution) dependent on 
    // internal flow which is unrelated.
    coord_t min_spacing         = perimeter_spacing      * (1 - INSET_OVERLAP_TOLERANCE);
    coord_t ext_min_spacing     = ext_perimeter_spacing  * (1 - INSET_OVERLAP_TOLERANCE);
    
    // prepare grown lower layer slices for overhang detection
    if (this->lower_slices != NULL && this->config->overhangs) {
        // We consider overhang any part where the entire nozzle diameter is not supported by the
        // lower layer, so we take lower slices and offset them by half the nozzle diameter used 
        // in the current layer
        double nozzle_diameter = this->print_config->nozzle_diameter.get_at(this->config->perimeter_extruder-1);
        this->_lower_slices_p = offset(*this->lower_slices, float(scale_(+nozzle_diameter/2)));
    }
    
    // we need to process each island separately because we might have different
    // extra perimeters for each one
    int surface_idx = 0;
    for (const Surface &surface : this->slices->surfaces) {
        // detect how many perimeters must be generated for this island
        int        loop_number = this->config->perimeters + surface.extra_perimeters - 1;  // 0-indexed loops
        surface_idx++;

        if (this->config->only_one_perimeter_top && this->upper_slices == NULL){
            loop_number = 0;
        }
        
        ExPolygons gaps;
        //this var store infill surface removed from last to not add any more perimeters to it.
        ExPolygons stored;
        ExPolygons last        = union_ex(surface.expolygon.simplify_p(SCALED_RESOLUTION));
        if (loop_number >= 0) {
            // In case no perimeters are to be generated, loop_number will equal to -1.            
            std::vector<PerimeterGeneratorLoops> contours(loop_number+1);    // depth => loops
            std::vector<PerimeterGeneratorLoops> holes(loop_number+1);       // depth => loops
            ThickPolylines thin_walls;
            // we loop one time more than needed in order to find gaps after the last perimeter was applied
            for (int i = 0;; ++ i) {  // outer loop is 0

                //store surface for bridge infill to avoid unsupported perimeters (but the first one, this one is always good)
                if (this->config->no_perimeter_unsupported && i == this->config->min_perimeter_unsupported
                    && this->lower_slices != NULL && !this->lower_slices->expolygons.empty()) {
                    //note: i don't know where to use the safety offset or not, so if you know, please modify the block.

                    //compute our unsupported surface
                    ExPolygons unsupported = diff_ex(last, this->lower_slices->expolygons, true);
                    if (!unsupported.empty()) {
                        ExPolygons to_draw;
                        //remove small overhangs
                        ExPolygons unsupported_filtered = offset2_ex(unsupported, -(float)(perimeter_spacing), (float)(perimeter_spacing));
                        if (!unsupported_filtered.empty()) {
                            //to_draw.insert(to_draw.end(), last.begin(), last.end());
                            //extract only the useful part of the lower layer. The safety offset is really needed here.
                            ExPolygons support = diff_ex(last, unsupported, true);
                            if (this->config->noperi_bridge_only && !unsupported.empty()) {
                                //only consider the part that can be bridged (really, by the bridge algorithm)
                                //first, separate into islands (ie, each ExPlolygon)
                                int numploy = 0;
                                //only consider the bottom layer that intersect unsupported, to be sure it's only on our island.
                                ExPolygonCollection lower_island(support);
                                BridgeDetector detector(unsupported_filtered,
                                    lower_island,
                                    perimeter_spacing);
                                if (detector.detect_angle(Geometry::deg2rad(this->config->bridge_angle.value))) {
                                    ExPolygons bridgeable = union_ex(detector.coverage(-1, true));
                                    if (!bridgeable.empty()) {
                                        //simplify to avoid most of artefacts from printing lines.
                                        ExPolygons bridgeable_simplified;
                                        for (ExPolygon &poly : bridgeable) {
                                            poly.simplify(perimeter_spacing/4, &bridgeable_simplified);
                                        }
                                        //offset by perimeter spacing because the simplify may have reduced it a bit.
                                        //it's not dangerous as it will be intersected by 'unsupported' later
                                        to_draw.insert(to_draw.end(), bridgeable.begin(), bridgeable.end());
                                        // add overlap (perimeter_spacing/4 was good in test, ie 25%)
                                        coord_t overlap = scale_(this->config->get_abs_value("infill_overlap", perimeter_spacing));
                                        unsupported_filtered = intersection_ex(unsupported_filtered, offset_ex(bridgeable_simplified, overlap));
                                    } else {
                                        unsupported_filtered.clear();
                                    }
                                } else {
                                    unsupported_filtered.clear();
                                }
                            } else {
                                //only consider the part that can be 'bridged' (inside the convex hull)
                                // it's not as precise as the bridge detector, but it's better than nothing, and quicker.
                            ExPolygonCollection coll_last(support);
                            ExPolygon hull;
                            hull.contour = coll_last.convex_hull();
                                unsupported_filtered = intersection_ex(unsupported_filtered, ExPolygons() = { hull });
                            }
                            if (!unsupported_filtered.empty()) {
                                        //to_draw.insert(to_draw.end(), detector._anchor_regions.begin(), detector._anchor_regions.end());
                                //and we want at least 1 perimeter of overlap
                                ExPolygons bridge = unsupported_filtered;
                                unsupported_filtered = intersection_ex(offset_ex(unsupported_filtered, (float)(perimeter_spacing)), last);
                                // unsupported need to be offset_ex by -(float)(perimeter_spacing/2) for the hole to be flush
                                ExPolygons supported = diff_ex(last, unsupported_filtered); //offset_ex(unsupported_filtered, -(float)(perimeter_spacing / 2)), true);
                                ExPolygons bridge_and_support = union_ex(bridge, support);
                                //to_draw.insert(to_draw.end(), support.begin(), support.end());
                                // make him flush with perimeter area
                                unsupported_filtered = intersection_ex(offset_ex(unsupported_filtered, (float)(perimeter_spacing / 2)), bridge_and_support);
                                // store the results
                                last = supported;

                                //add this directly to the infill list.
                                // this will avoid to throw wrong offsets into a good polygons
                                this->fill_surfaces->append(
                                    unsupported_filtered,
                                    stInternal);
                            }
                        }
                    }
                }

                // We can add more perimeters if there are uncovered overhangs
                // improvement for future: find a way to add perimeters only where it's needed.
                // It's hard to do, so here is a simple version.
                bool may_add_more_perimeters = false;
                if (this->config->extra_perimeters && i > loop_number && !last.empty()
                    && this->lower_slices != NULL && !this->lower_slices->expolygons.empty()){
                    //split the polygons with bottom/notbottom
                    ExPolygons unsupported = diff_ex(last, this->lower_slices->expolygons, true);
                    if (!unsupported.empty()) {
                        //only consider overhangs and let bridges alone
                        if (true) {
                            //only consider the part that can be bridged (really, by the bridge algorithm)
                            //first, separate into islands (ie, each ExPlolygon)
                            int numploy = 0;
                            //only consider the bottom layer that intersect unsupported, to be sure it's only on our island.
                            ExPolygonCollection lower_island(diff_ex(last, unsupported, true));
                            BridgeDetector detector(unsupported,
                                lower_island,
                                perimeter_spacing);
                            if (detector.detect_angle(Geometry::deg2rad(this->config->bridge_angle.value))) {
                                ExPolygons bridgeable = union_ex(detector.coverage(-1, true));
                                if (!bridgeable.empty()) {
                                    //simplify to avoid most of artefacts from printing lines.
                                    ExPolygons bridgeable_simplified;
                                    for (ExPolygon &poly : bridgeable) {
                                        poly.simplify(perimeter_spacing / 2, &bridgeable_simplified);
                                    }
                                    
                                    if (!bridgeable_simplified.empty())
                                        bridgeable_simplified = offset_ex(bridgeable_simplified, perimeter_spacing/1.9);
                                    if (!bridgeable_simplified.empty()) {
                                        //offset by perimeter spacing because the simplify may have reduced it a bit.
                                        unsupported = diff_ex(unsupported, bridgeable_simplified, true);
                                    }
                                } 
                            }
                        } else {
                            ExPolygonCollection coll_last(intersection_ex(last, offset_ex(this->lower_slices->expolygons, -perimeter_spacing / 2)));
                            ExPolygon hull;
                            hull.contour = coll_last.convex_hull();
                            unsupported = diff_ex(offset_ex(unsupported, perimeter_spacing), ExPolygons() = { hull });
                        }
                        if (!unsupported.empty()) {
                            //add fake perimeters here
                            may_add_more_perimeters = true;
                        }
                    }
                }

                // Calculate next onion shell of perimeters.
                //this variable stored the nexyt onion
                ExPolygons next_onion;
                if (i == 0) {
                    // the minimum thickness of a single loop is:
                    // ext_width/2 + ext_spacing/2 + spacing/2 + width/2
                    next_onion = this->config->thin_walls ?
                        offset2_ex(
                            last,
                            -(float)(ext_perimeter_width / 2 + ext_min_spacing / 2 - 1),
                            +(float)(ext_min_spacing / 2 - 1)) :
                            offset_ex(last, -(float)(ext_perimeter_width / 2));

                    // look for thin walls
                    if (this->config->thin_walls) {
                        // the following offset2 ensures almost nothing in @thin_walls is narrower than $min_width
                        // (actually, something larger than that still may exist due to mitering or other causes)
                        coord_t min_width = (coord_t)scale_(this->ext_perimeter_flow.nozzle_diameter / 3);
                        
                        Polygons no_thin_zone = offset(next_onion, (float)(ext_perimeter_width / 2));
                        ExPolygons expp = offset2_ex(
                            // medial axis requires non-overlapping geometry
                            diff_ex(to_polygons(last),
                                    no_thin_zone,
                                    true),
                                    (float)(-min_width / 2), (float)(min_width / 2));
                        // compute a bit of overlap to anchor thin walls inside the print.
                        ExPolygons anchor = intersection_ex(to_polygons(offset_ex(expp, (float)(ext_perimeter_width / 2))), no_thin_zone, true);
                        for (ExPolygon &ex : expp) {
                            ExPolygons bounds = union_ex(ExPolygons() = { ex }, anchor, true);
                            for (ExPolygon &bound : bounds) {
                                if (!intersection_ex(ex, bound).empty()) {
                                    // the maximum thickness of our thin wall area is equal to the minimum thickness of a single loop
                                    ex.medial_axis(bound, ext_perimeter_width + ext_perimeter_spacing2, min_width, &thin_walls);
                                    continue;
                                }
                            }
                        }
                    }
                } else {
                    //FIXME Is this offset correct if the line width of the inner perimeters differs
                    // from the line width of the infill?
                    coord_t distance = (i == 1) ? ext_perimeter_spacing2 : perimeter_spacing;
                    if (this->config->thin_walls){
                        // This path will ensure, that the perimeters do not overfill, as in 
                        // prusa3d/Slic3r GH #32, but with the cost of rounding the perimeters
                        // excessively, creating gaps, which then need to be filled in by the not very 
                        // reliable gap fill algorithm.
                        // Also the offset2(perimeter, -x, x) may sometimes lead to a perimeter, which is larger than
                        // the original.
                        next_onion = offset2_ex(last,
                            -(float)(distance + min_spacing / 2 - 1),
                            +(float)(min_spacing / 2 - 1));
                    } else {
                        // If "detect thin walls" is not enabled, this paths will be entered, which 
                        // leads to overflows, as in prusa3d/Slic3r GH #32
                        next_onion = offset_ex(last, -(float)(distance));
                    }
                    // look for gaps
                    if (this->config->gap_fill_speed.value > 0 && this->config->fill_density.value > 0)
                        // not using safety offset here would "detect" very narrow gaps
                        // (but still long enough to escape the area threshold) that gap fill
                        // won't be able to fill but we'd still remove from infill area
                        append(gaps, diff_ex(
                            offset(last, -0.5f*distance),
                            offset(next_onion, 0.5f * distance + 10)));  // safety offset
                }

                if (next_onion.empty()) {
                    // Store the number of loops actually generated.
                    loop_number = i - 1;
                    // No region left to be filled in.
                    last.clear();
                    break;
                } else if (i > loop_number) {
                    if (may_add_more_perimeters) {
                        loop_number++;
                        contours.emplace_back();
                        holes.emplace_back();
                    } else {
                        // If i > loop_number, we were looking just for gaps.
                        break;
                    }
                }

                for (const ExPolygon &expolygon : next_onion) {
                    contours[i].emplace_back(PerimeterGeneratorLoop(expolygon.contour, i, true));
                    if (! expolygon.holes.empty()) {
                        holes[i].reserve(holes[i].size() + expolygon.holes.size());
                        for (const Polygon &hole : expolygon.holes)
                            holes[i].emplace_back(PerimeterGeneratorLoop(hole, i, false));
                    }
                }
                last = std::move(next_onion);
                    
                //store surface for top infill if only_one_perimeter_top
                if(i==0 && config->only_one_perimeter_top && this->upper_slices != NULL){
                    //split the polygons with top/not_top
                    ExPolygons upper_polygons(this->upper_slices->expolygons);
                    ExPolygons top_polygons = diff_ex(last, (upper_polygons), true);
                    ExPolygons inner_polygons = diff_ex(last, top_polygons, true);
                    // increase a bit the inner space to fill the frontier between last and stored.
                    stored = union_ex(stored, intersection_ex(offset_ex(top_polygons, (float)(perimeter_spacing / 2)), last));
                    last = intersection_ex(offset_ex(inner_polygons, (float)(perimeter_spacing / 2)), last);
                }

                

            }
            
            // re-add stored polygons
            last = union_ex(last, stored);

            // nest loops: holes first
            for (int d = 0; d <= loop_number; ++d) {
                PerimeterGeneratorLoops &holes_d = holes[d];
                // loop through all holes having depth == d
                for (int i = 0; i < (int)holes_d.size(); ++i) {
                    const PerimeterGeneratorLoop &loop = holes_d[i];
                    // find the hole loop that contains this one, if any
                    for (int t = d+1; t <= loop_number; ++t) {
                        for (int j = 0; j < (int)holes[t].size(); ++j) {
                            PerimeterGeneratorLoop &candidate_parent = holes[t][j];
                            if (candidate_parent.polygon.contains(loop.polygon.first_point())) {
                                candidate_parent.children.push_back(loop);
                                holes_d.erase(holes_d.begin() + i);
                                --i;
                                goto NEXT_LOOP;
                            }
                        }
                    }
                    // if no hole contains this hole, find the contour loop that contains it
                    for (int t = loop_number; t >= 0; --t) {
                        for (int j = 0; j < (int)contours[t].size(); ++j) {
                            PerimeterGeneratorLoop &candidate_parent = contours[t][j];
                            if (candidate_parent.polygon.contains(loop.polygon.first_point())) {
                                candidate_parent.children.push_back(loop);
                                holes_d.erase(holes_d.begin() + i);
                                --i;
                                goto NEXT_LOOP;
                            }
                        }
                    }
                    NEXT_LOOP: ;
                }
            }
            // nest contour loops
            for (int d = loop_number; d >= 1; --d) {
                PerimeterGeneratorLoops &contours_d = contours[d];
                // loop through all contours having depth == d
                for (int i = 0; i < (int)contours_d.size(); ++i) {
                    const PerimeterGeneratorLoop &loop = contours_d[i];
                    // find the contour loop that contains it
                    for (int t = d-1; t >= 0; --t) {
                        for (int j = 0; j < contours[t].size(); ++j) {
                            PerimeterGeneratorLoop &candidate_parent = contours[t][j];
                            if (candidate_parent.polygon.contains(loop.polygon.first_point())) {
                                candidate_parent.children.push_back(loop);
                                contours_d.erase(contours_d.begin() + i);
                                --i;
                                goto NEXT_CONTOUR;
                            }
                        }
                    }
                    NEXT_CONTOUR: ;
                }
            }
            // at this point, all loops should be in contours[0]
            ExtrusionEntityCollection entities = this->_traverse_loops(contours.front(), thin_walls);
            // if brim will be printed, reverse the order of perimeters so that
            // we continue inwards after having finished the brim
            // TODO: add test for perimeter order
            if (this->config->external_perimeters_first || 
                (this->layer_id == 0 && this->print_config->brim_width.value > 0))
                    entities.reverse();
            // append perimeters for this slice as a collection
            if (!entities.empty())
                this->loops->append(entities);
        } // for each loop of an island

        // fill gaps
        if (!gaps.empty()) {
            // collapse 
            double min = 0.2 * perimeter_width * (1 - INSET_OVERLAP_TOLERANCE);
            double max = 2. * perimeter_spacing;
            ExPolygons gaps_ex = diff_ex(
                //FIXME offset2 would be enough and cheaper.
                offset2_ex(gaps, -min/2, +min/2),
                offset2_ex(gaps, -max/2, +max/2),
                true);
            ThickPolylines polylines;
            for (const ExPolygon &ex : gaps_ex)
                ex.medial_axis(ex, max, min, &polylines);
            if (!polylines.empty()) {
                ExtrusionEntityCollection gap_fill = this->_variable_width(polylines, 
                    erGapFill, this->solid_infill_flow);
                this->gap_fill->append(gap_fill.entities);
                /*  Make sure we don't infill narrow parts that are already gap-filled
                    (we only consider this surface's gaps to reduce the diff() complexity).
                    Growing actual extrusions ensures that gaps not filled by medial axis
                    are not subtracted from fill surfaces (they might be too short gaps
                    that medial axis skips but infill might join with other infill regions
                    and use zigzag).  */
                //FIXME Vojtech: This grows by a rounded extrusion width, not by line spacing,
                // therefore it may cover the area, but no the volume.
                last = diff_ex(to_polygons(last), gap_fill.polygons_covered_by_width(10.f));
            }
        }

        // create one more offset to be used as boundary for fill
        // we offset by half the perimeter spacing (to get to the actual infill boundary)
        // and then we offset back and forth by half the infill spacing to only consider the
        // non-collapsing regions
        coord_t inset = 
            (loop_number < 0) ? 0 :
            (loop_number == 0) ?
            // one loop
                ext_perimeter_spacing / 2 :
                // two or more loops?
                perimeter_spacing / 2;
        // only apply infill overlap if we actually have one perimeter
        if (inset > 0)
            inset -= scale_(this->config->get_abs_value("infill_overlap", unscale(inset + solid_infill_spacing / 2)));
        // simplify infill contours according to resolution
        Polygons pp;
        for (ExPolygon &ex : last)
            ex.simplify_p(SCALED_RESOLUTION, &pp);
        // collapse too narrow infill areas
        coord_t min_perimeter_infill_spacing = solid_infill_spacing * (1. - INSET_OVERLAP_TOLERANCE);
        // append infill areas to fill_surfaces
        this->fill_surfaces->append(
            offset2_ex(
                union_ex(pp),
                -inset -min_perimeter_infill_spacing/2,
                min_perimeter_infill_spacing / 2),
            stInternal);
    } // for each island
}

ExtrusionEntityCollection PerimeterGenerator::_traverse_loops(
    const PerimeterGeneratorLoops &loops, ThickPolylines &thin_walls) const
{
    // loops is an arrayref of ::Loop objects
    // turn each one into an ExtrusionLoop object
    ExtrusionEntityCollection coll;
    for (PerimeterGeneratorLoops::const_iterator loop = loops.begin();
        loop != loops.end(); ++loop) {
        bool is_external = loop->is_external();
        
        ExtrusionRole role;
        ExtrusionLoopRole loop_role;
        role = is_external ? erExternalPerimeter : erPerimeter;
        if (loop->is_internal_contour()) {
            // Note that we set loop role to ContourInternalPerimeter
            // also when loop is both internal and external (i.e.
            // there's only one contour loop).
            loop_role = elrContourInternalPerimeter;
        } else {
            loop_role = elrDefault;
        }
        
        // detect overhanging/bridging perimeters
        ExtrusionPaths paths;
        if (this->config->overhangs && this->layer_id > 0
            && !(this->object_config->support_material && this->object_config->support_material_contact_distance.value == 0)) {
            // get non-overhang paths by intersecting this loop with the grown lower slices
            extrusion_paths_append(
                paths,
                intersection_pl(loop->polygon, this->_lower_slices_p),
                role,
                is_external ? this->_ext_mm3_per_mm           : this->_mm3_per_mm,
                is_external ? this->ext_perimeter_flow.width  : this->perimeter_flow.width,
                this->layer_height);
            
            // get overhang paths by checking what parts of this loop fall 
            // outside the grown lower slices (thus where the distance between
            // the loop centerline and original lower slices is >= half nozzle diameter
            extrusion_paths_append(
                paths,
                diff_pl(loop->polygon, this->_lower_slices_p),
                erOverhangPerimeter,
                this->_mm3_per_mm_overhang,
                this->overhang_flow.width,
                this->overhang_flow.height);
            
            // reapply the nearest point search for starting point
            // We allow polyline reversal because Clipper may have randomly
            // reversed polylines during clipping.
            paths = (ExtrusionPaths)ExtrusionEntityCollection(paths).chained_path();
        } else {
            ExtrusionPath path(role);
            path.polyline   = loop->polygon.split_at_first_point();
            path.mm3_per_mm = is_external ? this->_ext_mm3_per_mm           : this->_mm3_per_mm;
            path.width      = is_external ? this->ext_perimeter_flow.width  : this->perimeter_flow.width;
            path.height     = this->layer_height;
            paths.push_back(path);
        }
        
        coll.append(ExtrusionLoop(paths, loop_role));
    }
    
    // append thin walls to the nearest-neighbor search (only for first iteration)
    if (!thin_walls.empty()) {
        ExtrusionEntityCollection tw = this->_variable_width
            (thin_walls, erExternalPerimeter, this->ext_perimeter_flow);
        
        coll.append(tw.entities);
        thin_walls.clear();
    }
    
    // sort entities into a new collection using a nearest-neighbor search,
    // preserving the original indices which are useful for detecting thin walls
    ExtrusionEntityCollection sorted_coll;
    coll.chained_path(&sorted_coll, false, erMixed, &sorted_coll.orig_indices);
    
    // traverse children and build the final collection
    ExtrusionEntityCollection entities;
    for (std::vector<size_t>::const_iterator idx = sorted_coll.orig_indices.begin();
        idx != sorted_coll.orig_indices.end();
        ++idx) {
        
        if (*idx >= loops.size()) {
            // this is a thin wall
            // let's get it from the sorted collection as it might have been reversed
            size_t i = idx - sorted_coll.orig_indices.begin();
            entities.append(*sorted_coll.entities[i]);
        } else {
            const PerimeterGeneratorLoop &loop = loops[*idx];
            ExtrusionLoop eloop = *dynamic_cast<ExtrusionLoop*>(coll.entities[*idx]);
            
            ExtrusionEntityCollection children = this->_traverse_loops(loop.children, thin_walls);
            if (loop.is_contour) {
                eloop.make_counter_clockwise();
                entities.append(children.entities);
                entities.append(eloop);
            } else {
                eloop.make_clockwise();
                entities.append(eloop);
                entities.append(children.entities);
            }
        }
    }
    return entities;
}

ExtrusionEntityCollection PerimeterGenerator::_variable_width(const ThickPolylines &polylines, ExtrusionRole role, Flow flow) const
{
    // this value determines granularity of adaptive width, as G-code does not allow
    // variable extrusion within a single move; this value shall only affect the amount
    // of segments, and any pruning shall be performed before we apply this tolerance
    const double tolerance = scale_(0.05);
    
    ExtrusionEntityCollection coll;
    for (const ThickPolyline &p : polylines) {
        ExtrusionPaths paths;
        ExtrusionPath path(role);
        ThickLines lines = p.thicklines();
        
        for (int i = 0; i < (int)lines.size(); ++i) {
            const ThickLine& line = lines[i];
            
            const coordf_t line_len = line.length();
            if (line_len < SCALED_EPSILON) continue;
            
            double thickness_delta = fabs(line.a_width - line.b_width);
            if (thickness_delta > tolerance) {
                const unsigned short segments = ceil(thickness_delta / tolerance);
                const coordf_t seg_len = line_len / segments;
                Points pp;
                std::vector<coordf_t> width;
                {
                    pp.push_back(line.a);
                    width.push_back(line.a_width);
                    for (size_t j = 1; j < segments; ++j) {
                        pp.push_back(line.point_at(j*seg_len));
                        
                        coordf_t w = line.a_width + (j*seg_len) * (line.b_width-line.a_width) / line_len;
                        width.push_back(w);
                        width.push_back(w);
                    }
                    pp.push_back(line.b);
                    width.push_back(line.b_width);
                    
                    assert(pp.size() == segments + 1);
                    assert(width.size() == segments*2);
                }
                
                // delete this line and insert new ones
                lines.erase(lines.begin() + i);
                for (size_t j = 0; j < segments; ++j) {
                    ThickLine new_line(pp[j], pp[j+1]);
                    new_line.a_width = width[2*j];
                    new_line.b_width = width[2*j+1];
                    lines.insert(lines.begin() + i + j, new_line);
                }
                
                --i;
                continue;
            }
            
            const double w = fmax(line.a_width, line.b_width);
            if (path.polyline.points.empty()) {
                path.polyline.append(line.a);
                path.polyline.append(line.b);
                // Convert from spacing to extrusion width based on the extrusion model
                // of a square extrusion ended with semi circles.
                flow.width = unscale(w) + flow.height * (1. - 0.25 * PI);
                #ifdef SLIC3R_DEBUG
                printf("  filling %f gap\n", flow.width);
                #endif
                path.mm3_per_mm  = flow.mm3_per_mm();
                path.width       = flow.width;
                path.height      = flow.height;
            } else {
                thickness_delta = fabs(scale_(flow.width) - w);
                if (thickness_delta <= tolerance) {
                    // the width difference between this line and the current flow width is 
                    // within the accepted tolerance
                    path.polyline.append(line.b);
                } else {
                    // we need to initialize a new line
                    paths.emplace_back(std::move(path));
                    path = ExtrusionPath(role);
                    --i;
                }
            }
        }
        if (path.polyline.is_valid())
            paths.emplace_back(std::move(path));        
        // Append paths to collection.
        if (!paths.empty()) {
            if (paths.front().first_point().coincides_with(paths.back().last_point())) {
                coll.append(ExtrusionLoop(paths));
            } else {
                //not a loop : avoid to "sort" it.
                ExtrusionEntityCollection unsortable_coll(paths);
                unsortable_coll.no_sort = true;
                coll.append(unsortable_coll);
            }
        }
    }

    return coll;
}

bool PerimeterGeneratorLoop::is_internal_contour() const
{
    // An internal contour is a contour containing no other contours
    if (! this->is_contour)
                return false;
    for (const PerimeterGeneratorLoop &loop : this->children)
        if (loop.is_contour)
            return false;
        return true;
    }

}

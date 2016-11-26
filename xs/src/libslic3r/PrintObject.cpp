#include "Print.hpp"
#include "BoundingBox.hpp"
#include "ClipperUtils.hpp"
#include "Geometry.hpp"

namespace Slic3r {

PrintObject::PrintObject(Print* print, ModelObject* model_object, const BoundingBoxf3 &modobj_bbox)
:   typed_slices(false),
    _print(print),
    _model_object(model_object)
{
    // Compute the translation to be applied to our meshes so that we work with smaller coordinates
    {
        // Translate meshes so that our toolpath generation algorithms work with smaller
        // XY coordinates; this translation is an optimization and not strictly required.
        // A cloned mesh will be aligned to 0 before slicing in _slice_region() since we
        // don't assume it's already aligned and we don't alter the original position in model.
        // We store the XY translation so that we can place copies correctly in the output G-code
        // (copies are expressed in G-code coordinates and this translation is not publicly exposed).
        this->_copies_shift = Point(
            scale_(modobj_bbox.min.x), scale_(modobj_bbox.min.y));

        // Scale the object size and store it
        Pointf3 size = modobj_bbox.size();
        this->size = Point3(scale_(size.x), scale_(size.y), scale_(size.z));
    }
    
    this->reload_model_instances();
    this->layer_height_ranges = model_object->layer_height_ranges;
}

PrintObject::~PrintObject()
{
}

Print*
PrintObject::print()
{
    return this->_print;
}

ModelObject*
PrintObject::model_object()
{
    return this->_model_object;
}

Points
PrintObject::copies() const
{
    return this->_copies;
}

bool
PrintObject::add_copy(const Pointf &point)
{
    Points points = this->_copies;
    points.push_back(Point::new_scale(point.x, point.y));
    return this->set_copies(points);
}

bool
PrintObject::delete_last_copy()
{
    Points points = this->_copies;
    points.pop_back();
    return this->set_copies(points);
}

bool
PrintObject::delete_all_copies()
{
    Points points;
    return this->set_copies(points);
}

bool
PrintObject::set_copies(const Points &points)
{
    this->_copies = points;
    
    // order copies with a nearest neighbor search and translate them by _copies_shift
    this->_shifted_copies.clear();
    this->_shifted_copies.reserve(points.size());
    
    // order copies with a nearest-neighbor search
    std::vector<Points::size_type> ordered_copies;
    Slic3r::Geometry::chained_path(points, ordered_copies);
    
    for (std::vector<Points::size_type>::const_iterator it = ordered_copies.begin(); it != ordered_copies.end(); ++it) {
        Point copy = points[*it];
        copy.translate(this->_copies_shift);
        this->_shifted_copies.push_back(copy);
    }
    
    bool invalidated = false;
    if (this->_print->invalidate_step(psSkirt)) invalidated = true;
    if (this->_print->invalidate_step(psBrim)) invalidated = true;
    return invalidated;
}

bool
PrintObject::reload_model_instances()
{
    Points copies;
    for (ModelInstancePtrs::const_iterator i = this->_model_object->instances.begin(); i != this->_model_object->instances.end(); ++i) {
        copies.push_back(Point::new_scale((*i)->offset.x, (*i)->offset.y));
    }
    return this->set_copies(copies);
}

BoundingBox
PrintObject::bounding_box() const
{
    // since the object is aligned to origin, bounding box coincides with size
    Points pp;
    pp.push_back(Point(0,0));
    pp.push_back(this->size);
    return BoundingBox(pp);
}

void
PrintObject::add_region_volume(int region_id, int volume_id)
{
    region_volumes[region_id].push_back(volume_id);
}

/*  This is the *total* layer count (including support layers)
    this value is not supposed to be compared with Layer::id
    since they have different semantics */
size_t
PrintObject::total_layer_count() const
{
    return this->layer_count() + this->support_layer_count();
}

size_t
PrintObject::layer_count() const
{
    return this->layers.size();
}

void
PrintObject::clear_layers()
{
    for (int i = this->layers.size()-1; i >= 0; --i)
        this->delete_layer(i);
}

Layer*
PrintObject::get_layer(int idx)
{
    return this->layers.at(idx);
}

Layer*
PrintObject::add_layer(int id, coordf_t height, coordf_t print_z, coordf_t slice_z)
{
    Layer* layer = new Layer(id, this, height, print_z, slice_z);
    layers.push_back(layer);
    return layer;
}

void
PrintObject::delete_layer(int idx)
{
    LayerPtrs::iterator i = this->layers.begin() + idx;
    delete *i;
    this->layers.erase(i);
}

size_t
PrintObject::support_layer_count() const
{
    return this->support_layers.size();
}

void
PrintObject::clear_support_layers()
{
    for (int i = this->support_layers.size()-1; i >= 0; --i)
        this->delete_support_layer(i);
}

SupportLayer*
PrintObject::get_support_layer(int idx)
{
    return this->support_layers.at(idx);
}

SupportLayer*
PrintObject::add_support_layer(int id, coordf_t height, coordf_t print_z)
{
    SupportLayer* layer = new SupportLayer(id, this, height, print_z, -1);
    support_layers.push_back(layer);
    return layer;
}

void
PrintObject::delete_support_layer(int idx)
{
    SupportLayerPtrs::iterator i = this->support_layers.begin() + idx;
    delete *i;
    this->support_layers.erase(i);
}

bool
PrintObject::invalidate_state_by_config_options(const std::vector<t_config_option_key> &opt_keys)
{
    std::set<PrintObjectStep> steps;
    
    // this method only accepts PrintObjectConfig and PrintRegionConfig option keys
    for (std::vector<t_config_option_key>::const_iterator opt_key = opt_keys.begin(); opt_key != opt_keys.end(); ++opt_key) {
        if (*opt_key == "perimeters"
            || *opt_key == "extra_perimeters"
            || *opt_key == "gap_fill_speed"
            || *opt_key == "overhangs"
            || *opt_key == "first_layer_extrusion_width"
            || *opt_key == "perimeter_extrusion_width"
            || *opt_key == "infill_overlap"
            || *opt_key == "thin_walls"
            || *opt_key == "external_perimeters_first") {
            steps.insert(posPerimeters);
        } else if (*opt_key == "layer_height"
            || *opt_key == "first_layer_height"
            || *opt_key == "xy_size_compensation"
            || *opt_key == "raft_layers") {
            steps.insert(posSlice);
        } else if (*opt_key == "support_material"
            || *opt_key == "support_material_angle"
            || *opt_key == "support_material_extruder"
            || *opt_key == "support_material_extrusion_width"
            || *opt_key == "support_material_interface_layers"
            || *opt_key == "support_material_interface_extruder"
            || *opt_key == "support_material_interface_spacing"
            || *opt_key == "support_material_interface_speed"
            || *opt_key == "support_material_pattern"
            || *opt_key == "support_material_spacing"
            || *opt_key == "support_material_threshold"
            || *opt_key == "dont_support_bridges"
            || *opt_key == "first_layer_extrusion_width") {
            steps.insert(posSupportMaterial);
        } else if (*opt_key == "interface_shells"
            || *opt_key == "infill_only_where_needed"
            || *opt_key == "infill_every_layers"
            || *opt_key == "solid_infill_every_layers"
            || *opt_key == "bottom_solid_layers"
            || *opt_key == "top_solid_layers"
            || *opt_key == "solid_infill_below_area"
            || *opt_key == "infill_extruder"
            || *opt_key == "solid_infill_extruder"
            || *opt_key == "infill_extrusion_width") {
            steps.insert(posPrepareInfill);
        } else if (*opt_key == "external_fill_pattern"
            || *opt_key == "fill_angle"
            || *opt_key == "fill_pattern"
            || *opt_key == "top_infill_extrusion_width"
            || *opt_key == "first_layer_extrusion_width") {
            steps.insert(posInfill);
        } else if (*opt_key == "fill_density"
            || *opt_key == "solid_infill_extrusion_width") {
            steps.insert(posPerimeters);
            steps.insert(posPrepareInfill);
        } else if (*opt_key == "external_perimeter_extrusion_width"
            || *opt_key == "perimeter_extruder") {
            steps.insert(posPerimeters);
            steps.insert(posSupportMaterial);
        } else if (*opt_key == "bridge_flow_ratio") {
            steps.insert(posPerimeters);
            steps.insert(posInfill);
        } else if (*opt_key == "seam_position"
            || *opt_key == "support_material_speed"
            || *opt_key == "bridge_speed"
            || *opt_key == "external_perimeter_speed"
            || *opt_key == "infill_speed"
            || *opt_key == "perimeter_speed"
            || *opt_key == "small_perimeter_speed"
            || *opt_key == "solid_infill_speed"
            || *opt_key == "top_solid_infill_speed") {
            // these options only affect G-code export, so nothing to invalidate
        } else {
            // for legacy, if we can't handle this option let's invalidate all steps
            return this->invalidate_all_steps();
        }
    }
    
    bool invalidated = false;
    for (std::set<PrintObjectStep>::const_iterator step = steps.begin(); step != steps.end(); ++step) {
        if (this->invalidate_step(*step)) invalidated = true;
    }
    
    return invalidated;
}

bool
PrintObject::invalidate_step(PrintObjectStep step)
{
    bool invalidated = this->state.invalidate(step);
    
    // propagate to dependent steps
    if (step == posPerimeters) {
        this->invalidate_step(posPrepareInfill);
        this->_print->invalidate_step(psSkirt);
        this->_print->invalidate_step(psBrim);
    } else if (step == posPrepareInfill) {
        this->invalidate_step(posInfill);
    } else if (step == posInfill) {
        this->_print->invalidate_step(psSkirt);
        this->_print->invalidate_step(psBrim);
    } else if (step == posSlice) {
        this->invalidate_step(posPerimeters);
        this->invalidate_step(posSupportMaterial);
    } else if (step == posSupportMaterial) {
        this->_print->invalidate_step(psSkirt);
        this->_print->invalidate_step(psBrim);
    }
    
    return invalidated;
}

bool
PrintObject::invalidate_all_steps()
{
    // make a copy because when invalidating steps the iterators are not working anymore
    std::set<PrintObjectStep> steps = this->state.started;
    
    bool invalidated = false;
    for (std::set<PrintObjectStep>::const_iterator step = steps.begin(); step != steps.end(); ++step) {
        if (this->invalidate_step(*step)) invalidated = true;
    }
    return invalidated;
}

bool
PrintObject::has_support_material() const
{
    return this->config.support_material
        || this->config.raft_layers > 0
        || this->config.support_material_enforce_layers > 0;
}

// This function analyzes slices of a region (SurfaceCollection slices).
// Each region slice (instance of Surface) is analyzed, whether it is supported or whether it is the top surface.
// Initially all slices are of type S_TYPE_INTERNAL.
// Slices are compared against the top / bottom slices and regions and classified to the following groups:
// S_TYPE_TOP - Part of a region, which is not covered by any upper layer. This surface will be filled with a top solid infill.
// S_TYPE_BOTTOMBRIDGE - Part of a region, which is not fully supported, but it hangs in the air, or it hangs losely on a support or a raft.
// S_TYPE_BOTTOM - Part of a region, which is not supported by the same region, but it is supported either by another region, or by a soluble interface layer.
// S_TYPE_INTERNAL - Part of a region, which is supported by the same region type.
// If a part of a region is of S_TYPE_BOTTOM and S_TYPE_TOP, the S_TYPE_BOTTOM wins.
void
PrintObject::detect_surfaces_type()
{
    //Slic3r::debugf "Detecting solid surfaces...\n";
    FOREACH_REGION(this->_print, region) {
        size_t region_id = region - this->_print->regions.begin();
        
        FOREACH_LAYER(this, layer_it) {
            size_t layer_idx    = layer_it - this->layers.begin();
            Layer &layer        = **layer_it;
            LayerRegion &layerm = *layer.get_region(region_id);
            // comparison happens against the *full* slices (considering all regions)
            // unless internal shells are requested
            
            const Layer* upper_layer = layer_idx < (this->layer_count()-1) ? this->get_layer(layer_idx+1) : NULL;
            const Layer* lower_layer = layer_idx > 0 ? this->get_layer(layer_idx-1) : NULL;
            
            // collapse very narrow parts (using the safety offset in the diff is not enough)
            const float offset = layerm.flow(frExternalPerimeter).scaled_width() / 10.f;

            const Polygons layerm_slices_surfaces = layerm.slices;

            // find top surfaces (difference between current surfaces
            // of current layer and upper one)
            SurfaceCollection top;
            if (upper_layer != NULL) {
                const Polygons upper_slices = this->config.interface_shells.value
                    ? (Polygons)upper_layer->get_region(region_id)->slices
                    : (Polygons)upper_layer->slices;
                
                top.append(
                    offset2_ex(
                        diff(layerm_slices_surfaces, upper_slices, true),
                        -offset, offset
                    ),
                    stTop
                );
            } else {
                // if no upper layer, all surfaces of this one are solid
                // we clone surfaces because we're going to clear the slices collection
                top = layerm.slices;
                for (Surfaces::iterator it = top.surfaces.begin(); it != top.surfaces.end(); ++ it)
                    it->surface_type = stTop;
            }
            
            // find bottom surfaces (difference between current surfaces
            // of current layer and lower one)
            SurfaceCollection bottom;
            if (lower_layer != NULL) {
                // If we have soluble support material, don't bridge. The overhang will be squished against a soluble layer separating
                // the support from the print.
                const SurfaceType surface_type_bottom = 
                    (this->config.support_material.value && this->config.support_material_contact_distance.value == 0)
                    ? stBottom
                    : stBottomBridge;
                
                // Any surface lying on the void is a true bottom bridge (an overhang)
                bottom.append(
                    offset2_ex(
                        diff(layerm_slices_surfaces, lower_layer->slices, true), 
                        -offset, offset
                    ),
                    surface_type_bottom
                );
                
                // if user requested internal shells, we need to identify surfaces
                // lying on other slices not belonging to this region
                if (this->config.interface_shells) {
                    // non-bridging bottom surfaces: any part of this layer lying 
                    // on something else, excluding those lying on our own region
                    bottom.append(
                        offset2_ex(
                            diff(
                                intersection(layerm_slices_surfaces, lower_layer->slices), // supported
                                lower_layer->get_region(region_id)->slices, 
                                true
                            ), 
                            -offset, offset
                        ),
                        stBottom
                    );
                }
            } else {
                // if no lower layer, all surfaces of this one are solid
                // we clone surfaces because we're going to clear the slices collection
                bottom = layerm.slices;
                
                // if we have raft layers, consider bottom layer as a bridge
                // just like any other bottom surface lying on the void
                const SurfaceType surface_type_bottom = 
                    (this->config.raft_layers.value > 0 && this->config.support_material_contact_distance.value > 0)
                    ? stBottomBridge
                    : stBottom;
                for (Surfaces::iterator it = bottom.surfaces.begin(); it != bottom.surfaces.end(); ++ it)
                    it->surface_type = surface_type_bottom;
            }
            
            // now, if the object contained a thin membrane, we could have overlapping bottom
            // and top surfaces; let's do an intersection to discover them and consider them
            // as bottom surfaces (to allow for bridge detection)
            if (!top.empty() && !bottom.empty()) {
                const Polygons top_polygons = to_polygons(STDMOVE(top));
                top.clear();
                top.append(
                    offset2_ex(diff(top_polygons, bottom, true), -offset, offset),
                    stTop
                );
            }
            
            // save surfaces to layer
            layerm.slices.clear();
            layerm.slices.append(STDMOVE(top));
            layerm.slices.append(STDMOVE(bottom));

            // find internal surfaces (difference between top/bottom surfaces and others)
            {
                Polygons topbottom = top; append_to(topbottom, (Polygons)bottom);
                
                layerm.slices.append(
                    offset2_ex(
                        diff(layerm_slices_surfaces, topbottom, true),
                        -offset, offset
                    ),
                    stInternal
                );
            }
            
            /*
            Slic3r::debugf "  layer %d has %d bottom, %d top and %d internal surfaces\n",
                $layerm->layer->id, scalar(@bottom), scalar(@top), scalar(@internal) if $Slic3r::debug;
            */

        } // for each layer of a region
        
        /*  Fill in layerm->fill_surfaces by trimming the layerm->slices by the cummulative layerm->fill_surfaces.
            Note: this method should be idempotent, but fill_surfaces gets modified 
            in place. However we're now only using its boundaries (which are invariant)
            so we're safe. This guarantees idempotence of prepare_infill() also in case
            that combine_infill() turns some fill_surface into VOID surfaces.  */
        FOREACH_LAYER(this, layer_it) {
            LayerRegion &layerm = *(*layer_it)->get_region(region_id);
            
            const Polygons fill_boundaries = layerm.fill_surfaces;
            layerm.fill_surfaces.clear();
            for (Surfaces::const_iterator surface = layerm.slices.surfaces.begin();
                surface != layerm.slices.surfaces.end(); ++ surface) {
                layerm.fill_surfaces.append(
                    intersection_ex(*surface, fill_boundaries),
                    surface->surface_type
                );
            }
        }
    }
}

void
PrintObject::process_external_surfaces()
{
    FOREACH_REGION(this->_print, region) {
        size_t region_id = region - this->_print->regions.begin();
        
        FOREACH_LAYER(this, layer_it) {
            const Layer* lower_layer = (layer_it == this->layers.begin())
                ? NULL
                : *(layer_it-1);
            
            (*layer_it)->get_region(region_id)->process_external_surfaces(lower_layer);
        }
    }
}

/* This method applies bridge flow to the first internal solid layer above
   sparse infill */
void
PrintObject::bridge_over_infill()
{
    FOREACH_REGION(this->_print, region) {
        size_t region_id = region - this->_print->regions.begin();
        
        // skip bridging in case there are no voids
        if ((*region)->config.fill_density.value == 100) continue;
        
        // get bridge flow
        Flow bridge_flow = (*region)->flow(
            frSolidInfill,
            -1,     // layer height, not relevant for bridge flow
            true,   // bridge
            false,  // first layer
            -1,     // custom width, not relevant for bridge flow
            *this
        );
        
        FOREACH_LAYER(this, layer_it) {
            // skip first layer
            if (layer_it == this->layers.begin()) continue;
            
            Layer* layer        = *layer_it;
            LayerRegion* layerm = layer->get_region(region_id);
            
            // extract the stInternalSolid surfaces that might be transformed into bridges
            Polygons internal_solid;
            layerm->fill_surfaces.filter_by_type(stInternalSolid, &internal_solid);
            
            // check whether the lower area is deep enough for absorbing the extra flow
            // (for obvious physical reasons but also for preventing the bridge extrudates
            // from overflowing in 3D preview)
            ExPolygons to_bridge;
            {
                Polygons to_bridge_pp = internal_solid;
                
                // iterate through lower layers spanned by bridge_flow
                double bottom_z = layer->print_z - bridge_flow.height;
                for (int i = (layer_it - this->layers.begin()) - 1; i >= 0; --i) {
                    const Layer* lower_layer = this->layers[i];
                    
                    // stop iterating if layer is lower than bottom_z
                    if (lower_layer->print_z < bottom_z) break;
                    
                    // iterate through regions and collect internal surfaces
                    Polygons lower_internal;
                    FOREACH_LAYERREGION(lower_layer, lower_layerm_it)
                        (*lower_layerm_it)->fill_surfaces.filter_by_type(stInternal, &lower_internal);
                    
                    // intersect such lower internal surfaces with the candidate solid surfaces
                    to_bridge_pp = intersection(to_bridge_pp, lower_internal);
                }
                
                // there's no point in bridging too thin/short regions
                {
                    double min_width = bridge_flow.scaled_width() * 3;
                    to_bridge_pp = offset2(to_bridge_pp, -min_width, +min_width);
                }
                
                if (to_bridge_pp.empty()) continue;
                
                // convert into ExPolygons
                to_bridge = union_ex(to_bridge_pp);
            }
            
            #ifdef SLIC3R_DEBUG
            printf("Bridging %zu internal areas at layer %zu\n", to_bridge.size(), layer->id());
            #endif
            
            // compute the remaning internal solid surfaces as difference
            ExPolygons not_to_bridge = diff_ex(internal_solid, to_polygons(to_bridge), true);
            
            // build the new collection of fill_surfaces
            {
                Surfaces new_surfaces;
                for (Surfaces::const_iterator surface = layerm->fill_surfaces.surfaces.begin(); surface != layerm->fill_surfaces.surfaces.end(); ++surface) {
                    if (surface->surface_type != stInternalSolid)
                        new_surfaces.push_back(*surface);
                }
                
                for (ExPolygons::const_iterator ex = to_bridge.begin(); ex != to_bridge.end(); ++ex)
                    new_surfaces.push_back(Surface(stInternalBridge, *ex));
                
                for (ExPolygons::const_iterator ex = not_to_bridge.begin(); ex != not_to_bridge.end(); ++ex)
                    new_surfaces.push_back(Surface(stInternalSolid, *ex));
                
                layerm->fill_surfaces.surfaces = new_surfaces;
            }
            
            /*
            # exclude infill from the layers below if needed
            # see discussion at https://github.com/alexrj/Slic3r/issues/240
            # Update: do not exclude any infill. Sparse infill is able to absorb the excess material.
            if (0) {
                my $excess = $layerm->extruders->{infill}->bridge_flow->width - $layerm->height;
                for (my $i = $layer_id-1; $excess >= $self->get_layer($i)->height; $i--) {
                    Slic3r::debugf "  skipping infill below those areas at layer %d\n", $i;
                    foreach my $lower_layerm (@{$self->get_layer($i)->regions}) {
                        my @new_surfaces = ();
                        # subtract the area from all types of surfaces
                        foreach my $group (@{$lower_layerm->fill_surfaces->group}) {
                            push @new_surfaces, map $group->[0]->clone(expolygon => $_),
                                @{diff_ex(
                                    [ map $_->p, @$group ],
                                    [ map @$_, @$to_bridge ],
                                )};
                            push @new_surfaces, map Slic3r::Surface->new(
                                expolygon       => $_,
                                surface_type    => S_TYPE_INTERNALVOID,
                            ), @{intersection_ex(
                                [ map $_->p, @$group ],
                                [ map @$_, @$to_bridge ],
                            )};
                        }
                        $lower_layerm->fill_surfaces->clear;
                        $lower_layerm->fill_surfaces->append($_) for @new_surfaces;
                    }
                    
                    $excess -= $self->get_layer($i)->height;
                }
            }
            */
        }
    }
}

void
PrintObject::_make_perimeters()
{
    if (this->state.is_done(posPerimeters)) return;
    this->state.set_started(posPerimeters);
    
    // merge slices if they were split into types
    if (this->typed_slices) {
        FOREACH_LAYER(this, layer_it)
            (*layer_it)->merge_slices();
        this->typed_slices = false;
        this->state.invalidate(posPrepareInfill);
    }
    
    // compare each layer to the one below, and mark those slices needing
    // one additional inner perimeter, like the top of domed objects-
    
    // this algorithm makes sure that at least one perimeter is overlapping
    // but we don't generate any extra perimeter if fill density is zero, as they would be floating
    // inside the object - infill_only_where_needed should be the method of choice for printing
    // hollow objects
    FOREACH_REGION(this->_print, region_it) {
        size_t region_id = region_it - this->_print->regions.begin();
        const PrintRegion &region = **region_it;
        
        
        if (!region.config.extra_perimeters
            || region.config.perimeters == 0
            || region.config.fill_density == 0
            || this->layer_count() < 2) continue;
        
        for (size_t i = 0; i <= (this->layer_count()-2); ++i) {
            LayerRegion &layerm                     = *this->get_layer(i)->get_region(region_id);
            const LayerRegion &upper_layerm         = *this->get_layer(i+1)->get_region(region_id);
            const Polygons upper_layerm_polygons    = upper_layerm.slices;
            
            // Filter upper layer polygons in intersection_ppl by their bounding boxes?
            // my $upper_layerm_poly_bboxes= [ map $_->bounding_box, @{$upper_layerm_polygons} ];
            double total_loop_length = 0;
            for (Polygons::const_iterator it = upper_layerm_polygons.begin(); it != upper_layerm_polygons.end(); ++it)
                total_loop_length += it->length();
            
            const coord_t perimeter_spacing     = layerm.flow(frPerimeter).scaled_spacing();
            const Flow ext_perimeter_flow       = layerm.flow(frExternalPerimeter);
            const coord_t ext_perimeter_width   = ext_perimeter_flow.scaled_width();
            const coord_t ext_perimeter_spacing = ext_perimeter_flow.scaled_spacing();
            
            for (Surfaces::iterator slice = layerm.slices.surfaces.begin();
                slice != layerm.slices.surfaces.end(); ++slice) {
                while (true) {
                    // compute the total thickness of perimeters
                    const coord_t perimeters_thickness = ext_perimeter_width/2 + ext_perimeter_spacing/2
                        + (region.config.perimeters-1 + region.config.extra_perimeters) * perimeter_spacing;
                    
                    // define a critical area where we don't want the upper slice to fall into
                    // (it should either lay over our perimeters or outside this area)
                    const coord_t critical_area_depth = perimeter_spacing * 1.5;
                    const Polygons critical_area = diff(
                        offset(slice->expolygon, -perimeters_thickness),
                        offset(slice->expolygon, -(perimeters_thickness + critical_area_depth))
                    );
                    
                    // check whether a portion of the upper slices falls inside the critical area
                    const Polylines intersection = intersection_pl(
                        upper_layerm_polygons,
                        critical_area
                    );
                    
                    // only add an additional loop if at least 30% of the slice loop would benefit from it
                    {
                        double total_intersection_length = 0;
                        for (Polylines::const_iterator it = intersection.begin(); it != intersection.end(); ++it)
                            total_intersection_length += it->length();
                        if (total_intersection_length <= total_loop_length*0.3) break;
                    }
                    
                    /*
                    if (0) {
                        require "Slic3r/SVG.pm";
                        Slic3r::SVG::output(
                            "extra.svg",
                            no_arrows   => 1,
                            expolygons  => union_ex($critical_area),
                            polylines   => [ map $_->split_at_first_point, map $_->p, @{$upper_layerm->slices} ],
                        );
                    }
                    */
                    
                    slice->extra_perimeters++;
                }
                
                #ifdef DEBUG
                    if (slice->extra_perimeters > 0)
                        printf("  adding %d more perimeter(s) at layer %zu\n", slice->extra_perimeters, i);
                #endif
            }
        }
    }
    
    parallelize<Layer*>(
        std::queue<Layer*>(std::deque<Layer*>(this->layers.begin(), this->layers.end())),  // cast LayerPtrs to std::queue<Layer*>
        boost::bind(&Slic3r::Layer::make_perimeters, _1),
        this->_print->config.threads.value
    );
    
    /*
        simplify slices (both layer and region slices),
        we only need the max resolution for perimeters
    ### This makes this method not-idempotent, so we keep it disabled for now.
    ###$self->_simplify_slices(&Slic3r::SCALED_RESOLUTION);
    */
    
    this->state.set_done(posPerimeters);
}

void
PrintObject::_infill()
{
    if (this->state.is_done(posInfill)) return;
    this->state.set_started(posInfill);
    
    parallelize<Layer*>(
        std::queue<Layer*>(std::deque<Layer*>(this->layers.begin(), this->layers.end())),  // cast LayerPtrs to std::queue<Layer*>
        boost::bind(&Slic3r::Layer::make_fills, _1),
        this->_print->config.threads.value
    );
    
    /*  we could free memory now, but this would make this step not idempotent
    ### $_->fill_surfaces->clear for map @{$_->regions}, @{$object->layers};
    */
    
    this->state.set_done(posInfill);
}

}

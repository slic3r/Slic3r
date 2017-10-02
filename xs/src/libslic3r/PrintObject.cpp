#include "Print.hpp"
#include "BoundingBox.hpp"
#include "ClipperUtils.hpp"
#include "Geometry.hpp"
#include <algorithm>
#include <vector>

namespace Slic3r {

PrintObject::PrintObject(Print* print, ModelObject* model_object, const BoundingBoxf3 &modobj_bbox)
:   typed_slices(false),
    _print(print),
    _model_object(model_object),
    layer_height_spline(model_object->layer_height_spline)
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

// returns 0-based indices of used extruders
std::set<size_t>
PrintObject::extruders() const
{
    std::set<size_t> extruders = this->_print->extruders();
    std::set<size_t> sm_extruders = this->support_material_extruders();
    extruders.insert(sm_extruders.begin(), sm_extruders.end());
    return extruders;
}

// returns 0-based indices of used extruders
std::set<size_t>
PrintObject::support_material_extruders() const
{
    std::set<size_t> extruders;
    if (this->has_support_material()) {
        extruders.insert(this->config.support_material_extruder - 1);
        extruders.insert(this->config.support_material_interface_extruder - 1);
    }
    return extruders;
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
PrintObject::invalidate_state_by_config(const PrintConfigBase &config)
{
    const t_config_option_keys diff = this->config.diff(config);
    
    std::set<PrintObjectStep> steps;
    bool all = false;
    
    // this method only accepts PrintObjectConfig and PrintRegionConfig option keys
    for (const t_config_option_key &opt_key : diff) {
        if (opt_key == "layer_height"
            || opt_key == "first_layer_height"
            || opt_key == "adaptive_slicing"
            || opt_key == "adaptive_slicing_quality"
            || opt_key == "match_horizontal_surfaces"
            || opt_key == "regions_overlap") {
            steps.insert(posLayers);
        } else if (opt_key == "xy_size_compensation"
            || opt_key == "raft_layers") {
            steps.insert(posSlice);
        } else if (opt_key == "support_material_contact_distance") {
            steps.insert(posSlice);
            steps.insert(posPerimeters);
            steps.insert(posSupportMaterial);
        } else if (opt_key == "support_material") {
            steps.insert(posPerimeters);
            steps.insert(posSupportMaterial);
        } else if (opt_key == "support_material_angle"
            || opt_key == "support_material_extruder"
            || opt_key == "support_material_extrusion_width"
            || opt_key == "support_material_interface_layers"
            || opt_key == "support_material_interface_extruder"
            || opt_key == "support_material_interface_spacing"
            || opt_key == "support_material_interface_speed"
            || opt_key == "support_material_buildplate_only"
            || opt_key == "support_material_pattern"
            || opt_key == "support_material_spacing"
            || opt_key == "support_material_threshold"
            || opt_key == "dont_support_bridges") {
            steps.insert(posSupportMaterial);
        } else if (opt_key == "interface_shells"
            || opt_key == "infill_only_where_needed") {
            steps.insert(posPrepareInfill);
        } else if (opt_key == "seam_position"
            || opt_key == "support_material_speed") {
            // these options only affect G-code export, so nothing to invalidate
        } else {
            // for legacy, if we can't handle this option let's invalidate all steps
            all = true;
            break;
        }
    }
    
    if (!diff.empty())
        this->config.apply(config, true);
    
    bool invalidated = false;
    if (all) {
        invalidated = this->invalidate_all_steps();
    } else {
        for (const PrintObjectStep &step : steps)
            if (this->invalidate_step(step))
                invalidated = true;
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
    } else if (step == posDetectSurfaces) {
        this->invalidate_step(posPrepareInfill);
    } else if (step == posPrepareInfill) {
        this->invalidate_step(posInfill);
    } else if (step == posInfill) {
        this->_print->invalidate_step(psSkirt);
        this->_print->invalidate_step(psBrim);
    } else if (step == posSlice) {
        this->invalidate_step(posPerimeters);
        this->invalidate_step(posDetectSurfaces);
        this->invalidate_step(posSupportMaterial);
    }else if (step == posLayers) {
        this->invalidate_step(posSlice);
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

void
PrintObject::detect_surfaces_type()
{
    // prerequisites
    // this->slice();
    
    if (this->state.is_done(posDetectSurfaces)) return;
    this->state.set_started(posDetectSurfaces);
    
    parallelize<Layer*>(
        std::queue<Layer*>(std::deque<Layer*>(this->layers.begin(), this->layers.end())),  // cast LayerPtrs to std::queue<Layer*>
        boost::bind(&Slic3r::Layer::detect_surfaces_type, _1),
        this->_print->config.threads.value
    );
    
    this->typed_slices = true;
    this->state.set_done(posDetectSurfaces);
}

void
PrintObject::process_external_surfaces()
{
    parallelize<Layer*>(
        std::queue<Layer*>(std::deque<Layer*>(this->layers.begin(), this->layers.end())),  // cast LayerPtrs to std::queue<Layer*>
        boost::bind(&Slic3r::Layer::process_external_surfaces, _1),
        this->_print->config.threads.value
    );
}

/* This method applies bridge flow to the first internal solid layer above
   sparse infill */
void
PrintObject::bridge_over_infill()
{
    FOREACH_REGION(this->_print, region) {
        const size_t region_id = region - this->_print->regions.begin();
        
        // skip bridging in case there are no voids
        if ((*region)->config.fill_density.value == 100) continue;
        
        // get bridge flow
        const Flow bridge_flow = (*region)->flow(
            frSolidInfill,
            -1,     // layer height, not relevant for bridge flow
            true,   // bridge
            false,  // first layer
            -1,     // custom width, not relevant for bridge flow
            *this
        );
        
        // get the average extrusion volume per surface unit
        const double mm3_per_mm  = bridge_flow.mm3_per_mm();
        const double mm3_per_mm2 = mm3_per_mm / bridge_flow.width;
        
        FOREACH_LAYER(this, layer_it) {
            // skip first layer
            if (layer_it == this->layers.begin()) continue;
            
            Layer* layer        = *layer_it;
            LayerRegion* layerm = layer->get_region(region_id);
            
            // extract the stInternalSolid surfaces that might be transformed into bridges
            Polygons internal_solid;
            layerm->fill_surfaces.filter_by_type(stInternalSolid, &internal_solid);
            if (internal_solid.empty()) continue;
            
            // check whether we should bridge or not according to density
            {
                // get the normal solid infill flow we would use if not bridging
                const Flow normal_flow = layerm->flow(frSolidInfill, false);
                
                // Bridging over sparse infill has two purposes:
                // 1) cover better the gaps of internal sparse infill, especially when
                //    printing at very low densities;
                // 2) provide a greater flow when printing very thin layers where normal
                //    solid flow would be very poor.
                // So we calculate density threshold as interpolation according to normal flow.
                // If normal flow would be equal or greater than the bridge flow, we can keep
                // a low threshold like 25% in order to bridge only when printing at very low
                // densities, when sparse infill has significant gaps.
                // If normal flow would be equal or smaller than half the bridge flow, we
                // use a higher threshold like 50% in order to bridge in more cases.
                // We still never bridge whenever fill density is greater than 50% because
                // we would overstuff.
                const float min_threshold = 25.0;
                const float max_threshold = 50.0;
                const float density_threshold = std::max(
                    std::min<float>(
                        min_threshold
                            + (max_threshold - min_threshold)
                            * (normal_flow.mm3_per_mm() - mm3_per_mm)
                            / (mm3_per_mm/2 - mm3_per_mm),
                        max_threshold
                    ),
                    min_threshold
                );
                
                if ((*region)->config.fill_density.value > density_threshold) continue;
            }
            
            // check whether the lower area is deep enough for absorbing the extra flow
            // (for obvious physical reasons but also for preventing the bridge extrudates
            // from overflowing in 3D preview)
            ExPolygons to_bridge;
            {
                Polygons to_bridge_pp = internal_solid;
                
                // Only bridge where internal infill exists below the solid shell matching
                // these two conditions:
                // 1) its depth is at least equal to our bridge extrusion diameter;
                // 2) its free volume (thus considering infill density) is at least equal
                //    to the volume needed by our bridge flow.
                double excess_mm3_per_mm2 = mm3_per_mm2;
                
                // iterate through lower layers spanned by bridge_flow
                const double bottom_z = layer->print_z - bridge_flow.height;
                for (int i = (layer_it - this->layers.begin()) - 1; i >= 0; --i) {
                    const Layer* lower_layer = this->layers[i];
                    
                    // subtract the void volume of this layer
                    excess_mm3_per_mm2 -= lower_layer->height * (100 - (*region)->config.fill_density.value)/100;
                    
                    // stop iterating if both conditions are matched
                    if (lower_layer->print_z < bottom_z && excess_mm3_per_mm2 <= 0) break;
                    
                    // iterate through regions and collect internal surfaces
                    Polygons lower_internal;
                    FOREACH_LAYERREGION(lower_layer, lower_layerm_it)
                        (*lower_layerm_it)->fill_surfaces.filter_by_type(stInternal, &lower_internal);
                    
                    // intersect such lower internal surfaces with the candidate solid surfaces
                    to_bridge_pp = intersection(to_bridge_pp, lower_internal);
                }
                
                // don't bridge if the volume condition isn't matched
                if (excess_mm3_per_mm2 > 0) continue;
                
                // there's no point in bridging too thin/short regions
                {
                    const double min_width = bridge_flow.scaled_width() * 3;
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
            const ExPolygons not_to_bridge = diff_ex(internal_solid, to_polygons(to_bridge), true);
            
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

// adjust the layer height to the next multiple of the z full-step resolution
coordf_t PrintObject::adjust_layer_height(coordf_t layer_height) const
{
    coordf_t result = layer_height;
    if(this->_print->config.z_steps_per_mm > 0) {
        coordf_t min_dz = 1 / this->_print->config.z_steps_per_mm * 4;
        result = int(layer_height / min_dz + 0.5) * min_dz;
    }

    return result > 0 ? result : layer_height;
}

// generate a vector of print_z coordinates in object coordinate system (starting with 0) but including
// the first_layer_height if provided.
std::vector<coordf_t> PrintObject::generate_object_layers(coordf_t first_layer_height) {

    std::vector<coordf_t> result;

    // collect values from config
    coordf_t min_nozzle_diameter = 1.0;
    coordf_t min_layer_height = 0.0;
    coordf_t max_layer_height = 10.0;
    std::set<size_t> object_extruders = this->_print->object_extruders();
    for (std::set<size_t>::const_iterator it_extruder = object_extruders.begin(); it_extruder != object_extruders.end(); ++ it_extruder) {
        min_nozzle_diameter = std::min(min_nozzle_diameter, this->_print->config.nozzle_diameter.get_at(*it_extruder));
        min_layer_height = std::max(min_layer_height, this->_print->config.min_layer_height.get_at(*it_extruder));
        max_layer_height = std::min(max_layer_height, this->_print->config.max_layer_height.get_at(*it_extruder));

    }
    coordf_t layer_height = std::min(min_nozzle_diameter, this->config.layer_height.getFloat());
    layer_height = this->adjust_layer_height(layer_height);
    this->config.layer_height.value = layer_height;

    // respect first layer height
    if(first_layer_height) {
        result.push_back(first_layer_height);
    }

    coordf_t print_z = first_layer_height;
    coordf_t height = first_layer_height;

    // Update object size at the spline object to define upper border
    this->layer_height_spline.setObjectHeight(unscale(this->size.z));
    if (this->state.is_done(posLayers)) {
        // layer heights are already generated, just update layers from spline
        // we don't need to respect first layer here, it's correctly provided by the spline object
        result = this->layer_height_spline.getInterpolatedLayers();
    }else{ // create new set of layers
        // create stateful objects and variables for the adaptive slicing process
        SlicingAdaptive as;
        coordf_t adaptive_quality = this->config.adaptive_slicing_quality.value;
        if(this->config.adaptive_slicing.value) {
            const ModelVolumePtrs volumes = this->model_object()->volumes;
            for (ModelVolumePtrs::const_iterator it = volumes.begin(); it != volumes.end(); ++ it)
                if (! (*it)->modifier)
                    as.add_mesh(&(*it)->mesh);
            as.prepare(unscale(this->size.z));
        }

        // loop until we have at least one layer and the max slice_z reaches the object height
        while (print_z < unscale(this->size.z)) {

            if (this->config.adaptive_slicing.value) {
                height = 999;

                // determine next layer height
                height = as.next_layer_height(print_z, adaptive_quality, min_layer_height, max_layer_height);

                // check for horizontal features and object size
                if(this->config.match_horizontal_surfaces.value) {
                    coordf_t horizontal_dist = as.horizontal_facet_distance(print_z + height, min_layer_height);
                    if((horizontal_dist < min_layer_height) && (horizontal_dist > 0)) {
                        #ifdef SLIC3R_DEBUG
                        std::cout << "Horizontal feature ahead, distance: " << horizontal_dist << std::endl;
                        #endif
                        // can we shrink the current layer a bit?
                        if(height-(min_layer_height - horizontal_dist) > min_layer_height) {
                            // yes we can
                            height -= (min_layer_height - horizontal_dist);
                            #ifdef SLIC3R_DEBUG
                            std::cout << "Shrink layer height to " << height << std::endl;
                            #endif
                        }else{
                            // no, current layer would become too thin
                            height += horizontal_dist;
                            #ifdef SLIC3R_DEBUG
                            std::cout << "Widen layer height to " << height << std::endl;
                            #endif
                        }
                    }
                }
            }else{
                height = layer_height;
            }

            // look for an applicable custom range
            for (t_layer_height_ranges::const_iterator it_range = this->layer_height_ranges.begin(); it_range != this->layer_height_ranges.end(); ++ it_range) {
                if(print_z >= it_range->first.first && print_z <= it_range->first.second) {
                    if(it_range->second > 0) {
                        height = it_range->second;
                    }
                }
            }

            print_z += height;

            result.push_back(print_z);
        }

        // Reduce or thicken the top layer in order to match the original object size.
        // This is not actually related to z_steps_per_mm but we only enable it in case
        // user provided that value, as it means they really care about the layer height
        // accuracy and we don't provide unexpected result for people noticing the last
        // layer has a different layer height.
        if (this->_print->config.z_steps_per_mm > 0 && result.size() > 1 && !this->config.adaptive_slicing.value) {
            coordf_t diff = result.back() - unscale(this->size.z);
            int last_layer = result.size()-1;

            if (diff < 0) {
                // we need to thicken last layer
                coordf_t new_h = result[last_layer] - result[last_layer-1];
                new_h = std::min(min_nozzle_diameter, new_h - diff); // add (negativ) diff value
                result[last_layer] = result[last_layer-1] + new_h;
            } else {
                // we need to reduce last layer
                coordf_t new_h = result[last_layer] - result[last_layer-1];
                if(min_nozzle_diameter/2 < new_h) { //prevent generation of a too small layer
                    new_h = std::max(min_nozzle_diameter/2, new_h - diff); // subtract (positive) diff value
                    result[last_layer] = result[last_layer-1] + new_h;
                }
            }
        }

        // Store layer vector for interactive manipulation
        this->layer_height_spline.setLayers(result);
        if (this->config.adaptive_slicing.value) { // smoothing after adaptive algorithm
            result = this->layer_height_spline.getInterpolatedLayers();
        }

        this->state.set_done(posLayers);
    }

    // push modified spline object back to model
    this->_model_object->layer_height_spline = this->layer_height_spline;

    // apply z-gradation (this is redundant for static layer height...)
    coordf_t gradation = 1 / this->_print->config.z_steps_per_mm * 4;
    if(this->_print->config.z_steps_per_mm > 0) {
        coordf_t last_z = 0;
        coordf_t height;
        for(std::vector<coordf_t>::iterator l = result.begin(); l != result.end(); ++l) {
            height = *l - last_z;

            coordf_t gradation_effect = unscale((scale_(height)) % (scale_(gradation)));
            if(gradation_effect > gradation/2 && (height + (gradation-gradation_effect)) <= max_layer_height) { // round up
                height = height + (gradation-gradation_effect);
            }else{ // round down
                height = height - gradation_effect;
            }
            height = std::min(std::max(height, min_layer_height), max_layer_height);
            *l = last_z + height;
            last_z = *l;
        }
    }

    return result;
}

// 1) Decides Z positions of the layers,
// 2) Initializes layers and their regions
// 3) Slices the object meshes
// 4) Slices the modifier meshes and reclassifies the slices of the object meshes by the slices of the modifier meshes
// 5) Applies size compensation (offsets the slices in XY plane)
// 6) Replaces bad slices by the slices reconstructed from the upper/lower layer
// Resulting expolygons of layer regions are marked as Internal.
//
// this should be idempotent
void PrintObject::_slice()
{

    coordf_t raft_height = 0;
    coordf_t first_layer_height = this->config.first_layer_height.get_abs_value(this->config.layer_height.value);


    // take raft layers into account
    int id = 0;

    if (this->config.raft_layers > 0) {
        id = this->config.raft_layers;

        coordf_t min_support_nozzle_diameter = 1.0;
        std::set<size_t> support_material_extruders = this->_print->support_material_extruders();
        for (std::set<size_t>::const_iterator it_extruder = support_material_extruders.begin(); it_extruder != support_material_extruders.end(); ++ it_extruder) {
            min_support_nozzle_diameter = std::min(min_support_nozzle_diameter, this->_print->config.nozzle_diameter.get_at(*it_extruder));
        }
        coordf_t support_material_layer_height = 0.75 * min_support_nozzle_diameter;

        // raise first object layer Z by the thickness of the raft itself
        // plus the extra distance required by the support material logic
        raft_height += first_layer_height;
        raft_height += support_material_layer_height * (this->config.raft_layers - 1);

        // reset for later layer generation
        first_layer_height = 0;

        // detachable support
        if(this->config.support_material_contact_distance > 0) {
            first_layer_height = min_support_nozzle_diameter;
            raft_height += this->config.support_material_contact_distance;

        }
    }

    // Initialize layers and their slice heights.
    std::vector<float> slice_zs;
    {
        this->clear_layers();
        // All print_z values for this object, without the raft.
        std::vector<coordf_t> object_layers = this->generate_object_layers(first_layer_height);
        // Reserve object layers for the raft. Last layer of the raft is the contact layer.
        slice_zs.reserve(object_layers.size());
        Layer *prev = nullptr;
        coordf_t lo = raft_height;
        coordf_t hi = lo;
        for (size_t i_layer = 0; i_layer < object_layers.size(); i_layer++) {
            lo = hi;  // store old value
            hi = object_layers[i_layer] + raft_height;
            coordf_t slice_z = 0.5 * (lo + hi) - raft_height;
            Layer *layer = this->add_layer(id++, hi - lo, hi, slice_z);
            slice_zs.push_back(float(slice_z));
            if (prev != nullptr) {
                prev->upper_layer = layer;
                layer->lower_layer = prev;
            }
            // Make sure all layers contain layer region objects for all regions.
            for (size_t region_id = 0; region_id < this->_print->regions.size(); ++ region_id)
                layer->add_region(this->print()->regions[region_id]);
            prev = layer;
        }
    }

    if (this->print()->regions.size() == 1) {
        // Optimized for a single region. Slice the single non-modifier mesh.
        std::vector<ExPolygons> expolygons_by_layer = this->_slice_region(0, slice_zs, false);
        for (size_t layer_id = 0; layer_id < expolygons_by_layer.size(); ++ layer_id)
            this->layers[layer_id]->regions.front()->slices.append(std::move(expolygons_by_layer[layer_id]), stInternal);
    } else {
        // Slice all non-modifier volumes.
        for (size_t region_id = 0; region_id < this->print()->regions.size(); ++ region_id) {
            std::vector<ExPolygons> expolygons_by_layer = this->_slice_region(region_id, slice_zs, false);
            for (size_t layer_id = 0; layer_id < expolygons_by_layer.size(); ++ layer_id)
                this->layers[layer_id]->regions[region_id]->slices.append(std::move(expolygons_by_layer[layer_id]), stInternal);
        }
        // Slice all modifier volumes.
        for (size_t region_id = 0; region_id < this->print()->regions.size(); ++ region_id) {
            std::vector<ExPolygons> expolygons_by_layer = this->_slice_region(region_id, slice_zs, true);
            // loop through the other regions and 'steal' the slices belonging to this one
            for (size_t other_region_id = 0; other_region_id < this->print()->regions.size(); ++ other_region_id) {
                if (region_id == other_region_id)
                    continue;
                for (size_t layer_id = 0; layer_id < expolygons_by_layer.size(); ++ layer_id) {
                    Layer       *layer = layers[layer_id];
                    LayerRegion *layerm = layer->regions[region_id];
                    LayerRegion *other_layerm = layer->regions[other_region_id];
                    if (layerm == nullptr || other_layerm == nullptr)
                        continue;
                    Polygons other_slices = to_polygons(other_layerm->slices);
                    ExPolygons my_parts = intersection_ex(other_slices, to_polygons(expolygons_by_layer[layer_id]));
                    if (my_parts.empty())
                        continue;
                    // Remove such parts from original region.
                    other_layerm->slices.set(diff_ex(other_slices, to_polygons(my_parts)), stInternal);
                    // Append new parts to our region.
                    layerm->slices.append(std::move(my_parts), stInternal);
                }
            }
        }
    }

    // remove last layer(s) if empty
    bool done = false;
    while (! this->layers.empty()) {
        const Layer *layer = this->layers.back();
        for (size_t region_id = 0; region_id < this->print()->regions.size(); ++ region_id)
            if (layer->regions[region_id] != nullptr && ! layer->regions[region_id]->slices.empty()) {
                done = true;
                break;
            }
        if(done) {
            break;
        }
        this->delete_layer(int(this->layers.size()) - 1);
    }
    
    // Apply size compensation and perform clipping of multi-part objects.
    const coord_t xy_size_compensation = scale_(this->config.xy_size_compensation.value);
    for (Layer* layer : this->layers) {
        if (abs(xy_size_compensation) > 0) {
            if (layer->regions.size() == 1) {
                // Single region, growing or shrinking.
                LayerRegion* layerm = layer->regions.front();
                layerm->slices.set(
                    offset_ex(to_expolygons(std::move(layerm->slices.surfaces)), xy_size_compensation),
                    stInternal
                );
            } else {
                // Multiple regions, growing, shrinking or just clipping one region by the other.
                // When clipping the regions, priority is given to the first regions.
                Polygons processed;
                for (size_t region_id = 0; region_id < layer->regions.size(); ++region_id) {
                    LayerRegion* layerm = layer->regions[region_id];
                    Polygons slices = layerm->slices;
                    
                    if (abs(xy_size_compensation) > 0)
                        slices = offset(slices, xy_size_compensation);
                    
                    if (region_id > 0)
                        // Trim by the slices of already processed regions.
                        slices = diff(std::move(slices), processed);
                    
                    if (region_id + 1 < layer->regions.size())
                        // Collect the already processed regions to trim the to be processed regions.
                        append_to(processed, slices);
                    
                    layerm->slices.set(union_ex(slices), stInternal);
                }
            }
        }
        
        // Merge all regions' slices to get islands, chain them by a shortest path.
        layer->make_slices();
        
        // Apply regions overlap
        if (this->config.regions_overlap.value > 0) {
            const coord_t delta = scale_(this->config.regions_overlap.value)/2;
            for (LayerRegion* layerm : layer->regions)
                layerm->slices.set(
                    intersection_ex(
                        offset(layerm->slices, +delta),
                        layer->slices
                    ),
                    stInternal
                );
        }
    }
}

// called from slice()
std::vector<ExPolygons>
PrintObject::_slice_region(size_t region_id, std::vector<float> z, bool modifier)
{
    std::vector<ExPolygons> layers;
    std::vector<int> &region_volumes = this->region_volumes[region_id];
    if (region_volumes.empty()) return layers;
    
    ModelObject &object = *this->model_object();
    
    // compose mesh
    TriangleMesh mesh;
    for (std::vector<int>::const_iterator it = region_volumes.begin();
        it != region_volumes.end(); ++it) {
        
        const ModelVolume &volume = *object.volumes[*it];
        if (volume.modifier != modifier) continue;
        
        mesh.merge(volume.mesh);
    }
    if (mesh.facets_count() == 0) return layers;

    // transform mesh
    // we ignore the per-instance transformations currently and only 
    // consider the first one
    object.instances[0]->transform_mesh(&mesh, true);

    // align mesh to Z = 0 (it should be already aligned actually) and apply XY shift
    mesh.translate(
        -unscale(this->_copies_shift.x),
        -unscale(this->_copies_shift.y),
        -object.bounding_box().min.z
    );
    
    // perform actual slicing
    TriangleMeshSlicer<Z>(&mesh).slice(z, &layers);
    return layers;
}

void
PrintObject::_make_perimeters()
{
    if (this->state.is_done(posPerimeters)) return;
    this->state.set_started(posPerimeters);
    
    // merge slices if they were split into types
    // This is not currently taking place because since merge_slices + detect_surfaces_type
    // are not truly idempotent we are invalidating posSlice here (see the Perl part of 
    // this method).
    if (this->typed_slices) {
        // merge_slices() undoes detect_surfaces_type()
        FOREACH_LAYER(this, layer_it)
            (*layer_it)->merge_slices();
        this->typed_slices = false;
        this->state.invalidate(posDetectSurfaces);
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
            
            // In order to avoid diagonal gaps (GH #3732) we ignore the external half of the upper
            // perimeter, since it's not truly covering this layer.
            const Polygons upper_layerm_polygons = offset(
                upper_layerm.slices,
                -upper_layerm.flow(frExternalPerimeter).scaled_width()/2
            );
            
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
                        + (region.config.perimeters-1 + slice->extra_perimeters) * perimeter_spacing;
                    
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

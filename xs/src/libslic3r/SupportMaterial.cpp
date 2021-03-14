#include "SupportMaterial.hpp"
#include "Log.hpp"


namespace Slic3r
{

#if BOOST_VERSION >= 107300
using boost::placeholders::_1;
#endif

PolylineCollection _fill_surface(Fill *fill, Surface *surface)
{
    PolylineCollection ps;
    ps.polylines = fill->fill_surface(*surface);
    return ps;
}

void
SupportMaterial::generate_toolpaths(PrintObject *object,
                                    map<coordf_t, Polygons> overhang,
                                    map<coordf_t, Polygons> contact,
                                    map<int, Polygons> _interface,
                                    map<int, Polygons> base)
{
    // Assign the object to the supports class.
    this->object = object;

    // Shape of contact area.
    toolpaths_params params;
    params.contact_loops = 1;
    params.circle_radius = 1.5 * interface_flow.scaled_width();
    params.circle_distance = 3 * params.circle_radius;
    params.circle = create_circle(params.circle_radius);

#ifdef SLIC3R_DEBUG
    printf("Generating patterns.\n");
#endif

    // Prepare fillers.
    params.pattern = object_config->support_material_pattern.value;
    params.angles.push_back(object_config->support_material_angle.value);

    if (params.pattern == smpRectilinearGrid) {
        params.pattern = smpRectilinear;
        params.angles.push_back(params.angles[0] + 90);
    }
    else if (params.pattern == smpPillars) {
        params.pattern = smpHoneycomb;
    }
    params.interface_angle = object_config->support_material_angle.value + 90;
    params.interface_spacing = object_config->support_material_interface_spacing.value + interface_flow.spacing();
    params.interface_density =
        static_cast<float>(params.interface_spacing == 0 ? 1 : interface_flow.spacing() / params.interface_spacing);
    params.support_spacing = object_config->support_material_spacing.value + flow.spacing();
    params.support_density = params.support_spacing == 0 ? 1 : flow.spacing() / params.support_spacing;

    parallelize<size_t>(
        0,
        object->support_layers.size() - 1,
        boost::bind(&SupportMaterial::process_layer, this, _1, params),
        this->config->threads.value
    );
}

void
SupportMaterial::generate(PrintObject *object)
{
    // Determine the top surfaces of the support, defined as:
    // contact = overhangs - clearance + margin
    // This method is responsible for identifying what contact surfaces
    // should the support material expose to the object in order to guarantee
    // that it will be effective, regardless of how it's built below.
    pair<map<coordf_t, Polygons>, map<coordf_t, Polygons>> contact_overhang = contact_area(object);
    map<coordf_t, Polygons> &contact = contact_overhang.first;
    map<coordf_t, Polygons> &overhang = contact_overhang.second;

    //bugfix: do not try to generate overhang if there is no contact area 
    if( contact.empty() ){
	Slic3r::Log::error("Support material") << "Empty contact_areas of SupportMaterial for object "
	    <<"[" << object->model_object()->name << "]" << std::endl;
    	return;
    }
	
    // Determine the top surfaces of the object. We need these to determine
    // the layer heights of support material and to clip support to the object
    // silhouette.
    map<coordf_t, Polygons> top = object_top(object, &contact);
    // We now know the upper and lower boundaries for our support material object
    // (@$contact_z and @$top_z), so we can generate intermediate layers.
    vector<coordf_t> support_z = support_layers_z(get_keys_sorted(contact),
                                                  get_keys_sorted(top),
                                                  get_max_layer_height(object));
    // If we wanted to apply some special logic to the first support layers lying on
    // object's top surfaces this is the place to detect them.
    map<int, Polygons> shape;
    if (object_config->support_material_pattern.value == smpPillars)
        this->generate_pillars_shape(contact, support_z, shape);

    // Propagate contact layers downwards to generate interface layers.
    map<int, Polygons> _interface = generate_interface_layers(support_z, contact, top);
    clip_with_object(_interface, support_z, *object);
    if (!shape.empty())
        clip_with_shape(_interface, shape);
    // Propagate contact layers and interface layers downwards to generate
    // the main support layers.
    map<int, Polygons> base = generate_base_layers(support_z, contact, _interface, top);
    clip_with_object(base, support_z, *object);
    if (!shape.empty())
        clip_with_shape(base, shape);

    // Detect what part of base support layers are "reverse interfaces" because they
    // lie above object's top surfaces.
    generate_bottom_interface_layers(support_z, base, top, _interface);
    // Install support layers into object.
    for (int i = 0; i < int(support_z.size()); i++) {
        object->add_support_layer(
            i, // id.
            (i == 0) ? support_z[0] - 0 : (support_z[i] - support_z[i - 1]), // height.
            support_z[i] // print_z
        );

        if (i >= 1) {
            object->support_layers.end()[-2]->upper_layer = object->support_layers.end()[-1];
            object->support_layers.end()[-1]->lower_layer = object->support_layers.end()[-2];
        }
    }
    // Generate the actual toolpaths and save them into each layer.
    generate_toolpaths(object, overhang, contact, _interface, base);
}

vector<coordf_t>
SupportMaterial::support_layers_z(vector<coordf_t> contact_z,
                                  vector<coordf_t> top_z,
                                  coordf_t max_object_layer_height)
{
    // Quick table to check whether a given Z is a top surface.
    map<coordf_t, bool> is_top;
    for (auto z : top_z) is_top[z] = true;

    // determine layer height for any non-contact layer
    // we use max() to prevent many ultra-thin layers to be inserted in case
    // layer_height > nozzle_diameter * 0.75.
    auto nozzle_diameter = config->nozzle_diameter.get_at(static_cast<size_t>(
                                                              object_config->support_material_extruder - 1));
    auto support_material_height = max(max_object_layer_height, (nozzle_diameter * 0.75));
    coordf_t _contact_distance = this->contact_distance(support_material_height, nozzle_diameter);
    // Initialize known, fixed, support layers.
    vector<coordf_t> z;
    for (auto c_z : contact_z) z.push_back(c_z);
    for (auto t_z : top_z) {
        z.push_back(t_z);
        z.push_back(t_z + _contact_distance);
    }
    sort(z.begin(), z.end());

    // Enforce first layer height.
    coordf_t first_layer_height = object_config->first_layer_height;
    while (!z.empty() && z.front() <= first_layer_height) z.erase(z.begin());
    z.insert(z.begin(), first_layer_height);

    // Add raft layers by dividing the space between first layer and
    // first contact layer evenly.
    if (object_config->raft_layers > 1 && z.size() >= 2) {
        // z[1] is last raft layer (contact layer for the first layer object) TODO @Samir55 How so?
        coordf_t height = (z[1] - z[0]) / (object_config->raft_layers - 1);

        // since we already have two raft layers ($z[0] and $z[1]) we need to insert
        // raft_layers-2 more
        int idx = 1;
        for (int j = 1; j <= object_config->raft_layers - 2; j++) {
            float z_new =
                roundf(static_cast<float>((z[0] + height * idx) * 100)) / 100; // round it to 2 decimal places.
            z.insert(z.begin() + idx, z_new);
            idx++;
        }
    }

    // Create other layers (skip raft layers as they're already done and use thicker layers).
    for (auto i = static_cast<int>(z.size()) - 1; i >= object_config->raft_layers; i--) {
        coordf_t target_height = support_material_height;
        if (i > 0 && is_top.count(z[i - 1]) > 0 && is_top[z[i - 1]]) {
            target_height = nozzle_diameter;
        }
        // Enforce first layer height.
        if ((i == 0 && z[i] > target_height + first_layer_height)
            || (i > 0 && z[i] - z[i - 1] > target_height + EPSILON)) {
            z.insert(z.begin() + i, (z[i] - target_height));
            i++;
        }
    }

    // Remove duplicates and make sure all 0.x values have the leading 0.
    {
        set<coordf_t> s;
        for (coordf_t el : z)
            s.insert(int(el * 1000) / 1000.0); // round it to 2 decimal places.
        z = vector<coordf_t>();
        for (coordf_t el : s)
            z.push_back(el);
    }

    return z;
}

pair<map<coordf_t, Polygons>, map<coordf_t, Polygons>>
SupportMaterial::contact_area(PrintObject *object)
{
    PrintObjectConfig &conf = *this->object_config;

    // If user specified a custom angle threshold, convert it to radians.
    float threshold_rad = 0.0;

    if (!conf.support_material_threshold.percent) {
        threshold_rad = static_cast<float>(Geometry::deg2rad(
            conf.support_material_threshold + 1)); // +1 makes the threshold inclusive

#ifdef SLIC3R_DEBUG
        printf("Threshold angle = %d°\n", static_cast<int>(Geometry::rad2deg(threshold_rad)));
#endif
    }

    // Build support on a build plate only? If so, then collect top surfaces into $buildplate_only_top_surfaces
    // and subtract buildplate_only_top_surfaces from the contact surfaces, so
    // there is no contact surface supported by a top surface.
    bool buildplate_only =
        (conf.support_material || conf.support_material_enforce_layers)
            && conf.support_material_buildplate_only;
    Polygons buildplate_only_top_surfaces;

    // Determine contact areas.
    map<coordf_t, Polygons> contact; // contact_z => [ polygons ].
    map<coordf_t, Polygons> overhang; // This stores the actual overhang supported by each contact layer
    for (int layer_id = 0; layer_id < (int)object->layers.size(); layer_id++) {
        // Note $layer_id might != $layer->id when raft_layers > 0
        // so $layer_id == 0 means first object layer
        // and $layer->id == 0 means first print layer (including raft).

        // If no raft, and we're at layer 0, skip to layer 1
        if (conf.raft_layers == 0 && layer_id == 0) {
            continue;
        }
        // With or without raft, if we're above layer 1, we need to quit
        // support generation if supports are disabled, or if we're at a high
        // enough layer that enforce-supports no longer applies.
        if (layer_id > 0
            && !conf.support_material
            && (layer_id >= conf.support_material_enforce_layers))
            // If we are only going to generate raft just check
            // the 'overhangs' of the first object layer.
            break;

        Layer *layer = object->get_layer(layer_id);

        if (conf.support_material_max_layers
            && layer_id > conf.support_material_max_layers)
            break;

        if (buildplate_only) {
            // Collect the top surfaces up to this layer and merge them. TODO @Ask about this line.
            Polygons projection_new;
            for (auto const &region : layer->regions) {
                SurfacesPtr top_surfaces = region->slices.filter_by_type(stTop);
                for (const auto &polygon : p(top_surfaces)) {
                    projection_new.push_back(polygon);
                }
            }
            if (!projection_new.empty()) {
                // Merge the new top surfaces with the preceding top surfaces. TODO @Ask about this line.
                // Apply the safety offset to the newly added polygons, so they will connect
                // with the polygons collected before,
                // but don't apply the safety offset during the union operation as it would
                // inflate the polygons over and over.
                append_to(buildplate_only_top_surfaces, offset(projection_new, scale_(0.01)));

                buildplate_only_top_surfaces = union_(buildplate_only_top_surfaces, 0);
            }
        }

        // Detect overhangs and contact areas needed to support them.
        Polygons tmp_overhang, tmp_contact;
        if (layer_id == 0) {
            // this is the first object layer, so we're here just to get the object
            // footprint for the raft.
            // we only consider contours and discard holes to get a more continuous raft.
            for (auto const &contour : layer->slices.contours())
                tmp_overhang.push_back(contour);

            Polygons polygons = offset(tmp_overhang, scale_(+SUPPORT_MATERIAL_MARGIN));
            append_to(tmp_contact, polygons);
        }
        else {
            Layer *lower_layer = object->get_layer(layer_id - 1);
            for (auto layer_m : layer->regions) {
                auto fw = layer_m->flow(frExternalPerimeter).scaled_width();
                Polygons difference;

                // If a threshold angle was specified, use a different logic for detecting overhangs.
                if ((conf.support_material && threshold_rad != 0.0)
                    || layer_id <= conf.support_material_enforce_layers
                    || (conf.raft_layers > 0 && layer_id
                        == 0)) { // TODO ASK @Samir why layer_id ==0 check , layer_id will never equal to zero
                    float d = 0;
                    float layer_threshold_rad = threshold_rad;
                    if (layer_id <= conf.support_material_enforce_layers) {
                        // Use ~45 deg number for enforced supports if we are in auto.
                        layer_threshold_rad = static_cast<float>(Geometry::deg2rad(89));
                    }
                    if (layer_threshold_rad > 0) {
                        d = scale_(lower_layer->height
                                       * (cos(layer_threshold_rad) / sin(layer_threshold_rad)));
                    }

                    difference = diff(
                        Polygons(layer_m->slices),
                        offset(lower_layer->slices, +d)
                    );

                    // only enforce spacing from the object ($fw/2) if the threshold angle
                    // is not too high: in that case, $d will be very small (as we need to catch
                    // very short overhangs), and such contact area would be eaten by the
                    // enforced spacing, resulting in high threshold angles to be almost ignored
                    if (d > fw / 2)
                        difference = diff(
                            offset(difference, d - fw / 2),
                            lower_layer->slices);

                }
                else {
                    difference = diff(
                        Polygons(layer_m->slices),
                        offset(lower_layer->slices,
                               static_cast<const float>(+conf.get_abs_value("support_material_threshold", fw)))
                    );

                    // Collapse very tiny spots.
                    difference = offset2(difference, -fw / 10, +fw / 10);
                    // $diff now contains the ring or stripe comprised between the boundary of
                    // lower slices and the centerline of the last perimeter in this overhanging layer.
                    // Void $diff means that there's no upper perimeter whose centerline is
                    // outside the lower slice boundary, thus no overhang
                }

                if (conf.dont_support_bridges) {
                    // Compute the area of bridging perimeters.
                    Polygons bridged_perimeters;
                    {
                        auto bridge_flow = layer_m->flow(FlowRole::frPerimeter, 1);

                        // Get the lower layer's slices and grow them by half the nozzle diameter
                        // because we will consider the upper perimeters supported even if half nozzle
                        // falls outside the lower slices.
                        Polygons lower_grown_slices;
                        {
                            coordf_t nozzle_diameter = this->config->nozzle_diameter
                                .get_at(static_cast<size_t>(layer_m->region()->config.perimeter_extruder - 1));

                            lower_grown_slices = offset(
                                lower_layer->slices,
                                scale_(nozzle_diameter / 2)
                            );
                        }

                        // TODO Revise Here.
                        // Get all perimeters as polylines.
                        // TODO: split_at_first_point() (called by as_polyline() for ExtrusionLoops)
                        // could split a bridge mid-way.
                        Polylines overhang_perimeters;
                        for (auto extr_path : ExtrusionPaths(layer_m->perimeters.flatten())) {
                            overhang_perimeters.push_back(extr_path.as_polyline());
                        }

                        // Only consider the overhang parts of such perimeters,
                        // overhangs being those parts not supported by
                        // workaround for Clipper bug, see Slic3r::Polygon::clip_as_polyline()
                        for (auto &overhang_perimeter : overhang_perimeters)
                            overhang_perimeter.translate(1, 0);
                        overhang_perimeters = diff_pl(overhang_perimeters, lower_grown_slices);

                        // Only consider straight overhangs.
                        Polylines new_overhangs_perimeters_polylines;
                        for (const auto &p : overhang_perimeters)
                            if (p.is_straight())
                                new_overhangs_perimeters_polylines.push_back(p);

                        overhang_perimeters = new_overhangs_perimeters_polylines;

                        // Only consider overhangs having endpoints inside layer's slices
                        for (auto &p : overhang_perimeters) {
                            p.extend_start(fw);
                            p.extend_end(fw);
                        }

                        new_overhangs_perimeters_polylines = Polylines();
                        for (const auto &p : overhang_perimeters) {
                            if (layer->slices.contains_b(p.first_point())
                                && layer->slices.contains_b(p.last_point())) {
                                new_overhangs_perimeters_polylines.push_back(p);
                            }
                        }

                        overhang_perimeters = new_overhangs_perimeters_polylines;
                        new_overhangs_perimeters_polylines = Polylines();

                        // Convert bridging polylines into polygons by inflating them with their thickness.
                        {
                            // For bridges we can't assume width is larger than spacing because they
                            // are positioned according to non-bridging perimeters spacing.
                            coord_t widths[] = {bridge_flow.scaled_width(),
                                                bridge_flow.scaled_spacing(),
                                                fw,
                                                layer_m->flow(FlowRole::frPerimeter).scaled_width()};

                            auto w = *max_element(widths, widths + 4);

                            // Also apply safety offset to ensure no gaps are left in between.
                            Polygons ps = offset(overhang_perimeters, w / 2 + 10);
                            bridged_perimeters = union_(ps);
                        }
                    }

                    if (1) {
                        // Remove the entire bridges and only support the unsupported edges.
                        ExPolygons bridges;
                        for (auto surface : layer_m->fill_surfaces.filter_by_type(stBottom | stBridge)) {
                            if (surface->bridge_angle != -1) {
                                bridges.push_back(surface->expolygon);
                            }
                        }

                        Polygons ps = to_polygons(bridges);
                        append_to(ps, bridged_perimeters);

                        difference = diff( // TODO ASK about expolygons and polygons miss match.
                            difference,
                            ps,
                            true
                        );

                        append_to(difference,
                                  intersection(
                                      offset(layer_m->unsupported_bridge_edges.polylines,
                                             +scale_(SUPPORT_MATERIAL_MARGIN)), to_polygons(bridges)));
                    }
                    else {
                        // just remove bridged areas.
                        difference = diff(
                            difference,
                            layer_m->bridged,
                            true
                        );
                    }
                } // if ($conf->dont_support_bridges)

                if (buildplate_only) {
                    // Don't support overhangs above the top surfaces.
                    // This step is done before the contact surface is calcuated by growing the overhang region.
                    difference = diff(difference, buildplate_only_top_surfaces);
                }

                if (difference.empty()) continue;

                // NOTE: this is not the full overhang as it misses the outermost half of the perimeter width!
                append_to(tmp_overhang, difference);

                // Let's define the required contact area by using a max gap of half the upper
                // extrusion width and extending the area according to the configured margin.
                //    We increment the area in steps because we don't want our support to overflow
                // on the other side of the object (if it's very thin).
                {
                    Polygons slices_margin = offset(lower_layer->slices, +fw / 2);

                    if (buildplate_only) {
                        // Trim the inflated contact surfaces by the top surfaces as well.
                        append_to(slices_margin, buildplate_only_top_surfaces);
                        slices_margin = union_(slices_margin);
                    }

                    vector<coord_t> scale_vector
                        (static_cast<unsigned long>(SUPPORT_MATERIAL_MARGIN / MARGIN_STEP), scale_(MARGIN_STEP));
                    scale_vector.push_back(fw / 2);
                    for (int i = static_cast<int>(scale_vector.size()) - 1; i >= 0; i--) {
                        difference = diff(
                            offset(difference, i),
                            slices_margin
                        );
                    }
                }
                append_to(tmp_contact, difference);
            }
        }
        if (tmp_contact.empty())
            continue;

        // Now apply the contact areas to the layer were they need to be made.
        {
            // Get the average nozzle diameter used on this layer.
            vector<double> nozzle_diameters;
            for (auto region : layer->regions) {
                nozzle_diameters.push_back(config->nozzle_diameter.get_at(static_cast<size_t>(
                                                                              region->region()->config
                                                                                  .perimeter_extruder - 1)));
                nozzle_diameters.push_back(config->nozzle_diameter.get_at(static_cast<size_t>(
                                                                              region->region()->config
                                                                                  .infill_extruder - 1)));
                nozzle_diameters.push_back(config->nozzle_diameter.get_at(static_cast<size_t>(
                                                                              region->region()->config
                                                                                  .solid_infill_extruder - 1)));
            }

            int nozzle_diameters_count = static_cast<int>(!nozzle_diameters.empty() ? nozzle_diameters.size() : 1);
            auto nozzle_diameter =
                accumulate(nozzle_diameters.begin(), nozzle_diameters.end(), 0.0) / nozzle_diameters_count;

            coordf_t contact_z = layer->print_z - contact_distance(layer->height, nozzle_diameter);

            // Ignore this contact area if it's too low.
            if (contact_z < conf.first_layer_height - EPSILON)
                continue;

            contact[contact_z] = tmp_contact;
            overhang[contact_z] = tmp_overhang;
        }
    }

    return make_pair(contact, overhang);
}

map<coordf_t, Polygons>
SupportMaterial::object_top(PrintObject *object, map<coordf_t, Polygons> *contact)
{
    // find object top surfaces
    // we'll use them to clip our support and detect where does it stick.
    map<coordf_t, Polygons> top;
    if (object_config->support_material_buildplate_only.value)
        return top;

    Polygons projection;
    for (auto i = static_cast<int>(object->layers.size()) - 1; i >= 0; i--) {

        Layer *layer = object->layers[i];
        SurfacesPtr m_top;

        for (auto r : layer->regions)
            append_to(m_top, r->slices.filter_by_type(stTop));

        if (m_top.empty()) continue;

        // compute projection of the contact areas above this top layer
        // first add all the 'new' contact areas to the current projection
        // ('new' means all the areas that are lower than the last top layer
        // we considered).
        double min_top = (!top.empty() ? top.begin()->first : contact->rbegin()->first);

        // Use <= instead of just < because otherwise we'd ignore any contact regions
        // having the same Z of top layers.
        for (auto el : *contact)
            if (el.first > layer->print_z && el.first <= min_top)
                for (const auto &p : el.second)
                    projection.push_back(p);

        // Now find whether any projection falls onto this top surface.
        Polygons touching = intersection(projection, p(m_top));
        if (!touching.empty()) {
            // Grow top surfaces so that interface and support generation are generated
            // with some spacing from object - it looks we don't need the actual
            // top shapes so this can be done here.
            top[layer->print_z] = offset(touching, flow.scaled_width());
        }

        // Remove the areas that touched from the projection that will continue on
        // next, lower, top surfaces.
        projection = diff(projection, touching);

    }
    return top;
}

void
SupportMaterial::generate_pillars_shape(const map<coordf_t, Polygons> &contact,
                                        const vector<coordf_t> &support_z,
                                        map<int, Polygons> &shape)
{
    // This prevents supplying an empty point set to BoundingBox constructor.
    if (contact.empty()) return;

    coord_t pillar_size = scale_(object_config->support_material_pillar_size.value);
    coord_t pillar_spacing = scale_(object_config->support_material_pillar_spacing.value);

    Polygons grid;
    {
        auto pillar = Polygon({
                                  Point(0, 0),
                                  Point(pillar_size, coord_t(0)),
                                  Point(pillar_size, pillar_size),
                                  Point(coord_t(0), pillar_size)
                              });

        Polygons pillars;
        BoundingBox bb;
        {
            Points bb_points;
            for (auto contact_el : contact) {
                append_to(bb_points, to_points(contact_el.second));
            }
            bb = BoundingBox(bb_points);
        }

        for (auto x = bb.min.x; x <= bb.max.x - pillar_size; x += pillar_spacing) {
            for (auto y = bb.min.y; y <= bb.max.y - pillar_size; y += pillar_spacing) {
                pillars.push_back(pillar);
                pillar.translate(x, y);
            }
        }

        grid = union_(pillars);
    }
    // Add pillars to every layer.
    for (size_t i = 0; i < support_z.size(); i++) {
        shape[i] = grid;
    }
    // Build capitals.
    for (size_t i = 0; i < support_z.size(); i++) {
        coordf_t z = support_z[i];

        auto capitals = intersection(
            grid,
            contact.count(z) > 0 ? contact.at(z) : Polygons()
        );
        // Work on one pillar at time (if any) to prevent the capitals from being merged
        // but store the contact area supported by the capital because we need to make
        // sure nothing is left.
        Polygons contact_supported_by_capitals;
        for (auto capital : capitals) {
            // Enlarge capital tops.
            auto capital_polygons = offset(Polygons({capital}), +(pillar_spacing - pillar_size) / 2);
            append_to(contact_supported_by_capitals, capital_polygons);

            for (int j = i - 1; j >= 0; j--) {
                capital_polygons = offset(Polygons{capital}, -interface_flow.scaled_width() / 2);
                if (capitals.empty()) break;
                append_to(shape[i], capital_polygons);
            }
        }
        // Work on one pillar at time (if any) to prevent the capitals from being merged
        // but store the contact area supported by the capital because we need to make
        // sure nothing is left.
        auto contact_not_supported_by_capitals = diff(
            contact.count(z) > 0 ? contact.at(z) : Polygons(),
            contact_supported_by_capitals
        );

        if (!contact_not_supported_by_capitals.empty()) {
            for (int j = i - 1; j >= 0; j--) {
                append_to(shape[j], contact_not_supported_by_capitals);
            }
        }
    }
}

map<int, Polygons>
SupportMaterial::generate_base_layers(vector<coordf_t> support_z,
                                      map<coordf_t, Polygons> contact,
                                      map<int, Polygons> _interface,
                                      map<coordf_t, Polygons> top)
{
    // Let's now generate support layers under interface layers.
    map<int, Polygons> base;
    {
        for (auto i = static_cast<int>(support_z.size()) - 1; i >= 0; i--) {
            auto overlapping_layers = this->overlapping_layers(i, support_z);
            vector<coordf_t> overlapping_z;
            for (auto el : overlapping_layers)
                overlapping_z.push_back(support_z[el]);

            // In case we have no interface layers, look at upper contact
            // (1 interface layer means we only have contact layer, so $interface->{$i+1} is empty).
            Polygons upper_contact;
            if (object_config->support_material_interface_layers.value <= 1) {
                append_to(upper_contact, ((size_t)i + 1 < support_z.size() ? contact[support_z[i + 1]] : contact[-1]));
            }

            Polygons ps_1;
            append_to(ps_1, base[i + 1]); // support regions on upper layer.
            append_to(ps_1, _interface[i + 1]); // _interface regions on upper layer
            append_to(ps_1, upper_contact); // contact regions on upper layer

            Polygons ps_2;
            for (auto el : overlapping_z) {
                if (top.count(el) > 0)
                    append_to(ps_2, top[el]); // top slices on this layer.
                if (_interface.count(el) > 0)
                    append_to(ps_2, _interface[el]); // _interface regions on this layer.
                if (contact.count(el) > 0)
                    append_to(ps_2, contact[el]); // contact regions on this layer.
            }

            base[i] = diff(
                ps_1,
                ps_2,
                1
            );
        }
    }
    return base;
}

map<int, Polygons>
SupportMaterial::generate_interface_layers(vector<coordf_t> support_z,
                                           map<coordf_t, Polygons> contact,
                                           map<coordf_t, Polygons> top)
{
    // let's now generate interface layers below contact areas.
    map<int, Polygons> _interface;
    auto interface_layers_num = object_config->support_material_interface_layers.value;

    for (int layer_id = 0; layer_id < (int)support_z.size(); layer_id++) {
        auto z = support_z[layer_id];

        if (contact.count(z) <= 0)
            continue;
        Polygons &_contact = contact[z];

        // Count contact layer as interface layer.
        for (int i = layer_id - 1; i >= 0 && i > layer_id - interface_layers_num; i--) {
            auto overlapping_layers = this->overlapping_layers(i, support_z);
            vector<coordf_t> overlapping_z;
            for (auto z_el : overlapping_layers)
                overlapping_z.push_back(support_z[z_el]);

            // Compute interface area on this layer as diff of upper contact area
            // (or upper interface area) and layer slices.
            // This diff is responsible of the contact between support material and
            // the top surfaces of the object. We should probably offset the top
            // surfaces vertically before performing the diff, but this needs
            // investigation.
            Polygons ps_1;
            append_to(ps_1, _contact); // clipped projection of the current contact regions.
            append_to(ps_1, _interface[i]); // _interface regions already applied to this layer.

            Polygons ps_2;
            for (auto el : overlapping_z) {
                if (top.count(el) > 0)
                    append_to(ps_2, top[el]); // top slices on this layer.
                if (contact.count(el) > 0)
                    append_to(ps_2, contact[el]); // contact regions on this layer.
            }

            _contact = _interface[i] = diff(
                ps_1,
                ps_2,
                true
            );
        }
    }
    return _interface;
}

void
SupportMaterial::generate_bottom_interface_layers(const vector<coordf_t> &support_z,
                                                  map<int, Polygons> &base,
                                                  map<coordf_t, Polygons> &top,
                                                  map<int, Polygons> &_interface)
{
    // If no interface layers are allowed, don't generate bottom interface layers.
    if (object_config->support_material_interface_layers.value == 0)
        return;

    auto area_threshold = interface_flow.scaled_spacing() * interface_flow.scaled_spacing();

    // Loop through object's top surfaces. TODO CHeck if the keys are sorted.
    for (auto &top_el : top) {
        // Keep a count of the interface layers we generated for this top surface.
        int interface_layers = 0;

        // Loop through support layers until we find the one(s) right above the top
        // surface.
        for (size_t layer_id = 0; layer_id < support_z.size(); layer_id++) {
            auto z = support_z[layer_id];
            if (z <= top_el.first) // next unless $z > $top_z;
                continue;

            if (base.count(layer_id) > 0) {
                // Get the support material area that should be considered interface.
                auto interface_area = intersection(
                    base[layer_id],
                    top_el.second
                );

                // Discard too small areas.
                Polygons new_interface_area;
                for (auto p : interface_area) {
                    if (abs(p.area()) >= area_threshold)
                        new_interface_area.push_back(p);
                }
                interface_area = new_interface_area;

                // Subtract new interface area from base.
                base[layer_id] = diff(
                    base[layer_id],
                    interface_area
                );

                // Add the new interface area to interface.
                append_to(_interface[layer_id], interface_area);
            }

            interface_layers++;
            if (interface_layers == object_config->support_material_interface_layers.value)
                layer_id = static_cast<int>(support_z.size());
        }
    }
}

coordf_t
SupportMaterial::contact_distance(coordf_t layer_height, coordf_t nozzle_diameter)
{
    coordf_t extra = static_cast<float>(object_config->support_material_contact_distance.value);
    if (extra == 0) {
        return layer_height;
    }
    else {
        return nozzle_diameter + extra;
    }
}

vector<int>
SupportMaterial::overlapping_layers(int layer_idx, const vector<coordf_t> &support_z)
{
    vector<int> ret;

    coordf_t z_max = support_z[layer_idx];
    coordf_t z_min = layer_idx == 0 ? 0 : support_z[layer_idx - 1];

    for (int i = 0; i < (int) support_z.size(); i++) {
        if (i == layer_idx) continue;

        coordf_t z_max2 = support_z[i];
        coordf_t z_min2 = i == 0 ? 0 : support_z[i - 1];

        if (z_max > z_min2 && z_min < z_max2)
            ret.push_back(i);
    }

    return ret;
}

void
SupportMaterial::clip_with_shape(map<int, Polygons> &support, map<int, Polygons> &shape)
{
    for (auto layer : support) {
        // Don't clip bottom layer with shape so that we
        // can generate a continuous base flange
        // also don't clip raft layers
        if (layer.first == 0) continue;
        else if (layer.first < object_config->raft_layers) continue;

        layer.second = intersection(layer.second, shape[layer.first]);
    }
}

void
SupportMaterial::clip_with_object(map<int, Polygons> &support, vector<coordf_t> support_z, PrintObject &object)
{
    int i = 0;
    for (auto support_layer: support) {
        if (support_layer.second.empty()) {
            i++;
            continue;
        }
        coordf_t z_max = support_z[i];
        coordf_t z_min = (i == 0) ? 0 : support_z[i - 1];

        LayerPtrs layers;
        for (auto layer : object.layers) {
            if (layer->print_z > z_min && (layer->print_z - layer->height) < z_max) {
                layers.push_back(layer);
            }
        }

        // $layer->slices contains the full shape of layer, thus including
        // perimeter's width. $support contains the full shape of support
        // material, thus including the width of its foremost extrusion.
        // We leave a gap equal to a full extrusion width. TODO ask about this line @samir
        Polygons slices;
        for (Layer *l : layers) {
            for (auto s : l->slices.contours()) {
                slices.push_back(s);
            }
        }
        support_layer.second = diff(support_layer.second, offset(slices, flow.scaled_width()));
    }
    /*
        $support->{$i} = diff(
            $support->{$i},
            offset([ map @$_, map @{$_->slices}, @layers ], +$self->flow.scaled_width),
        );
     */
}

void
SupportMaterial::process_layer(int layer_id, toolpaths_params params)
{
    SupportLayer *layer = this->object->support_layers[layer_id];
    coordf_t z = layer->print_z;

    // We redefine flows locally by applyinh this layer's height.
    Flow _flow = flow;
    Flow _interface_flow = interface_flow;
    _flow.height = static_cast<float>(layer->height);
    _interface_flow.height = static_cast<float>(layer->height);

    Polygons overhang = this->overhang.count(z) > 0 ? this->overhang[z] : Polygons();
    Polygons contact = this->contact.count(z) > 0 ? this->contact[z] : Polygons();
    Polygons _interface = this->_interface.count(layer_id) > 0 ? this->_interface[layer_id] : Polygons();
    Polygons base = this->base.count(layer_id) > 0 ? this->base[layer_id] : Polygons();

    // Islands.
    {
        Polygons ps;
        append_to(ps, base);
        append_to(ps, contact);
        append_to(ps, _interface);
        layer->support_islands.append(union_ex(ps));
    }

    // Contact.
    Polygons contact_infill;
    if (object_config->support_material_interface_layers.value == 0) {
        // If no interface layers were requested we treat the contact layer
        // exactly as a generic base layer.
        append_to(base, contact);
    }
    else if (!contact.empty() && params.contact_loops > 0) { // Generate the outermost loop.
        // Find the centerline of the external loop (or any other kind of extrusions should the loop be skipped).
        contact = offset(contact, -_interface_flow.scaled_width() / 2);

        Polygons loops0;
        {
            // Find centerline of the external loop of the contours.
            auto external_loops = contact;

            // Only consider the loops facing the overhang.
            {
                auto overhang_with_margin = offset(overhang, +_interface_flow.scaled_width() / 2);
                {
                    Polygons ps;
                    for (auto p : external_loops) {
                        if (!intersection_pl(
                            p.split_at_first_point(),
                            overhang_with_margin).empty()
                            )
                            ps.push_back(p);
                    }
                    external_loops = ps;
                }
            }

            // Apply a pattern to the loop.
            Points positions;
            for (auto p : external_loops)
                append_to(positions, Polygon(p).equally_spaced_points(params.circle_distance));
            Polygons circles;
            for (auto pos : positions) {
                circles.push_back(params.circle);
                circles.back().translate(pos.x, pos.y);
            }
            loops0 = diff(
                external_loops,
                circles
            );
        }

        // TODO Revise the loop range.
        // Make more loops.
        auto loops = loops0;
        for (int i = 2; i <= params.contact_loops; i++) {
            auto d = (i - 1) * _interface_flow.scaled_spacing();
            append_to(loops,
                      offset2(loops0,
                              -d - 0.5 * _interface_flow.scaled_spacing(),
                              +0.5 * _interface_flow.scaled_spacing()));
        }

        // Clip such loops to the side oriented towards the object.
        Polylines ps;
        for (auto p : loops)
            ps.push_back(p.split_at_first_point());

        auto new_loops = intersection_pl(
            ps,
            offset(overhang, scale_(SUPPORT_MATERIAL_MARGIN))
        );

        // Add the contact infill area to the interface area
        // note that growing loops by $circle_radius ensures no tiny
        // extrusions are left inside the circles; however it creates
        // a very large gap between loops and contact_infill, so maybe another
        // solution should be found to achieve both goals.
        {
            Polygons ps;
            for (auto pl : new_loops)
                append_to(ps, offset(pl, params.circle_radius * 1.1));

            contact_infill = diff(
                contact,
                ps
            );
        }

        // Transform loops into ExtrusionPath objects.
        auto mm3_per_mm = _interface_flow.mm3_per_mm();
        ExtrusionPaths loops_extrusion_paths;
        for (auto l : new_loops) {
            ExtrusionPath extrusion_path(erSupportMaterialInterface);
            extrusion_path.polyline = l;
            extrusion_path.mm3_per_mm = mm3_per_mm;
            extrusion_path.width = _interface_flow.width;
            extrusion_path.height = layer->height;
            loops_extrusion_paths.push_back(extrusion_path);
        }

        layer->support_interface_fills.append(loops_extrusion_paths);
    }

    // Allocate the fillers exclusively in the worker threads! Don't allocate them at the main thread,
    // as Perl copies the C++ pointers by default, so then the C++ objects are shared between threads!
    map<string, Fill *> fillers;
    fillers["interface"] = Fill::new_from_type("rectilinear");
    fillers["support"] = Fill::new_from_type(InfillPattern(params.pattern)); // FIXME @Samir55

    BoundingBox bounding_box = object->bounding_box();
    fillers["interface"]->bounding_box = bounding_box;
    fillers["support"]->bounding_box = bounding_box;

    // Interface and contact infill.
    if (!_interface.empty() || !contact_infill.empty()) {
        // Make interface layers alternate angles by 90 degrees.
        double alternate_angle = params.interface_angle + (90 * ((layer_id + 1) % 2));
        fillers["interface"]->angle = static_cast<float>(Geometry::deg2rad(alternate_angle));
        fillers["interface"]->min_spacing = _interface_flow.spacing();

        // Find centerline of the external loop.
        _interface = offset2(_interface, +SCALED_EPSILON, -(SCALED_EPSILON + _interface_flow.scaled_width() / 2));

        // Join regions by offsetting them to ensure they're merged.
        {
            Polygons ps;
            append_to(ps, _interface);
            append_to(ps, contact_infill);
            _interface = offset(ps, SCALED_EPSILON);
        }

        // turn base support into interface when it's contained in our holes
        // (this way we get wider interface anchoring).
        {
            Polygons ps = _interface;
            _interface = Polygons();
            for (auto p: ps) {
                if (p.is_clockwise()) {
                    Polygon p2 = p;
                    p2.make_counter_clockwise();
                    if (diff(Polygons({p2}), base, 1).empty())
                        continue;
                    _interface.push_back(p);
                }
            }
        }
        base = diff(base, _interface);

        ExtrusionPaths paths;
        ExPolygons expolygons = union_ex(_interface);
        for (auto expolygon : expolygons) {
            Surface surface(stInternal, expolygon);
            fillers["interface"]->density = params.interface_density;
            fillers["interface"]->complete = true;
            // TODO What layer height come from FIXME. or Polyline collection
            Polylines ps = fillers["interface"]->fill_surface(surface);

            auto mm3_per_mm = _interface_flow.mm3_per_mm();

            for (auto p : ps) {
                ExtrusionPath extrusion_path(erSupportMaterialInterface);
                extrusion_path.polyline = p;
                extrusion_path.mm3_per_mm = mm3_per_mm;
                extrusion_path.width = _interface_flow.width;
                extrusion_path.height = layer->height;
                paths.push_back(extrusion_path);
            }
        }

        layer->support_interface_fills.append(paths);
    }

    // Support or flange
    if (!base.empty()) {
        Fill *filler = fillers["support"];
        filler->angle = static_cast<float>(Geometry::deg2rad(params.angles[layer_id % int(params.angles.size())]));

        // We don't use $base_flow.spacing because we need a constant spacing
        // value that guarantees that all layers are correctly aligned.
        filler->min_spacing = flow.spacing();

        auto density = params.support_density;
        Flow *base_flow = &_flow;

        // Find centerline of the external loop/extrusions.
        Polygons to_infill = offset2(base, +SCALED_EPSILON, -(SCALED_EPSILON + _flow.scaled_width() / 2));
        ExPolygons new_to_infill;

        ExtrusionPaths paths;

        // Base flange.
        if (layer_id == 0) {
            filler = fillers["interface"];
            filler->angle = static_cast<float>(Geometry::deg2rad(object_config->support_material_angle.value + 90));
            density = 0.5;
            base_flow = &first_layer_flow;

            // Use the proper spacing for first layer as we don't need to align
            // its pattern to the other layers.
            filler->min_spacing = base_flow->spacing();

            // Subtract brim so that it goes around the object fully (and support gets its own brim).
            if (config->brim_width.value > 0) {
                float d = +scale_(config->brim_width.value * 2);

                new_to_infill = diff_ex(
                    to_infill,
                    offset(to_polygons(object->get_layer(0)->slices), d)
                );
            }
            else {
                new_to_infill = union_ex(to_infill);
            }
        }
        else {
            // Draw a perimeter all around support infill.
            // TODO: use brim ordering algorithm.
            auto mm3_per_mm = _flow.mm3_per_mm();
            for (auto p : to_infill) {
                ExtrusionPath extrusionPath(erSupportMaterial);
                extrusionPath.polyline = p.split_at_first_point();
                extrusionPath.mm3_per_mm = mm3_per_mm;
                extrusionPath.width = _flow.width;
                extrusionPath.height = layer->height;

                paths.push_back(extrusionPath);
            }

            // TODO: use offset2_ex()
            new_to_infill = offset_ex(to_infill, -_flow.scaled_spacing());
        }

        auto mm3_per_mm = base_flow->mm3_per_mm();
        for (const auto &expolygon : new_to_infill) {
            Surface surface(stInternal, expolygon);
            filler->density = static_cast<float>(density);
            filler->complete = true;
            // TODO What layer height come from FIXME. or Polyline collection
            Polylines ps = filler->fill_surface(surface);

            for (auto pl : ps) {
                ExtrusionPath extrusionPath(erSupportMaterial);
                extrusionPath.polyline = pl;
                extrusionPath.mm3_per_mm = mm3_per_mm;
                extrusionPath.width = base_flow->width;
                extrusionPath.height = static_cast<float>(layer->height);

                paths.push_back(extrusionPath);
            }
        }

        layer->support_fills.append(paths);
    }
}

Polygons
SupportMaterial::p(SurfacesPtr &surfaces)
{
    Polygons ret;
    for (auto surface : surfaces) {
        ret.push_back(surface->expolygon.contour);
        for (const auto &hole_polygon : surface->expolygon.holes) {
            ret.push_back(hole_polygon);
        }
    }
    return ret;
}

void
SupportMaterial::append_polygons(Polygons &dst, Polygons &src)
{
    for (const auto polygon : src) {
        dst.push_back(polygon);
    }
}

vector<coordf_t>
SupportMaterial::get_keys_sorted(map<coordf_t, Polygons> _map)
{
    vector<coordf_t> ret;
    for (auto el : _map)
        ret.push_back(el.first);
    sort(ret.begin(), ret.end());
    return ret;
}

coordf_t
SupportMaterial::get_max_layer_height(PrintObject *object)
{
    coordf_t ret = -1;
    for (auto layer : object->layers)
        ret = max(ret, layer->height);
    return ret;
}

Polygon
SupportMaterial::create_circle(coordf_t radius)
{
    Points points;
    coordf_t positions[] = {5 * PI / 3,
                            4 * PI / 3,
                            PI,
                            2 * PI / 3,
                            PI / 3,
                            0};
    for (auto pos : positions) {
        points.emplace_back(radius * cos(pos), (radius * sin(pos)));
    }

    return Polygon(points);
}

}

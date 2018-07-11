#include "SupportMaterial.hpp"

namespace Slic3r
{
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
        points.push_back(Point(radius * cos(pos), (radius * sin(pos))));
    }

    return Polygon(points);
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

vector<coordf_t>
SupportMaterial::support_layers_z(vector<coordf_t> contact_z,
                                  vector<coordf_t> top_z,
                                  coordf_t max_object_layer_height)
{
    // Quick table to check whether a given Z is a top surface.
    map<float, bool> is_top;
    for (auto z : top_z) is_top[z] = true;

    // determine layer height for any non-contact layer
    // we use max() to prevent many ultra-thin layers to be inserted in case
    // layer_height > nozzle_diameter * 0.75.
    auto nozzle_diameter = config->nozzle_diameter.get_at(object_config->support_material_extruder - 1);
    auto support_material_height = (max_object_layer_height, (nozzle_diameter * 0.75));
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
        for (int j = 0; j < object_config->raft_layers - 2; j++) {
            float z_new =
                roundf(static_cast<float>((z[0] + height * idx) * 100)) / 100; // round it to 2 decimal places.
            z.insert(z.begin() + idx, z_new);
            idx++;
        }
    }

    // Create other layers (skip raft layers as they're already done and use thicker layers).
    for (size_t i = z.size(); i >= object_config->raft_layers; i--) {
        coordf_t target_height = support_material_height;
        if (i > 0 && is_top[z[i - 1]]) {
            target_height = nozzle_diameter;
        }

        // Enforce first layer height.
        if ((i == 0 && z[i] > target_height + first_layer_height)
            || (z[i] - z[i - 1] > target_height + EPSILON)) {
            z.insert(z.begin() + i, (z[i] - target_height));
            i++;
        }
    }

    // Remove duplicates and make sure all 0.x values have the leading 0.
    {
        set<coordf_t> s;
        for (auto el : z)
            s.insert(roundf(static_cast<float>((el * 100)) / 100)); // round it to 2 decimal places.
        z = vector<coordf_t>();
        for (auto el : s)
            z.push_back(el);
    }

    return z;
}

vector<int>
SupportMaterial::overlapping_layers(int layer_idx, vector<coordf_t> support_z)
{
    vector<int> ret;

    coordf_t z_max = support_z[layer_idx];
    coordf_t z_min = layer_idx == 0 ? 0 : support_z[layer_idx - 1];

    for (int i = 0; i < support_z.size(); i++) {
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
        support_layer.second = diff(support_layer.second, offset(slices, flow->scaled_width()));
    }
    /*
        $support->{$i} = diff(
            $support->{$i},
            offset([ map @$_, map @{$_->slices}, @layers ], +$self->flow->scaled_width),
        );
     */
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
    for (auto i = static_cast<int>(object->layers.size() - 1); i >= 0; i--) {

        Layer *layer = object->layers[i];
        SurfacesPtr m_top;

        for (auto r : layer->regions)
            for (auto s : r->slices.filter_by_type(stTop))
                m_top.push_back(s);

        if (!m_top.empty()) {
            // compute projection of the contact areas above this top layer
            // first add all the 'new' contact areas to the current projection
            // ('new' means all the areas that are lower than the last top layer
            // we considered).
            // TODO Ask about this line
            /*
             my $min_top = min(keys %top) // max(keys %$contact);
             */
            double min_top = top.begin()->first;

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
                top[layer->print_z] = offset(touching, flow->scaled_width());
            }

            // Remove the areas that touched from the projection that will continue on
            // next, lower, top surfaces.
            projection = diff(projection, touching);
        }
    }
    return top;
}

void
SupportMaterial::generate_toolpaths(PrintObject *object,
                                    map<coordf_t, Polygons> overhang,
                                    map<coordf_t, Polygons> contact,
                                    map<int, Polygons> interface,
                                    map<int, Polygons> base)
{
    // Assig the object to the supports class.
    this->object = object;

    // Shape of contact area.
    int contact_loops = 1;
    coordf_t circle_radius = 1.5 * interface_flow->scaled_width();
    coordf_t circle_distance = 3 * circle_radius;
    Polygon circle = create_circle(circle_radius);

    // TODO Add Slic3r debug. "Generating patterns\n"
    // Prepare fillers.
    auto pattern = object_config->support_material_pattern;
    vector<int> angles;
    angles.push_back(object_config->support_material_angle.value);

    if (pattern == smpRectilinearGrid) {
        pattern = smpRectilinear;
        angles.push_back(angles[0] + 90);
    }
    else if (pattern == smpPillars) {
        pattern = smpHoneycomb;
    }

    auto interface_angle = object_config->support_material_angle.value + 90;
    auto interface_spacing = object_config->support_material_interface_spacing.value + interface_flow->spacing();
    auto interface_density = interface_spacing == 0 ? 1 : interface_flow->spacing() / interface_spacing;
    auto support_spacing = object_config->support_material_spacing + flow->spacing();
    auto support_density = support_spacing == 0 ? 1 : flow->spacing() / support_spacing;

    parallelize<size_t>(
        0,
        object->support_layers.size() - 1,
        boost::bind(&SupportMaterial::process_layer, this, _1),
        this->config->threads.value
    );
}

pair<map<coordf_t, Polygons>, map<coordf_t, Polygons>>
SupportMaterial::contact_area(PrintObject *object)
{
    PrintObjectConfig conf = this->object_config;

    // If user specified a custom angle threshold, convert it to radians.
    float threshold_rad;
    cout << conf.support_material_threshold << endl;
    if (conf.support_material_threshold > 0) {
        threshold_rad = static_cast<float>(Geometry::deg2rad(
            conf.support_material_threshold.value + 1)); // +1 makes the threshold inclusive
        // TODO @Samir55 add debug statetments.
    }

    // Build support on a build plate only? If so, then collect top surfaces into $buildplate_only_top_surfaces
    // and subtract buildplate_only_top_surfaces from the contact surfaces, so
    // there is no contact surface supported by a top surface.
    bool buildplate_only =
        (conf.support_material || conf.support_material_enforce_layers)
            && conf.support_material_buildplate_only;
    Polygons buildplate_only_top_surfaces;

    // Determine contact areas.
    map<coordf_t, Polygons> contact;
    map<coordf_t, Polygons> overhang; // This stores the actual overhang supported by each contact layer

    for (int layer_id = 0; layer_id < object->layers.size(); layer_id++) {
        // Note $layer_id might != $layer->id when raft_layers > 0
        // so $layer_id == 0 means first object layer
        // and $layer->id == 0 means first print layer (including raft).

        // If no raft, and we're at layer 0, skip to layer 1
        if (conf.raft_layers == 0 && layer_id == 0)
            continue;

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
                for (auto polygon : p(top_surfaces)) {
                    projection_new.push_back(polygon);
                }
            }
            if (!projection_new.empty()) {
                // Merge the new top surfaces with the preceding top surfaces.
                // Apply the safety offset to the newly added polygons, so they will connect
                // with the polygons collected before,
                // but don't apply the safety offset during the union operation as it would
                // inflate the polygons over and over.
                Polygons polygons = offset(projection_new, scale_(0.01));
                append_polygons(buildplate_only_top_surfaces, polygons);

                buildplate_only_top_surfaces = union_(buildplate_only_top_surfaces, 0);
            }
        }

        // Detect overhangs and contact areas needed to support them.
        Polygons m_overhang, m_contact;
        if (layer_id == 0) {
            // this is the first object layer, so we're here just to get the object
            // footprint for the raft.
            // we only consider contours and discard holes to get a more continuous raft.
            for (auto const &contour : layer->slices.contours()) {
                auto contour_clone = contour;
                m_overhang.push_back(contour_clone);
            }

            Polygons polygons = offset(m_overhang, scale_(+SUPPORT_MATERIAL_MARGIN));
            append_polygons(m_contact, polygons);
        }
        else {
            Layer *lower_layer = object->get_layer(layer_id - 1);
            for (auto layerm : layer->regions) {
                auto fw = layerm->flow(frExternalPerimeter).scaled_width();
                Polygons difference;

                // If a threshold angle was specified, use a different logic for detecting overhangs.
                if ((conf.support_material && threshold_rad > 0)
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
                                       * ((cos(layer_threshold_rad)) / (sin(layer_threshold_rad))));
                    }

                    // TODO Check.
                    difference = diff(
                        Polygons(layerm->slices),
                        offset(lower_layer->slices, +d)
                    );

                    // only enforce spacing from the object ($fw/2) if the threshold angle
                    // is not too high: in that case, $d will be very small (as we need to catch
                    // very short overhangs), and such contact area would be eaten by the
                    // enforced spacing, resulting in high threshold angles to be almost ignored
                    if (d > fw / 2) {
                        difference = diff(
                            offset(difference, d - fw / 2),
                            lower_layer->slices);
                    }
                }
                else {
                    difference = diff(
                        Polygons(layerm->slices),
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
                        auto bridge_flow = layerm->flow(FlowRole::frPerimeter, 1);

                        // Get the lower layer's slices and grow them by half the nozzle diameter
                        // because we will consider the upper perimeters supported even if half nozzle
                        // falls outside the lower slices.
                        Polygons lower_grown_slices;
                        {
                            coordf_t nozzle_diameter = this->config->nozzle_diameter
                                .get_at(static_cast<size_t>(layerm->region()->config.perimeter_extruder - 1));

                            lower_grown_slices = offset(
                                lower_layer->slices,
                                scale_(nozzle_diameter / 2)
                            );
                        }

                        // Get all perimeters as polylines.
                        // TODO: split_at_first_point() (called by as_polyline() for ExtrusionLoops)
                        // could split a bridge mid-way.
                        Polylines overhang_perimeters;
                        overhang_perimeters.push_back(layerm->perimeters.flatten().as_polyline());

                        // Only consider the overhang parts of such perimeters,
                        // overhangs being those parts not supported by
                        // workaround for Clipper bug, see Slic3r::Polygon::clip_as_polyline()
                        overhang_perimeters[0].translate(1, 0);
                        overhang_perimeters = diff_pl(overhang_perimeters, lower_grown_slices);

                        // Only consider straight overhangs.
                        Polylines new_overhangs_perimeters_polylines;
                        for (auto p : overhang_perimeters)
                            if (p.is_straight())
                                new_overhangs_perimeters_polylines.push_back(p);

                        overhang_perimeters = new_overhangs_perimeters_polylines;
                        new_overhangs_perimeters_polylines = Polylines();

                        // Only consider overhangs having endpoints inside layer's slices
                        for (auto &p : overhang_perimeters) {
                            p.extend_start(fw);
                            p.extend_end(fw);
                        }

                        for (auto p : overhang_perimeters) {
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
                                                 layerm->flow(FlowRole::frPerimeter).scaled_width()};

                            auto w = *max_element(widths, widths + 4);

                            // Also apply safety offset to ensure no gaps are left in between.
                            // TODO CHeck this.
                            for (auto &p : overhang_perimeters) {
                                Polygons ps = union_(offset(p, w / 2 + 10));
                                for (auto ps_el : ps)
                                    bridged_perimeters.push_back(ps_el);
                            }
                        }
                    }

                    if (1) {
                        // Remove the entire bridges and only support the unsupported edges.
                        ExPolygons bridges;
                        for (auto surface : layerm->fill_surfaces.filter_by_type(stBottomBridge)) {
                            if (surface->bridge_angle != -1) {
                                bridges.push_back(surface->expolygon);
                            }
                        }

                        ExPolygons ps;
                        for (auto p : bridged_perimeters)
                            ps.push_back(ExPolygon(p));
                        for (auto p : bridges)
                            ps.push_back(p);

                        // TODO ASK about this.
                        difference = diff(
                            difference,
                            to_polygons(ps),
                            true
                        );

                        // TODO Ask about this.
                        auto p_intersections = intersection(offset(layerm->unsupported_bridge_edges.polylines,
                                                                   +scale_(SUPPORT_MATERIAL_MARGIN)),
                                                            to_polygons(bridges));
                        for (auto p: p_intersections) {
                            difference.push_back(p);
                        }
                    }
                    else {
                        // just remove bridged areas.
                        difference = diff(
                            difference,
                            layerm->bridged,
                            1
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
                append_polygons(m_overhang, difference);

                // Let's define the required contact area by using a max gap of half the upper
                // extrusion width and extending the area according to the configured margin.
                //    We increment the area in steps because we don't want our support to overflow
                // on the other side of the object (if it's very thin).
                {
                    Polygons slices_margin = offset(lower_layer->slices, +fw / 2);

                    if (buildplate_only) {
                        // Trim the inflated contact surfaces by the top surfaces as well.
                        append_polygons(slices_margin, buildplate_only_top_surfaces);
                        slices_margin = union_(slices_margin);
                    }

                    // TODO Ask how to port this.
                    /*
                     for ($fw/2, map {scale MARGIN_STEP} 1..(MARGIN / MARGIN_STEP)) {
                        $diff = diff(
                            offset($diff, $_),
                            $slices_margin,
                        );
                     }
                     */
                }

                append_polygons(m_contact, difference);
            }
        }
        if (contact.empty())
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

            int nozzle_diameters_count = static_cast<int>(nozzle_diameters.size() > 0 ? nozzle_diameters.size() : 1);
            auto nozzle_diameter =
                accumulate(nozzle_diameters.begin(), nozzle_diameters.end(), 0.0) / nozzle_diameters_count;

            coordf_t contact_z = layer->print_z - contact_distance(layer->height, nozzle_diameter);

            // Ignore this contact area if it's too low.
            if (contact_z < conf.first_layer_height - EPSILON)
                continue;


            contact[contact_z] = m_contact;
            overhang[contact_z] = m_overhang;
        }
    }

    return make_pair(contact, overhang);
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
    // object's top surfaces this is the place to detect them. TODO


    // Propagate contact layers downwards to generate interface layers. TODO

    // Propagate contact layers and interface layers downwards to generate
    // the main support layers. TODO


    // Detect what part of base support layers are "reverse interfaces" because they
    // lie above object's top surfaces. TODO


    // Install support layers into object.
    for (int i = 0; i < support_z.size(); i++) {
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
    generate_toolpaths(object, overhang, contact, interface, base);

}

}

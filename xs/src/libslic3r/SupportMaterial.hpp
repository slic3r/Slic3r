#ifndef slic3r_SupportMaterial_hpp_
#define slic3r_SupportMaterial_hpp_

#include <numeric>
#include <vector>
#include <iostream>
#include <algorithm>
#include "libslic3r.h"
#include "PrintConfig.hpp"
#include "Flow.hpp"
#include "Layer.hpp"
#include "Geometry.hpp"
#include "Print.hpp"
#include "ClipperUtils.hpp"
#include "SupportMaterial.hpp"
#include "ExPolygon.hpp"
#include "SVG.hpp"

// Supports Material tests.


using namespace std;

namespace Slic3r
{

// how much we extend support around the actual contact area
constexpr coordf_t SUPPORT_MATERIAL_MARGIN = 1.5;

constexpr coordf_t PILLAR_SIZE = 2.5;

constexpr coordf_t PILLAR_SPACING = 10;

class SupportMaterial
{
public:
    PrintConfig *config; ///<
    PrintObjectConfig *object_config; ///<
    Flow *flow; ///<
    Flow *first_layer_flow; ///<
    Flow *interface_flow; ///<

    SupportMaterial(PrintConfig *print_config,
                    PrintObjectConfig *print_object_config,
                    Flow *flow = nullptr,
                    Flow *first_layer_flow = nullptr,
                    Flow *interface_flow = nullptr)
        : config(print_config),
          object_config(print_object_config),
          flow(flow),
          first_layer_flow(first_layer_flow),
          interface_flow(interface_flow)
    {}

    void generate()
    {}

    pair<Polygons, Polygons> contact_area(PrintObject *object)
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
        Surfaces buildplate_only_top_surfaces;

        // Determine contact areas.
        Polygons contact;
        Polygons overhang; // This stores the actual overhang supported by each contact layer

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

            auto layer = object->get_layer(layer_id);

            if (conf.support_material_max_layers
                && layer_id > conf.support_material_max_layers)
                break;

            ExPolygons buildplate_only_top_surfaces_polygons;
            if (buildplate_only) {
                // Collect the top surfaces up to this layer and merge them. TODO @Ask about this line.
                SurfacesPtr projection_new;
                for (auto const &region : layer->regions) {
                    SurfacesPtr top_surfaces = region->slices.filter_by_type(stTop);
                    for (auto top_surface : top_surfaces) {
                        projection_new.push_back(top_surface);
                    }
                }
                if (!projection_new.empty()) {
                    // Merge the new top surfaces with the preceding top surfaces.
                    // Apply the safety offset to the newly added polygons, so they will connect
                    // with the polygons collected before,
                    // but don't apply the safety offset during the union operation as it would
                    // inflate the polygons over and over.
                    for (Surface *surface : projection_new) {
                        Surfaces s = offset(*surface, scale_(0.01));
                        for (auto el : s)
                            buildplate_only_top_surfaces.push_back(el);
                    }
                    // TODO @Ask about this
                    buildplate_only_top_surfaces_polygons = union_ex(buildplate_only_top_surfaces, 0);
                }
            }

            // Detect overhangs and contact areas needed to support them.
            if (layer_id == 0) {
                // this is the first object layer, so we're here just to get the object
                // footprint for the raft.
                // we only consider contours and discard holes to get a more continuous raft.
                for (auto const &contour : layer->slices.contours()) {
                    auto contour_clone = contour;
                    overhang.push_back(contour_clone);
                }

                Polygons ps = offset(overhang, scale_(+SUPPORT_MATERIAL_MARGIN));
                for (auto p : ps)
                    contact.push_back(p);
            }
            else {
                Layer *lower_layer = object->get_layer(layer_id - 1);
                for (auto layerm : layer->regions) {
                    float fw = layerm->flow(frExternalPerimeter).scaled_width();
                    Polygons difference;

                    // If a threshold angle was specified, use a different logic for detecting overhangs.
                    if ((conf.support_material && threshold_rad > 0)
                        || layer_id <= conf.support_material_enforce_layers
                        || (conf.raft_layers > 0 && layer_id == 0)) {
                        float d = 0;
                        float layer_threshold_rad = threshold_rad;
                        if (layer_id <= conf.support_material_enforce_layers) {
                            // Use ~45 deg number for enforced supports if we are in auto.
                            layer_threshold_rad = Geometry::deg2rad(89);
                        }
                        if (layer_threshold_rad > 0) {
                            d = scale_(lower_layer->height
                                           * ((cos(layer_threshold_rad)) / (sin(layer_threshold_rad))));
                        }

                        // TODO Ask about this.
                        difference = diff(
                            layerm->slices,
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
                            layerm->slices,
                            offset(lower_layer->slices,
                                   static_cast<const float>(+conf.get_abs_value("support_material_threshols", fw)))
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
                                    .get_at(layerm->region()->config.perimeter_extruder - 1);

                                lower_grown_slices = offset(
                                    lower_layer->slices,
                                    scale_(nozzle_diameter / 2)
                                );
                            }

                            // Get all perimeters as polylines.
                            // TODO: split_at_first_point() (called by as_polyline() for ExtrusionLoops)
                            // could split a bridge mid-way.
                            Polyline overhang_perimeters_polyline = layerm->perimeters.flatten().as_polyline();
                            Polylines overhang_perimeters_polylines;

                            // Only consider the overhang parts of such perimeters,
                            // overhangs being those parts not supported by
                            // workaround for Clipper bug, see Slic3r::Polygon::clip_as_polyline()
                            // ASk About htis.
                            overhang_perimeters_polyline.translate(1, 0);
                            Polylines ps = diff_pl(overhang_perimeters_polyline, lower_grown_slices);
                            for (const auto &p : ps) {
                                overhang_perimeters_polylines.push_back(p);
                            }

                            // Only consider straight overhangs.
                            Polylines new_overhangs_perimeters_polylines;
                            for (auto p : overhang_perimeters_polylines) {
                                if (p.is_straight())
                                    new_overhangs_perimeters_polylines.push_back(p);
                            }
                            overhang_perimeters_polylines = new_overhangs_perimeters_polylines;
                            new_overhangs_perimeters_polylines = Polylines();

                            // Only consider overhangs having endpoints inside layer's slices
                            for (auto &p : overhang_perimeters_polylines) {
                                p.extend_start(fw);
                                p.extend_end(fw);
                            }

                            for (auto p : overhang_perimeters_polylines) {
                                if (layer->slices.contains_b(p.first_point())
                                    && layer->slices.contains_b(p.last_point())) {
                                    new_overhangs_perimeters_polylines.push_back(p);
                                }
                            }

                            overhang_perimeters_polylines = new_overhangs_perimeters_polylines;
                            new_overhangs_perimeters_polylines = Polylines();

                            // Convert bridging polylines into polygons by inflating them with their thickness.
                            {
                                // For bridges we can't assume width is larger than spacing because they
                                // are positioned according to non-bridging perimeters spacing.
                                coordf_t widths[] = {bridge_flow.scaled_width(),
                                                     bridge_flow.scaled_spacing(),
                                                     fw,
                                                     layerm->flow(FlowRole::frPerimeter).scaled_width()};

                                coordf_t w = *max_element(widths, widths + 4);

                                // Also apply safety offset to ensure no gaps are left in between.
                                for (auto &p : overhang_perimeters_polylines) {
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
                        // TODO Ask about this.
                        difference = diff(difference, to_polygons(buildplate_only_top_surfaces_polygons));
                    }

                    if (difference.empty()) continue;

                    // NOTE: this is not the full overhang as it misses the outermost half of the perimeter width!
                    for (auto p : difference) {
                        overhang.push_back(p);
                    }

                    // Let's define the required contact area by using a max gap of half the upper
                    // extrusion width and extending the area according to the configured margin.
                    //    We increment the area in steps because we don't want our support to overflow
                    // on the other side of the object (if it's very thin).
                    {
                        auto slices_margin = offset(lower_layer->slices, +fw / 2);

                        if (buildplate_only) {
                            // TODO Ask about this.
                            // Trim the inflated contact surfaces by the top surfaces as well.
                            for (auto s_p : to_polygons(buildplate_only_top_surfaces)) {
                                slices_margin.push_back(s_p);
                            }
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

                    for (auto p : difference)
                        contact.push_back(p);
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

                auto nozzle_diameter =
                    accumulate(nozzle_diameters.begin(), nozzle_diameters.end(), 0.0) / nozzle_diameters.size();

                auto contact_z = layer->print_z - contact_distance(layer->height, nozzle_diameter);

                // Ignore this contact area if it's too low.
                if (contact_z < conf.first_layer_height - EPSILON)
                    continue;

                // TODO ASK how to port this.
                /*
                     $contact{$contact_z}  = [ @contact ];
                     $overhang{$contact_z} = [ @overhang ];

                     require "Slic3r/SVG.pm";
                     Slic3r::SVG::output("out\\contact_" . $contact_z . ".svg",
                     green_expolygons => union_ex($buildplate_only_top_surfaces),
                     blue_expolygons  => union_ex(\@contact),
                     red_expolygons   => union_ex(\@overhang),
                     );
                 */
            }
        }
        return make_pair(contact, overhang);
    }

    ExPolygons *object_top(PrintObject *object, SurfacesPtr contact)
    {
        // find object top surfaces
        // we'll use them to clip our support and detect where does it stick.
        ExPolygons *top = new ExPolygons();
        if (object_config->support_material_buildplate_only.value)
            return top;

    }

    void generate_interface_layers()
    {}

    void generate_bottom_interface_layers()
    {}

    void generate_base_layers()
    {}

    void generate_toolpaths()
    {}

    void generate_pillars_shape()
    {}

    // This method removes object silhouette from support material
    // (it's used with interface and base only). It removes a bit more,
    // leaving a thin gap between object and support in the XY plane.
    void clip_with_object(map<int, Polygons> &support, vector<coordf_t> support_z, PrintObject &object)
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

    void clip_with_shape(map<int, Polygons> &support, map<int, Polygons> &shape)
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

    /// This method returns the indices of the layers overlapping with the given one.
    vector<int> overlapping_layers(int layer_idx, vector<float> support_z)
    {
        vector<int> ret;

        float z_max = support_z[layer_idx];
        float z_min = layer_idx == 0 ? 0 : support_z[layer_idx - 1];

        for (int i = 0; i < support_z.size(); i++) {
            if (i == layer_idx) continue;

            float z_max2 = support_z[i];
            float z_min2 = i == 0 ? 0 : support_z[i - 1];

            if (z_max > z_min2 && z_min < z_max2)
                ret.push_back(i);
        }

        return ret;
    }

    vector<coordf_t> support_layers_z(vector<float> contact_z, vector<float> top_z, coordf_t max_object_layer_height)
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

    coordf_t contact_distance(coordf_t layer_height, coordf_t nozzle_diameter)
    {
        coordf_t extra = static_cast<float>(object_config->support_material_contact_distance.value);
        if (extra == 0) {
            return layer_height;
        }
        else {
            return nozzle_diameter + extra;
        }
    }

};

class SupportMaterialTests
{
public:
    bool test_1()
    {
        // Create a mesh & modelObject.
        TriangleMesh mesh = TriangleMesh::make_cube(20, 20, 20);

        // Create modelObject.
        Model model = Model();
        ModelObject *object = model.add_object();
        object->add_volume(mesh);
        model.add_default_instances();

        // Align to origin.
        model.align_instances_to_origin();

        // Create Print.
        Print print = Print();

        // Configure the printObjectConfig.
        print.default_object_config.set_deserialize("support_material", "1");
        print.default_object_config.set_deserialize("layer_height", "0.2");
        print.config.set_deserialize("first_layer_height", "0.3");

        vector<float> contact_z;
        vector<float> top_z;
        contact_z.push_back(1.9);
        top_z.push_back(1.9);

        // Add the modelObject.
        print.add_model_object(model.objects[0]);

        // Create new supports.
        SupportMaterial support = SupportMaterial(&print.config, &print.objects.front()->config);

        vector<coordf_t>
            support_z = support.support_layers_z(contact_z, top_z, print.default_object_config.layer_height);

        bool
            is_1 = (support_z[0] == print.default_object_config.first_layer_height); // 'first layer height is honored'.

        bool is_2 = false; // 'no null or negative support layers'.
        for (int i = 1; i < support_z.size(); ++i) {
            if (support_z[i] - support_z[i - 1] <= 0) is_2 = true;
        }

        bool is_3 = false; // 'no layers thicker than nozzle diameter'.
        for (int i = 1; i < support_z.size(); ++i) {
            if (support_z[i] - support_z[i - 1] > print.config.nozzle_diameter.get_at(0) + EPSILON) is_2 = true;
        }

        coordf_t expected_top_spacing =
            support.contact_distance(print.default_object_config.layer_height, print.config.nozzle_diameter.get_at(0));
        coordf_t wrong_top_spacing = 0;
        for (auto top_z_el : top_z) {
            // find layer index of this top surface.
            int layer_id = -1;
            for (int i = 0; i < support_z.size(); i++) {
                if (abs(support_z[i] - top_z_el) < EPSILON) {
                    layer_id = i;
                    i = static_cast<int>(support_z.size());
                }
            }

            // check that first support layer above this top surface (or the next one) is spaced with nozzle diameter
            if ((support_z[layer_id + 1] - support_z[layer_id]) != expected_top_spacing
                && (support_z[layer_id + 2] - support_z[layer_id]) != expected_top_spacing)
                wrong_top_spacing = 1;
        }
        bool is_4 = !wrong_top_spacing; // 'layers above top surfaces are spaced correctly'

        /* Test Also with this
               $config->set('first_layer_height', 0.4);
               $test->();

               $config->set('layer_height', $config->nozzle_diameter->[0]);
               $test->();
        */
    }

    bool test_2()
    {

    }

};
}

#endif

#ifndef slic3r_SupportMaterial_hpp_
#define slic3r_SupportMaterial_hpp_

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

    void contact_area(PrintObject *object)
    {
        PrintObjectConfig conf = this->object_config;

        // If user specified a custom angle threshold, convert it to radians.
        float threshold_rad;
        cout << conf.support_material_threshold << endl;
        if (conf.support_material_threshold > 0) {
            threshold_rad = Geometry::deg2rad(conf.support_material_threshold.value + 1.0);

            // TODO @Samir55 add debug statetments.
        }

        // Build support on a build plate only? If so, then collect top surfaces into $buildplate_only_top_surfaces
        // and subtract $buildplate_only_top_surfaces from the contact surfaces, so
        // there is no contact surface supported by a top surface.
        bool buildplate_only = (conf.support_material || conf.support_material_enforce_layers)
            && conf.support_material_buildplate_only;
        SurfacesPtr buildplate_only_top_surfaces = SurfacesPtr();

        Polygons contact;
        Polygons overhangs; // This stores the actual overhang supported by each contact layer

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
            if (layer_id > 0 && !conf.support_material && (layer_id >= conf.support_material_enforce_layers))
                // If we are only going to generate raft just check
                // the 'overhangs' of the first object layer.
                break;

            auto layer = object->get_layer(layer_id);

            if (buildplate_only) {
                // Collect the top surfaces up to this layer and merge them.
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
                    for (auto surface : projection_new) {
//                        buildplate_only_top_surfaces.push_back(offset(surface, scale_(0.01)));
                    }
                }
            }

            // Detect overhangs and contact areas needed to support them.
            if (layer_id == 0) {
                // this is the first object layer, so we're here just to get the object
                // footprint for the raft.
                // we only consider contours and discard holes to get a more continuous raft.
                for (auto const &contour : layer->slices.contours()) {
                    auto contour_clone = contour;
                    overhangs.push_back(contour_clone);
//                    contact.push_back(offset(overhangs.back(), static_cast<const float>(SUPPORT_MATERIAL_MARGIN)));
                }
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
                            d = scale_(lower_layer->height) *
                                ((cos(layer_threshold_rad)) / (sin(layer_threshold_rad)));
                        }

                        difference = diff(offset(layerm->slices, -d), lower_layer->slices);

                        // only enforce spacing from the object ($fw/2) if the threshold angle
                        // is not too high: in that case, $d will be very small (as we need to catch
                        // very short overhangs), and such contact area would be eaten by the
                        // enforced spacing, resulting in high threshold angles to be almost ignored
                        if (d > fw / 2) {
                            difference = diff(offset(difference, d - fw / 2), lower_layer->slices);
                        }
                    }
                    else {
                        // $diff now contains the ring or stripe comprised between the boundary of
                        // lower slices and the centerline of the last perimeter in this overhanging layer.
                        // Void $diff means that there's no upper perimeter whose centerline is
                        // outside the lower slice boundary, thus no overhang
                    }
                }
            }
        }

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

    void clip_with_object()
    {}

    void generate_toolpaths()
    {}

    void generate_pillars_shape()
    {}

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
        assert(support_z[0] == print.default_object_config.first_layer_height);

    }

};
}

#endif

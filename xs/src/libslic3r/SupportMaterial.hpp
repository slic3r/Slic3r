#ifndef slic3r_SupportMaterial_hpp_
#define slic3r_SupportMaterial_hpp_

#include <vector>
#include "libslic3r.h"
#include "PrintConfig.hpp"
#include "Flow.hpp"
#include "Layer.hpp"
#include "Geometry.hpp"
#include "Print.hpp"
#include "ClipperUtils.hpp"
#include "SupportMaterial.hpp"
#include "ExPolygon.hpp"

using namespace std;

#define MARGIN_STEP MARGIN/3
#define PILLAR_SIZE 2.5
#define PILLAR_SPACING 10

namespace Slic3r {

// how much we extend support around the actual contact area
constexpr coordf_t SUPPORT_MATERIAL_MARGIN = 1.5;

class Supports
{
public:
    PrintConfig *print_config; ///<
    PrintObjectConfig *print_object_config; ///<
    Flow flow; ///<
    Flow first_layer_flow; ///<
    Flow interface_flow; ///<

    Supports(PrintConfig *print_config,
             PrintObjectConfig *print_object_config,
             const Flow &flow,
             const Flow &first_layer_flow,
             const Flow &interface_flow)
        : print_config(print_config),
          print_object_config(print_object_config),
          flow(flow),
          first_layer_flow(first_layer_flow),
          interface_flow(interface_flow)
    {}

    void generate()
    {}

    void contact_area(PrintObject *object)
    {
        PrintObjectConfig conf = this->print_object_config;

        // If user specified a custom angle threshold, convert it to radians.
        float threshold_rad;
        cout << conf.support_material_threshold << endl;
        if (conf.support_material_threshold > 0) {
            threshold_rad = deg2rad(conf.support_material_threshold + 1);
            cout << "Threshold angle = %d°\n", rad2deg(threshold_rad) << endl;
        }

        // Build support on a build plate only? If so, then collect top surfaces into $buildplate_only_top_surfaces
        // and subtract $buildplate_only_top_surfaces from the contact surfaces, so
        // there is no contact surface supported by a top surface.
        bool buildplate_only = (conf.support_material || conf.support_material_enforce_layers)
            && conf.support_material_buildplate_only;
        SurfacesPtr buildplate_only_top_surfaces = SurfacesPtr();

        auto contact;
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
                        buildplate_only_top_surfaces.push_back(offset(surface, scale(0.01)));
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
                    contact.push_back(offset(overhangs.back(), static_cast<const float>(SUPPORT_MATERIAL_MARGIN)));
                }
            }
            else {
                Layer *lower_layer = object->get_layer(layer_id - 1);
                for (auto layerm : layer->regions) {
                    float fw = layerm->flow(frExternalPerimeter).scaled_width();
                    auto difference;

                    // If a threshold angle was specified, use a different logic for detecting overhangs.
                    if ((conf.support_material && threshold_rad > 0)
                        || layer_id <= conf.support_material_enforce_layers
                        || (conf.raft_layers > 0 && layer_id == 0)) {
                        float d = 0;
                        float layer_threshold_rad = threshold_rad;
                        if (layer_id <= conf.support_material_enforce_layers) {
                            // Use ~45 deg number for enforced supports if we are in auto.
                            layer_threshold_rad = deg2rad(89);
                        }
                        if (layer_threshold_rad > 0) {
                            d = scale(lower_layer->height) *
                                ((cos(layer_threshold_rad)) / (sin(layer_threshold_rad)));
                        }

                        difference = diff(offset(layerm->slices, -d), lower_layer->slices);

                        // only enforce spacing from the object ($fw/2) if the threshold angle
                        // is not too high: in that case, $d will be very small (as we need to catch
                        // very short overhangs), and such contact area would be eaten by the
                        // enforced spacing, resulting in high threshold angles to be almost ignored
                        if (d > fw/2) {
                            difference = diff(offset(difference, d - fw/2), lower_layer->slices);
                        }
                    } else {
                        // $diff now contains the ring or stripe comprised between the boundary of
                        // lower slices and the centerline of the last perimeter in this overhanging layer.
                        // Void $diff means that there's no upper perimeter whose centerline is
                        // outside the lower slice boundary, thus no overhang
                    }
                }
            }
        }

    }

    void object_top()
    {}

    void support_layers_z()
    {}

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

    void clip_with_shape()
    {
        //TODO
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

    float contact_distance(float layer_height, float nozzle_diameter)
    {
        float extra = static_cast<float>(print_object_config->support_material_contact_distance.value);
        if (extra == 0) {
            return layer_height;
        }
        else {
            return nozzle_diameter + extra;
        }
    }

};

}

#endif

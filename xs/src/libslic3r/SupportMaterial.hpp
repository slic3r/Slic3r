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
          interface_flow(interface_flow),
          object(nullptr)
    {}

    void generate(PrintObject *object);

    void generate_interface_layers()
    {}

    void generate_bottom_interface_layers()
    {}

    void generate_base_layers()
    {}

    void generate_pillars_shape()
    {}

    pair<map<coordf_t, Polygons>, map<coordf_t, Polygons>> contact_area(PrintObject *object);

    void generate_toolpaths(PrintObject *object,
                            map<coordf_t, Polygons> overhang,
                            map<coordf_t, Polygons> contact,
                            map<int, Polygons> interface,
                            map<int, Polygons> base);

    // Is this expolygons or polygons?
    map<coordf_t, Polygons> object_top(PrintObject *object, map<coordf_t, Polygons> *contact);

    // This method removes object silhouette from support material
    // (it's used with interface and base only). It removes a bit more,
    // leaving a thin gap between object and support in the XY plane.
    void clip_with_object(map<int, Polygons> &support, vector<coordf_t> support_z, PrintObject &object);

    void clip_with_shape(map<int, Polygons> &support, map<int, Polygons> &shape);

    /// This method returns the indices of the layers overlapping with the given one.
    vector<int> overlapping_layers(int layer_idx, vector<float> support_z);

    vector<coordf_t> support_layers_z(vector<coordf_t> contact_z,
                                      vector<coordf_t> top_z,
                                      coordf_t max_object_layer_height);

    coordf_t contact_distance(coordf_t layer_height, coordf_t nozzle_diameter);

    Polygons p(SurfacesPtr &surfaces);

    Polygon create_circle(coordf_t radius);

    void append_polygons(Polygons &dst, Polygons &src);

    vector<coordf_t> get_keys_sorted(map<coordf_t, Polygons> _map);

    coordf_t get_max_layer_height(PrintObject *object);

    void process_layer(int layer_id)
    {
        SupportLayer *layer = this->object->support_layers[layer_id];
        coordf_t z = layer->print_z;

        // We redefine flows locally by applyinh this layer's height.
        auto _flow = *flow;
        auto _interface_flow = *interface_flow;
        _flow.height = static_cast<float>(layer->height);
        _interface_flow.height = static_cast<float>(layer->height);

        Polygons overhang = this->overhang.count(z) > 0 ? this->overhang[z] : Polygons();
        Polygons contact = this->contact.count(z) > 0 ? this->contact[z] : Polygons();
        Polygons interface = interface[layer_id];
        Polygons base = base[layer_id];

        // TODO add the equivalent debug code.

        // Islands.
//        layer->support_islands.append()

    }

private:
    // Used during generate_toolpaths function.
    PrintObject *object;
    map<coordf_t, Polygons> overhang;
    map<coordf_t, Polygons> contact;
    map<int, Polygons> interface;
    map<int, Polygons> base;

};

// To Be converted to catch.
// Supports Material tests.
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

        vector<coordf_t > contact_z;
        vector<coordf_t > top_z;
        contact_z.push_back(1.9);
        top_z.push_back(1.9);

        // Add the modelObject.
        print.add_model_object(model.objects[0]);

        // Create new supports.
        SupportMaterial support = SupportMaterial(&print.config, &print.objects.front()->config);

        vector<coordf_t>
            support_z = support.support_layers_z(contact_z, top_z, print.default_object_config.layer_height);

//        bool
//            is_1 = (support_z[0] == print.default_object_config.first_layer_height); // 'first layer height is honored'.
//
//        bool is_2 = false; // 'no null or negative support layers'.
//        for (int i = 1; i < support_z.size(); ++i) {
//            if (support_z[i] - support_z[i - 1] <= 0) is_2 = true;
//        }
//
//        bool is_3 = false; // 'no layers thicker than nozzle diameter'.
//        for (int i = 1; i < support_z.size(); ++i) {
//            if (support_z[i] - support_z[i - 1] > print.config.nozzle_diameter.get_at(0) + EPSILON) is_2 = true;
//        }
//
//        coordf_t expected_top_spacing =
//            support.contact_distance(print.default_object_config.layer_height, print.config.nozzle_diameter.get_at(0));
//        coordf_t wrong_top_spacing = 0;
//        for (auto top_z_el : top_z) {
//            // find layer index of this top surface.
//            int layer_id = -1;
//            for (int i = 0; i < support_z.size(); i++) {
//                if (abs(support_z[i] - top_z_el) < EPSILON) {
//                    layer_id = i;
//                    i = static_cast<int>(support_z.size());
//                }
//            }
//
//            // check that first support layer above this top surface (or the next one) is spaced with nozzle diameter
//            if ((support_z[layer_id + 1] - support_z[layer_id]) != expected_top_spacing
//                && (support_z[layer_id + 2] - support_z[layer_id]) != expected_top_spacing)
//                wrong_top_spacing = 1;
//        }
//        bool is_4 = !wrong_top_spacing; // 'layers above top surfaces are spaced correctly'

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

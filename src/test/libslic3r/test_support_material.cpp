#include <catch.hpp>
//#include "/home/ahmedsamir/Work/SamirSlic3r/Slic3r/build/external/Catch/include/catch.hpp"

#include "libslic3r.h"
#include "TriangleMesh.hpp"
#include "Model.hpp"
#include "SupportMaterial.hpp"

using namespace std;
using namespace Slic3r;

SCENARIO("SupportMaterial: support_layers_z and contact_distance")
{
    GIVEN("A print object having one modelObject") {
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
        print.default_object_config.set_deserialize("support_material", "1");

        vector<coordf_t> contact_z = {1.9};
        vector<coordf_t> top_z = {1.1};

        WHEN("Layer height = 0.2 and, first layer height = 0.3") {

            print.default_object_config.set_deserialize("layer_height", "0.2");
            print.default_object_config.set_deserialize("first_layer_height", "0.3");

            print.add_model_object(model.objects[0]);
            print.objects.front()->_slice();

            SupportMaterial support = SupportMaterial(&print.config, &print.objects.front()->config);

            vector<coordf_t>
                support_z = support.support_layers_z(contact_z, top_z, print.default_object_config.layer_height.value);

            THEN("First layer height is honored") {
                REQUIRE((support_z[0] == print.default_object_config.first_layer_height.value));
            }

            THEN("No null or negative support layers") {
                bool check = true;
                for (size_t i = 1; i < support_z.size(); ++i)
                    if (support_z[i] - support_z[i - 1] <= 0) check = false;
                REQUIRE(check);
            }

            THEN("No layers thicker than nozzle diameter") {
                bool check = true;
                for (size_t i = 1; i < support_z.size(); ++i)
                    if (support_z[i] - support_z[i - 1] > print.config.nozzle_diameter.get_at(0) + EPSILON)
                        check = false;
                REQUIRE(check);
            }

            THEN("Layers above top surfaces are spaced correctly") {
                coordf_t expected_top_spacing = support
                    .contact_distance(print.default_object_config.layer_height, print.config.nozzle_diameter.get_at(0));

                bool wrong_top_spacing = 0;
                for (coordf_t top_z_el : top_z) {
                    // find layer index of this top surface.
                    int layer_id = -1;
                    for (int i = 0; i < support_z.size(); i++) {
                        if (abs(support_z[i] - top_z_el) < EPSILON) {
                            layer_id = i;
                            i = static_cast<int>(support_z.size());
                        }
                    }

                    // check that first support layer above this top surface (or the next one) is spaced with nozzle diameter
                    if (abs(support_z[layer_id + 1] - support_z[layer_id] - expected_top_spacing) > EPSILON
                        && abs(support_z[layer_id + 2] - support_z[layer_id] - expected_top_spacing) > EPSILON) {
                        wrong_top_spacing = 1;
                    }
                    REQUIRE(!wrong_top_spacing);
                }
            }
        }
//        /* Test Also with this
//               $config->set('first_layer_height', 0.4);
//               $test->();
//
//               $config->set('layer_height', $config->nozzle_diameter->[0]);
//               $test->();
//        */
    }
}

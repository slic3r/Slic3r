#include <catch.hpp>
#include "Model.hpp"
#include "test_data.hpp" // get access to init_print, etc

using namespace Slic3r::Test;

SCENARIO("Model construction") {
    GIVEN("A Slic3r Model") {
        auto model {Slic3r::Model()};
        auto sample_mesh {Slic3r::TriangleMesh::make_cube(20,20,20)};
        sample_mesh.repair();
        
        auto config {Slic3r::Config::new_from_defaults()};
        std::shared_ptr<Slic3r::Print> print = std::make_shared<Slic3r::Print>();
        print->apply_config(config);

        WHEN("Model object is added") {
            ModelObject* mo {model.add_object()};
            THEN("Model object list == 1") {
                REQUIRE(model.objects.size() == 1);
            }

            mo->add_volume(sample_mesh);
            THEN("Model volume list == 1") {
                REQUIRE(mo->volumes.size() == 1);
            }
            THEN("Model volume modifier is false") {
                REQUIRE(mo->volumes.front()->modifier == false);
            }
            THEN("Mesh is equivalent to input mesh.") {
                REQUIRE(sample_mesh.vertices() == mo->volumes.front()->mesh.vertices());
            }
            ModelInstance* inst {mo->add_instance()};
            inst->rotation = 0;
            inst->scaling_factor = 1.0;
            model.arrange_objects(print->config.min_object_distance());
            model.center_instances_around_point(Slic3r::Pointf(100,100));
            print->auto_assign_extruders(mo);
            print->add_model_object(mo);
            THEN("Print works?") {
                print->process();
                auto gcode {std::stringstream("")};
                print->export_gcode(gcode, true);
                REQUIRE(gcode.str().size() > 0);
            }

        }
    }

}

#include <catch.hpp>
#include <string>
#include "test_data.hpp"
#include "libslic3r.h"

using namespace Slic3r::Test;
using namespace std::literals;

SCENARIO("PrintObject: object layer heights") {
    GIVEN("20mm cube and default initial config, initial layer height of 2mm") {
        auto config {Slic3r::Config::new_from_defaults()};
        TestMesh m { TestMesh::cube_20x20x20 };
        Slic3r::Model model;

        config->set("first_layer_height", 2.0);

        WHEN("generate_object_layers() is called for 2mm layer heights and nozzle diameter of 3mm") {
            config->set("nozzle_diameter", "3");
            config->set("layer_height", 2.0);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            auto result {print->objects[0]->generate_object_layers(2.0)};
            THEN("The output vector has 10 entries") {
                REQUIRE(result.size() == 10);
            }
            AND_THEN("Each layer is approximately 2mm above the previous Z") {
                coordf_t last = 0.0;
                for (size_t i = 0; i < result.size(); i++) {
                    REQUIRE((result[i] - last) == Approx(2.0));
                    last = result[i];
                }
            }
        }
        WHEN("generate_object_layers() is called for 10mm layer heights and nozzle diameter of 11mm") {
            config->set("nozzle_diameter", "11");
            config->set("layer_height", 10.0);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            auto result {print->objects[0]->generate_object_layers(2.0)};
            THEN("The output vector has 2 entries") {
                REQUIRE(result.size() == 3);
            }
            AND_THEN("Layer 0 is at 2mm") {
                REQUIRE(result[0] == Approx(2.0));
            }
            AND_THEN("Layer 1 is at 12mm") {
                REQUIRE(result[1] == Approx(12.0));
            }
        }
        WHEN("generate_object_layers() is called for 15mm layer heights and nozzle diameter of 16mm") {
            config->set("nozzle_diameter", "16");
            config->set("layer_height", 15.0);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            auto result {print->objects[0]->generate_object_layers(2.0)};
            THEN("The output vector has 2 entries") {
                REQUIRE(result.size() == 3);
            }
            AND_THEN("Layer 0 is at 2mm") {
                REQUIRE(result[0] == Approx(2.0));
            }
            AND_THEN("Layer 1 is at 17mm") {
                REQUIRE(result[1] == Approx(17.0));
            }
        }
        WHEN("generate_object_layers() is called for 15mm layer heights and nozzle diameter of 5mm") {
            config->set("nozzle_diameter", "5");
            config->set("layer_height", 15.0);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            auto result {print->objects[0]->generate_object_layers(2.0)};
            THEN("The layer height is limited to 5mm.") {
                CHECK(result.size() == 5);
                coordf_t last = 2.0;
                for (size_t i = 1; i < result.size(); i++) {
                    REQUIRE((result[i] - last) == Approx(5.0));
                    last = result[i];
                }
            }
        }

    }
}

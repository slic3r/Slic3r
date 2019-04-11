#include <catch.hpp>
#include <string>
#include "test_data.hpp"
#include "libslic3r.h"

using namespace Slic3r::Test;
using namespace std::literals;

SCENARIO("PrintObject: object layer heights") {
    GIVEN("20mm cube and config that has a 3mm nozzle and a 2mm requested layer height") {
        auto config {Slic3r::Config::new_from_defaults()};
        TestMesh m { TestMesh::cube_20x20x20 };
        Slic3r::Model model;
        config->set("nozzle_diameter", "3");
        config->set("layer_height", 2.0);
        config->set("first_layer_height", 2.0);

        WHEN("generate_object_layers() is called with a starting layer of 2mm") {
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
    }
}

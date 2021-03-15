#include <catch.hpp>
#include <string>
#include "test_data.hpp"
#include "Log.hpp"
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

SCENARIO("PrintObject: minimum horizontal shells") {
    GIVEN("20mm cube and default initial config, initial layer height of 0.1mm") {
        auto config {Slic3r::Config::new_from_defaults()};
        TestMesh m { TestMesh::cube_20x20x20 };
        Slic3r::Model model;

        config->set("nozzle_diameter", "3");
        config->set("bottom_solid_layers", 1);
        config->set("perimeters", 1);
        config->set("first_layer_height", 0.1);
        config->set("layer_height", 0.1);
        config->set("fill_density", "0%");
        config->set("min_top_bottom_shell_thickness", 1.0);

        WHEN("min shell thickness is 1.0 with layer height of 0.1") {
            config->set("min_top_bottom_shell_thickness", 1.0);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            slic3r_log->set_level(log_t::DEBUG);
            print->objects[0]->prepare_infill();
            for (int i = 0; i < 12; i++)
                print->objects[0]->layers[i]->make_fills();
            THEN("Layers 0-9 are solid (Z < 1.0) (all fill_surfaces are solid)") {
                for (int i = 0; i < 10; i++) {
                    CHECK(print->objects[0]->layers[i]->print_z <= (i+1 * 0.1));
                    for (auto* r : print->objects[0]->layers[i]->regions) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.is_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 10 (Z > 1.0) is not solid.") {
                for (auto* r : print->objects[0]->layers[10]->regions) {
                    bool all_solid = true;
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.is_solid());
                    }
                }
            }
        }
        WHEN("min shell thickness is 1.22 with layer height of 0.1") {
            config->set("min_top_bottom_shell_thickness", 1.22);
            config->set("layer_height", 0.1);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            slic3r_log->set_level(log_t::DEBUG);
            print->objects[0]->prepare_infill();
            for (int i = 0; i < 20; i++)
                print->objects[0]->layers[i]->make_fills();
            AND_THEN("Layers 0-12 are solid (bottom of layer >= 1.22) (all fill_surfaces are solid)") {
                for (int i = 0; i < 13; i++) {
                    CHECK(print->objects[0]->layers[i]->print_z <= (i+1 * 0.1));
                    for (auto* r : print->objects[0]->layers[i]->regions) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.is_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 13 (Z > 1.0) is not solid.") {
                for (auto* r : print->objects[0]->layers[13]->regions) {
                    bool all_solid = true;
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.is_solid());
                    }
                }
            }
        }
        WHEN("min shell thickness is 1.22 14 bottom layers") {
            config->set("min_top_bottom_shell_thickness", 1.22);
            config->set("bottom_solid_layers", 14);
            config->set("layer_height", 0.1);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            slic3r_log->set_level(log_t::DEBUG);
            print->objects[0]->prepare_infill();
            for (int i = 0; i < 20; i++)
                print->objects[0]->layers[i]->make_fills();
            AND_THEN("Layers 0-13 are solid (bottom of layer >= 1.22) (all fill_surfaces are solid)") {
                for (int i = 0; i < 14; i++) {
                    CHECK(print->objects[0]->layers[i]->print_z <= (i+1 * 0.1));
                    for (auto* r : print->objects[0]->layers[i]->regions) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.is_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 14 is not solid.") {
                for (auto* r : print->objects[0]->layers[14]->regions) {
                    bool all_solid = true;
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.is_solid());
                    }
                }
            }
        }
    }
}

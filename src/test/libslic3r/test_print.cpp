#include <catch.hpp>
#include <string>
#include "test_data.hpp"
#include "libslic3r.h"

using namespace Slic3r::Test;
using namespace std::literals;

SCENARIO("PrintObject: Perimeter generation") {
    GIVEN("20mm cube and default config") {
        auto config {Slic3r::Config::new_from_defaults()};
        TestMesh m { TestMesh::cube_20x20x20 };
        Slic3r::Model model;
        auto event_counter {0U};
        std::string stage;
        int value {0};
        auto callback {[&event_counter, &stage, &value] (int a, const char* b) { stage = std::string(b); event_counter++; value = a; }};
        config->set("fill_density", 0);

        WHEN("make_perimeters() is called")  {
            auto print {Slic3r::Test::init_print({m}, model, config)};
            const auto& object = *(print->objects.at(0));
            print->objects[0]->make_perimeters();
            THEN("67 layers exist in the model") {
                REQUIRE(object.layers.size() == 67);
            }
            THEN("Every layer in region 0 has 1 island of perimeters") {
                for(auto* layer : object.layers) {
                    REQUIRE(layer->regions[0]->perimeters.size() == 1);
                }
            }
            THEN("Every layer in region 0 has 3 paths in its perimeters list.") {
                for(auto* layer : object.layers) {
                    REQUIRE(layer->regions[0]->perimeters.items_count() == 3);
                }
            }
        }
    }
}

SCENARIO("Print: Skirt generation") {
    GIVEN("20mm cube and default config") {
        auto config {Slic3r::Config::new_from_defaults()};
        TestMesh m { TestMesh::cube_20x20x20 };
        Slic3r::Model model;
        auto event_counter {0U};
        std::string stage;
        int value {0};
        config->set("skirt_height", 1);
        config->set("skirt_distance", 1);
        WHEN("Skirts is set to 2 loops")  {
            config->set("skirts", 2);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            print->make_skirt();
            THEN("Skirt Extrusion collection has 2 loops in it") {
                REQUIRE(print->skirt.items_count() == 2);
                REQUIRE(print->skirt.flatten().entities.size() == 2);
            }
        }
    }
}

SCENARIO("Print: Brim generation") {
    GIVEN("20mm cube and default config, 1mm first layer width") {
        auto config {Slic3r::Config::new_from_defaults()};
        TestMesh m { TestMesh::cube_20x20x20 };
        Slic3r::Model model;
        auto event_counter {0U};
        std::string stage;
        int value {0};
        config->set("first_layer_extrusion_width", 1);
        WHEN("Brim is set to 3mm")  {
            config->set("brim_width", 3);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            print->make_brim();
            THEN("Brim Extrusion collection has 3 loops in it") {
                REQUIRE(print->brim.items_count() == 3);
            }
        }
        WHEN("Brim is set to 6mm")  {
            config->set("brim_width", 6);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            print->make_brim();
            THEN("Brim Extrusion collection has 6 loops in it") {
                REQUIRE(print->brim.items_count() == 6);
            }
        }
        WHEN("Brim is set to 6mm, extrusion width 0.5mm")  {
            config->set("brim_width", 6);
            config->set("first_layer_extrusion_width", 0.5);
            auto print {Slic3r::Test::init_print({m}, model, config)};
            print->make_brim();
            THEN("Brim Extrusion collection has 12 loops in it") {
                REQUIRE(print->brim.items_count() == 12);
            }
        }
    }
}

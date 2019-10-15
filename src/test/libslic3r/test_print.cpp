
//#define CATCH_CONFIG_DISABLE

#include <catch.hpp>
#include <string>
#include "../test_data.hpp"
#include "../../libslic3r/libslic3r.h"
#include "../../libslic3r/SVG.hpp"

using namespace Slic3r::Test;
using namespace Slic3r;
using namespace std::literals;

SCENARIO("PrintObject: Perimeter generation") {
    GIVEN("20mm cube and default config & 0.3 layer height") {
        DynamicPrintConfig *config = Slic3r::DynamicPrintConfig::new_from_defaults();
        TestMesh m = TestMesh::cube_20x20x20;
        Model model{};
        config->set_key_value("fill_density", new ConfigOptionPercent(0));
        config->set_deserialize("layer_height", "0.3");

        WHEN("make_perimeters() is called")  {
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, config);
            PrintObject& object = *(print.objects().at(0));
            print.process();
            // there are 66.66666.... layers for 0.3mm in 20mm
            //slic3r is rounded (slice at half-layer), slic3rPE is less?
            //TODO: check the slic32r why it's not cut at half-layer
            THEN("67 layers exist in the model") {
                REQUIRE(object.layers().size() == 67);
            }
            THEN("Every layer in region 0 has 1 island of perimeters") {
                for(Layer* layer : object.layers()) {
                    REQUIRE(layer->regions()[0]->perimeters.entities.size() == 1);
                }
            }
            THEN("Every layer (but top) in region 0 has 3 paths in its perimeters list.") {
                LayerPtrs layers = object.layers();
                for (auto it_layer = layers.begin(); it_layer != layers.end() - 1; ++it_layer) {
                    REQUIRE((*it_layer)->regions()[0]->perimeters.items_count() == 3);
                }
            }
            THEN("Top layer in region 0 has 1 path in its perimeters list (only 1 perimeter on top).") {
                REQUIRE(object.layers().back()->regions()[0]->perimeters.items_count() == 1);
            }
        }
    }
}

SCENARIO("Print: Skirt generation") {
    GIVEN("20mm cube and default config") {
        DynamicPrintConfig *config = Slic3r::DynamicPrintConfig::new_from_defaults();
        TestMesh m = TestMesh::cube_20x20x20;
        Slic3r::Model model{};
        config->set_key_value("skirt_height", new ConfigOptionInt(1));
        config->set_key_value("skirt_distance", new ConfigOptionFloat(1));
        WHEN("Skirts is set to 2 loops")  {
            config->set_key_value("skirts", new ConfigOptionInt(2));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, config);
            print.process();
            THEN("Skirt Extrusion collection has 2 loops in it") {
                REQUIRE(print.skirt().items_count() == 2);
                REQUIRE(print.skirt().flatten().entities.size() == 2);
            }
        }
    }
}

SCENARIO("Print: Brim generation") {
    GIVEN("20mm cube and default config, 1mm first layer width") {
        DynamicPrintConfig *config = Slic3r::DynamicPrintConfig::new_from_defaults();
        TestMesh m = TestMesh::cube_20x20x20;
        Slic3r::Model model{};
        config->set_key_value("first_layer_extrusion_width", new ConfigOptionFloatOrPercent(1, false));
        WHEN("Brim is set to 3mm")  {
            config->set_key_value("brim_width", new ConfigOptionFloat(3));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, config);
            print.process();
            //{
            //    std::stringstream stri;
            //    stri << "20mm_cube_brim_test_" << ".svg";
            //    SVG svg(stri.str());
            //    svg.draw(print.brim().as_polylines(), "red");
            //    svg.Close();
            //}
            THEN("Brim Extrusion collection has 3 loops in it") {
                REQUIRE(print.brim().items_count() == 3);
            }
        }
        WHEN("Brim is set to 6mm")  {
            config->set_key_value("brim_width", new ConfigOptionFloat(6));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, config);
            print.process();
            THEN("Brim Extrusion collection has 6 loops in it") {
                REQUIRE(print.brim().items_count() == 6);
            }
        }
        WHEN("Brim is set to 6mm, extrusion width 0.5mm")  {
            config->set_key_value("brim_width", new ConfigOptionFloat(6));
            config->set_key_value("first_layer_extrusion_width", new ConfigOptionFloatOrPercent(0.5, false));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, config);
            print.process();
            double nbLoops = 6.0 / print.brim_flow().spacing();
            THEN("Brim Extrusion collection has " + std::to_string(nbLoops) + " loops in it (flow="+ std::to_string(print.brim_flow().spacing())+")") {
                REQUIRE(print.brim().items_count() == floor(nbLoops));
            }
        }
        WHEN("Brim ears activated, 3mm") {
            config->set_key_value("brim_width", new ConfigOptionFloat(3));
            config->set_key_value("brim_ears", new ConfigOptionBool(true));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, config);
            print.process();
            THEN("Brim ears Extrusion collection has 4 extrusions in it") {
                REQUIRE(print.brim().items_count() == 4);
            }
        }
    }
}

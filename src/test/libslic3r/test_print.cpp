
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
        DynamicPrintConfig &config = Slic3r::DynamicPrintConfig::full_print_config();
        TestMesh m = TestMesh::cube_20x20x20;
        Model model{};
        config.set_key_value("fill_density", new ConfigOptionPercent(0));
        config.set_deserialize("layer_height", "0.3");

        WHEN("make_perimeters() is called")  {
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, &config);
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
        DynamicPrintConfig &config = Slic3r::DynamicPrintConfig::full_print_config();
        TestMesh m = TestMesh::cube_20x20x20;
        Slic3r::Model model{};
        config.set_key_value("skirt_height", new ConfigOptionInt(1));
        config.set_key_value("skirt_distance", new ConfigOptionFloat(1));
        WHEN("Skirts is set to 2 loops")  {
            config.set_key_value("skirts", new ConfigOptionInt(2));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, &config);
            print.process();
            THEN("Skirt Extrusion collection has 2 loops in it") {
                REQUIRE(print.skirt().items_count() == 2);
                REQUIRE(print.skirt().flatten().entities.size() == 2);
            }
        }
    }
}

void test_is_solid_infill(Print &p, size_t obj_id, size_t layer_id ) {
    const PrintObject& obj { *(p.objects().at(obj_id)) };
    const Layer& layer { *(obj.get_layer(layer_id)) };

    // iterate over all of the regions in the layer
    for (const LayerRegion* reg : layer.regions()) {
        // for each region, iterate over the fill surfaces
        for (const Surface& s : reg->fill_surfaces.surfaces) {
            CHECK(s.has_fill_solid());
        }
    }
}

SCENARIO("Print: Changing number of solid surfaces does not cause all surfaces to become internal.") {
    GIVEN("sliced 20mm cube and config with top_solid_surfaces = 2 and bottom_solid_surfaces = 1") {
        DynamicPrintConfig &config = Slic3r::DynamicPrintConfig::full_print_config();
        TestMesh m { TestMesh::cube_20x20x20 };
        config.set_key_value("top_solid_layers", new ConfigOptionInt(2));
        config.set_key_value("bottom_solid_layers", new ConfigOptionInt(1));
        config.set_key_value("layer_height", new ConfigOptionFloat(0.5)); // get a known number of layers
        config.set_key_value("first_layer_height", new ConfigOptionFloatOrPercent(0.5, false));
        config.set_key_value("enforce_full_fill_volume", new ConfigOptionBool(true)); 
        Slic3r::Model model;
        auto event_counter {0U};
        std::string stage;
        Print print{};
        Slic3r::Test::init_print(print, { m }, model, &config);
        print.process();
        // Precondition: Ensure that the model has 2 solid top layers (39, 38)
        // and one solid bottom layer (0).
        test_is_solid_infill(print, 0, 0); // should be solid
        test_is_solid_infill(print, 0, 39); // should be solid
        test_is_solid_infill(print, 0, 38); // should be solid
        WHEN("Model is re-sliced with top_solid_layers == 3") {
            ((ConfigOptionInt&)(print.regions()[0]->config().top_solid_layers)).value = 3;
            print.invalidate_state_by_config_options(std::vector<Slic3r::t_config_option_key>{ "posPrepareInfill" });
            print.process();
            THEN("Print object does not have 0 solid bottom layers.") {
                test_is_solid_infill(print, 0, 0);
            }
            AND_THEN("Print object has 3 top solid layers") {
                test_is_solid_infill(print, 0, 39);
                test_is_solid_infill(print, 0, 38);
                test_is_solid_infill(print, 0, 37);
            }
        }
    }

}

SCENARIO("Print: Brim generation") {
    GIVEN("20mm cube and default config, 1mm first layer width") {
        DynamicPrintConfig &config = Slic3r::DynamicPrintConfig::full_print_config();
        TestMesh m{ TestMesh::cube_20x20x20 };
        Slic3r::Model model{};
        config.set_key_value("first_layer_extrusion_width", new ConfigOptionFloatOrPercent(1, false));
        WHEN("Brim is set to 3mm")  {
            config.set_key_value("brim_width", new ConfigOptionFloat(3));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, &config);
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
            config.set_key_value("brim_width", new ConfigOptionFloat(6));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, &config);
            print.process();
            THEN("Brim Extrusion collection has 6 loops in it") {
                REQUIRE(print.brim().items_count() == 6);
            }
        }
        WHEN("Brim is set to 6mm, extrusion width 0.5mm")  {
            config.set_key_value("brim_width", new ConfigOptionFloat(6));
            config.set_key_value("first_layer_extrusion_width", new ConfigOptionFloatOrPercent(0.5, false));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, &config);
            print.process();
            double nbLoops = 6.0 / print.brim_flow().spacing();
            THEN("Brim Extrusion collection has " + std::to_string(nbLoops) + " loops in it (flow="+ std::to_string(print.brim_flow().spacing())+")") {
                REQUIRE(print.brim().items_count() == floor(nbLoops));
            }
        }
        WHEN("Brim ears activated, 3mm") {
            config.set_key_value("brim_width", new ConfigOptionFloat(3));
            config.set_key_value("brim_ears", new ConfigOptionBool(true));
            Print print{};
            Slic3r::Test::init_print(print, { m }, model, &config);
            print.process();
            THEN("Brim ears Extrusion collection has 4 extrusions in it") {
                REQUIRE(print.brim().items_count() == 4);
            }
        }
    }
}

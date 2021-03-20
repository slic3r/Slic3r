#include <catch2/catch.hpp>

#include "libslic3r/libslic3r.h"
#include "libslic3r/Print.hpp"
#include "libslic3r/Layer.hpp"

#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;

SCENARIO("PrintObject: object layer heights", "[PrintObject]") {
    GIVEN("20mm cube and default initial config, initial layer height of 2mm") {
        WHEN("generate_object_layers() is called for 2mm layer heights and nozzle diameter of 3mm") {
            Slic3r::Print print;
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, {
        		{ "first_layer_height", 2 },
				{ "layer_height", 		2 },
	            { "nozzle_diameter", 	3 }
	        });
			const std::vector<Slic3r::Layer*> &layers = print.objects().front()->layers();
            THEN("The output vector has 10 entries") {
                REQUIRE(layers.size() == 10);
            }
            AND_THEN("Each layer is approximately 2mm above the previous Z") {
                coordf_t last = 0.0;
                for (size_t i = 0; i < layers.size(); ++ i) {
                    REQUIRE((layers[i]->print_z - last) == Approx(2.0));
                    last = layers[i]->print_z;
                }
            }
        }
        WHEN("generate_object_layers() is called for 10mm layer heights and nozzle diameter of 11mm") {
            Slic3r::Print print;
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, {
        		{ "first_layer_height", 2 },
				{ "layer_height", 		10 },
	            { "nozzle_diameter", 	11 }
	        });
			const std::vector<Slic3r::Layer*> &layers = print.objects().front()->layers();
			THEN("The output vector has 3 entries") {
                REQUIRE(layers.size() == 3);
            }
            AND_THEN("Layer 0 is at 2mm") {
                REQUIRE(layers.front()->print_z == Approx(2.0));
            }
            AND_THEN("Layer 1 is at 12mm") {
                REQUIRE(layers[1]->print_z == Approx(12.0));
            }
        }
        WHEN("generate_object_layers() is called for 15mm layer heights and nozzle diameter of 16mm") {
            Slic3r::Print print;
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, {
        		{ "first_layer_height", 2 },
				{ "layer_height", 		15 },
	            { "nozzle_diameter", 	16 }
	        });
			const std::vector<Slic3r::Layer*> &layers = print.objects().front()->layers();
			THEN("The output vector has 2 entries") {
                REQUIRE(layers.size() == 2);
            }
            AND_THEN("Layer 0 is at 2mm") {
                REQUIRE(layers[0]->print_z == Approx(2.0));
            }
            AND_THEN("Layer 1 is at 17mm") {
                REQUIRE(layers[1]->print_z == Approx(17.0));
            }
        }
#if 0
        WHEN("generate_object_layers() is called for 15mm layer heights and nozzle diameter of 5mm") {
            Slic3r::Print print;
            Slic3r::Test::init_and_process_print({TestMesh::cube_20x20x20}, print, {
        		{ "first_layer_height", 2 },
				{ "layer_height", 		15 },
	            { "nozzle_diameter", 	5 }
	        });
			const std::vector<Slic3r::Layer*> &layers = print.objects().front()->layers();
			THEN("The layer height is limited to 5mm.") {
                CHECK(layers.size() == 5);
                coordf_t last = 2.0;
                for (size_t i = 1; i < layers.size(); i++) {
                    REQUIRE((layers[i]->print_z - last) == Approx(5.0));
                    last = layers[i]->print_z;
                }
            }
        }
#endif
    }
}

SCENARIO("PrintObject: minimum horizontal shells", "[PrintObject]") {
    GIVEN("20mm cube and default initial config, initial layer height of 0.1mm") {
        auto config {Slic3r::DynamicPrintConfig::full_print_config()};
        TestMesh m { TestMesh::cube_20x20x20 };
        Slic3r::Model model;

        config.set_deserialize({
                               {"nozzle_diameter", 3},
                               {"bottom_solid_layers", 0},
                               {"top_solid_layers", 0},
                               {"perimeters", 1},
                               {"first_layer_height", 0.1},
                               {"layer_height", 0.1},
                               {"fill_density", 0},
                               {"top_solid_min_thickness", 0.0},
                               {"bottom_solid_min_thickness", 0.0}
                               });

        WHEN("bottom_solid_min_thickness is 1.0 with layer height of 0.1") {
            Slic3r::Print print;
            config.set("bottom_solid_min_thickness", 1.0);
            Slic3r::Test::init_print({m}, print, model, config);
            print.process();
            THEN("Layers 0-9 are solid (Z < 1.0) (all fill_surfaces are solid)") {
                for (int i = 0; i < 10; i++) {
                    CHECK(print.objects().at(0)->layers().at(i)->print_z <= (i+1 * 0.1));
                    for (auto* r : print.objects().at(0)->layers().at(i)->regions()) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.has_fill_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 10 (Z > 1.0) is not solid.") {
                for (auto* r : print.objects().at(0)->layers().at(10)->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
            AND_THEN("Top layer is not solid.") {
                for (auto* r : print.objects().at(0)->layers().back()->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
        }
        WHEN("min shell thickness is 1.22 with layer height of 0.1") {
            Slic3r::Print print;
            config.set("bottom_solid_min_thickness", 1.22);
            config.set("layer_height", 0.1);
            Slic3r::Test::init_print({m}, print, model, config);
            print.process();
            AND_THEN("Layers 0-12 are solid (bottom of layer >= 1.22) (all fill_surfaces are solid)") {
                for (int i = 0; i < 13; i++) {
                    CHECK(print.objects().front()->layers().at(i)->print_z <= (i+1 * 0.1));
                    for (auto* r : print.objects().at(0)->layers().at(i)->regions()) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.has_fill_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 13 (Z > 1.0) is not solid.") {
                for (auto* r : print.objects().at(0)->layers().at(13)->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
            AND_THEN("Top layer is not solid.") {
                for (auto* r : print.objects().at(0)->layers().back()->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
        }
        WHEN("min shell thickness is 1.22 14 bottom layers") {
            config.set("bottom_solid_min_thickness", 1.22);
            config.set("bottom_solid_layers", 14);
            config.set("layer_height", 0.1);
            Slic3r::Print print;
            Slic3r::Test::init_print({m}, print, model, config);
            print.process();
            for (int i = 0; i < 20; i++)
                print.objects().at(0)->layers().at(i)->make_fills();
            AND_THEN("Layers 0-13 are solid (bottom of layer >= 1.22) (all fill_surfaces are solid)") {
                for (int i = 0; i < 14; i++) {
                    CHECK(print.objects().at(0)->layers().at(i)->print_z <= (i+1 * 0.1));
                    for (auto* r : print.objects().at(0)->layers().at(i)->regions()) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.has_fill_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 14 is not solid.") {
                for (auto* r : print.objects().at(0)->layers().at(14)->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
            AND_THEN("Top layer is not solid.") {
                for (auto* r : print.objects().at(0)->layers().back()->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
        }
        WHEN("top_solid_min_thickness is 1.0 with layer height of 0.1") {
            Slic3r::Print print;
            config.set("top_solid_min_thickness", 1.0);
            Slic3r::Test::init_print({m}, print, model, config);
            print.process();
            THEN("Top 9 Layers are solid (Z < 1.0) (all fill_surfaces are solid)") {
                for (int i = 0; i < 10; i++) {
                    CHECK(print.objects().at(0)->layers().at(i)->print_z <= (i+1 * 0.1));
                    for (auto* r : print.objects().at(0)->layers().at(i)->regions()) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.has_fill_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 10 (Z > 1.0) is not solid.") {
                for (auto* r : print.objects().at(0)->layers().at(10)->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
            AND_THEN("Top layer is not solid.") {
                for (auto* r : print.objects().at(0)->layers().back()->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
        }
        WHEN("min shell thickness is 1.22 with layer height of 0.1") {
            Slic3r::Print print;
            config.set("bottom_solid_min_thickness", 1.22);
            config.set("layer_height", 0.1);
            Slic3r::Test::init_print({m}, print, model, config);
            print.process();
            AND_THEN("Layers 0-12 are solid (bottom of layer >= 1.22) (all fill_surfaces are solid)") {
                for (int i = 0; i < 13; i++) {
                    CHECK(print.objects().front()->layers().at(i)->print_z <= (i+1 * 0.1));
                    for (auto* r : print.objects().at(0)->layers().at(i)->regions()) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.has_fill_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 13 (Z > 1.0) is not solid.") {
                for (auto* r : print.objects().at(0)->layers().at(13)->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
            AND_THEN("Top layer is not solid.") {
                for (auto* r : print.objects().at(0)->layers().back()->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
        }
        WHEN("min shell thickness is 1.22 14 bottom layers") {
            config.set("bottom_solid_min_thickness", 1.22);
            config.set("bottom_solid_layers", 14);
            config.set("layer_height", 0.1);
            Slic3r::Print print;
            Slic3r::Test::init_print({m}, print, model, config);
            print.process();
            for (int i = 0; i < 20; i++)
                print.objects().at(0)->layers().at(i)->make_fills();
            AND_THEN("Layers 0-13 are solid (bottom of layer >= 1.22) (all fill_surfaces are solid)") {
                for (int i = 0; i < 14; i++) {
                    CHECK(print.objects().at(0)->layers().at(i)->print_z <= (i+1 * 0.1));
                    for (auto* r : print.objects().at(0)->layers().at(i)->regions()) {
                        for (auto s : r->fill_surfaces) {
                            REQUIRE(s.has_fill_solid());
                        }
                    }
                }
            }
            AND_THEN("Layer 14 is not solid.") {
                for (auto* r : print.objects().at(0)->layers().at(14)->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
            AND_THEN("Top layer is not solid.") {
                for (auto* r : print.objects().at(0)->layers().back()->regions()) {
                    for (auto s : r->fill_surfaces) {
                        REQUIRE(!s.has_fill_solid());
                    }
                }
            }
        }

    }
}

#include <catch.hpp>

#include <numeric>
#include <sstream>

#include "test_data.hpp" // get access to init_print, etc

#include "Config.hpp"
#include "Model.hpp"
#include "ConfigBase.hpp"
#include "GCodeReader.hpp"
#include "Flow.hpp"

using namespace Slic3r::Test;
using namespace Slic3r;

SCENARIO("Extrusion width specifics") {
    GIVEN("A config with a skirt, brim, some fill density, 3 perimeters, and 1 bottom solid layer and a 20mm cube mesh") {
        // this is a sharedptr
        auto config {Slic3r::Config::new_from_defaults()};
        config->set("skirts", 1);
        config->set("brim_width", 2);
        config->set("perimeters", 3);
        config->set("fill_density", 0.4);
        config->set("first_layer_height", "100%");

        WHEN("first layer width set to 2mm") {
            Slic3r::Model model;
            config->set("first_layer_extrusion_width", 1.3);
            auto print {Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config)};

            std::vector<double> E_per_mm_bottom;
            auto gcode {std::stringstream("")};
            Slic3r::Test::gcode(gcode, print);
            auto parser {Slic3r::GCodeReader()};
            const auto layer_height { config->getFloat("layer_height") };
            parser.parse_stream(gcode, [&E_per_mm_bottom, layer_height] (Slic3r::GCodeReader& self, const Slic3r::GCodeReader::GCodeLine& line) 
            { 
                if (self.Z == layer_height) { // only consider first layer
                    if (line.extruding() && line.dist_XY() > 0) {
                        E_per_mm_bottom.emplace_back(line.dist_E() / line.dist_XY());
                    }
                }
            });
            THEN(" First layer width applies to everything on first layer.") {
                bool pass = false;
                auto avg_E {std::accumulate(E_per_mm_bottom.cbegin(), E_per_mm_bottom.cend(), 0.0) / static_cast<double>(E_per_mm_bottom.size())};

                pass = std::count_if(E_per_mm_bottom.cbegin(), E_per_mm_bottom.cend(), [avg_E] (double v) { return _equiv(v, avg_E, 0.015); }) == 0;
                REQUIRE(pass == true);
                REQUIRE(E_per_mm_bottom.size() > 0); // make sure it actually passed because of extrusion
            }
            THEN(" First layer width does not apply to upper layer.") {
            }
        }
    }
}
SCENARIO(" Bridge flow specifics.", "[!mayfail]") {
    GIVEN("A default config with no cooling and a fixed bridge speed, flow ratio and an overhang mesh.") {
        WHEN("bridge_flow_ratio is set to 1.0") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 0.5") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 2.0") {
            THEN("Output flow is as expected.") {
            }
        }
    }
    GIVEN("A default config with no cooling and a fixed bridge speed, flow ratio, fixed extrusion width of 0.4mm and an overhang mesh.") {
        WHEN("bridge_flow_ratio is set to 1.0") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 0.5") {
            THEN("Output flow is as expected.") {
            }
        }
        WHEN("bridge_flow_ratio is set to 2.0") {
            THEN("Output flow is as expected.") {
            }
        }
    }
}

/// Test the expected behavior for auto-width, 
/// spacing, etc
SCENARIO("Flow: Flow math for non-bridges", "[!mayfail]") {
    GIVEN("Nozzle Diameter of 0.4, a desired width of 1mm and layer height of 0.5") {
        auto width {ConfigOptionFloat(1.0)};
        auto spacing {0.4};
        auto nozzle_diameter {0.4};
        auto bridge_flow {1.0};

        // Spacing for non-bridges is has some overlap
        THEN("External perimeter flow has spacing of <>") {
        }

        THEN("Internal perimeter flow has spacing of <>") {
        }
    }
    /// Check the min/max
    GIVEN("Nozzle Diameter of 0.4") {
        WHEN("AA") {
        }
    }

    GIVEN("Nozzle Diameter of 0.4, a desired spacing of 1mm and layer height of 0.5") {
    }
}

/// Spacing, width calculation for bridge extrusions
SCENARIO("Flow: Flow math for bridges", "[!mayfail]") {
    GIVEN("Nozzle Diameter of 0.4, a desired width of 1mm and layer height of 0.5") {
//        auto width {ConfigOptionFloat(1.0)};
        auto spacing {0.4};
        auto nozzle_diameter {0.4};
        auto bridge_flow {1.0};
    }

    GIVEN("Nozzle Diameter of 0.4, a desired spacing of 1mm and layer height of 0.5") {
    }
}

SCENARIO("Flow: Auto spacing", "[!mayfail]") {
}

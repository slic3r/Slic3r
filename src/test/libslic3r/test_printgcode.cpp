#include <catch.hpp>

#include <regex>
#include "test_data.hpp"
#include "libslic3r.h"

using namespace Slic3r::Test;

std::regex perimeters_regex("^G1 X[-0-9.]* Y[-0-9.]* E[-0-9.]* ; perimeter[ ]*$");
std::regex infill_regex("^G1 X[-0-9.]* Y[-0-9.]* E[-0-9.]* ; infill[ ]*$");
std::regex skirt_regex("^G1 X[-0-9.]* Y[-0-9.]* E[-0-9.]* ; skirt[ ]*$");

SCENARIO( "PrintGCode basic functionality", "[!mayfail]") {
    GIVEN("A default configuration and a print test object") {
        auto config {Slic3r::Config::new_from_defaults()};
        auto gcode {std::stringstream("")};

        WHEN("the output is executed with no support material") {
            config->set("first_layer_extrusion_width", 0);
            config->set("gcode_comments", true);
            Slic3r::Model model;
            auto print {Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config)};
            Slic3r::Test::gcode(gcode, print);
            auto exported {gcode.str()};
            THEN("Some text output is generated.") {
                REQUIRE(exported.size() > 0);
            }
            THEN("Exported text contains slic3r version") {
                REQUIRE(exported.find(SLIC3R_VERSION) != std::string::npos);
            }
            THEN("Exported text contains git commit id") {
                REQUIRE(exported.find("; Git Commit") != std::string::npos);
                REQUIRE(exported.find(BUILD_COMMIT) != std::string::npos);
            }
            THEN("Exported text contains extrusion statistics.") {
                REQUIRE(exported.find("; external perimeters extrusion width") != std::string::npos);
                REQUIRE(exported.find("; perimeters extrusion width") != std::string::npos);
                REQUIRE(exported.find("; infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; solid infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; top solid infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; support material extrusion width") == std::string::npos);
                REQUIRE(exported.find("; first layer extrusion width") == std::string::npos);
            }

            THEN("GCode preamble is emitted.") {
                REQUIRE(exported.find("G21 ; set units to millimeters") != std::string::npos);
            }

            THEN("Config options emitted for print config, default region config, default object config") {
                REQUIRE(exported.find("; first_layer_temperature") != std::string::npos);
                REQUIRE(exported.find("; layer_height") != std::string::npos);
                REQUIRE(exported.find("; fill_density") != std::string::npos);
            }
            THEN("Infill is emitted.") {
                std::smatch has_match;
                REQUIRE(std::regex_search(exported, has_match, infill_regex));
            }
            THEN("Perimeters are emitted.") {
                std::smatch has_match;
                REQUIRE(std::regex_search(exported, has_match, perimeters_regex));
            }
            THEN("Skirt is emitted.") {
                std::smatch has_match;
                REQUIRE(std::regex_search(exported, has_match, skirt_regex));
            }
        }
        WHEN("the output is executed with support material") {
            Slic3r::Model model;
            config->set("first_layer_extrusion_width", 0);
            config->set("support_material", true);
            config->set("raft_layers", 3);
            auto print {Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config)};
            Slic3r::Test::gcode(gcode, print);
            auto exported {gcode.str()};
            THEN("Some text output is generated.") {
                REQUIRE(exported.size() > 0);
            }
            THEN("Exported text contains extrusion statistics.") {
                REQUIRE(exported.find("; external perimeters extrusion width") != std::string::npos);
                REQUIRE(exported.find("; perimeters extrusion width") != std::string::npos);
                REQUIRE(exported.find("; infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; solid infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; top solid infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; support material extrusion width") != std::string::npos);
                REQUIRE(exported.find("; first layer extrusion width") == std::string::npos);
            }
        }
        WHEN("the output is executed with a separate first layer extrusion width") {
            Slic3r::Model model;
            config->set("first_layer_extrusion_width", 0.5);
            auto print {Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config)};
            Slic3r::Test::gcode(gcode, print);
            auto exported {gcode.str()};
            THEN("Some text output is generated.") {
                REQUIRE(exported.size() > 0);
            }
            THEN("Exported text contains extrusion statistics.") {
                REQUIRE(exported.find("; external perimeters extrusion width") != std::string::npos);
                REQUIRE(exported.find("; perimeters extrusion width") != std::string::npos);
                REQUIRE(exported.find("; infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; solid infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; top solid infill extrusion width") != std::string::npos);
                REQUIRE(exported.find("; support material extrusion width") == std::string::npos);
                REQUIRE(exported.find("; first layer extrusion width") != std::string::npos);
            }
        }
        WHEN("Cooling is enabled and the fan is disabled.") {
            config->set("cooling", true);
            config->set("disable_fan_first_layers", 5);
            Slic3r::Model model;
            auto print {Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config)};
            Slic3r::Test::gcode(gcode, print);
            auto exported {gcode.str()};
            THEN("GCode to disable fan is emitted."){
                REQUIRE(exported.find("M107") != std::string::npos);
            }
        }

        gcode.clear();
    }
}

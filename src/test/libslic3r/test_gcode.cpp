#include <catch.hpp>
#include <regex>
#include "test_data.hpp"
#include "GCodeReader.hpp"
#include "GCode.hpp"

using namespace Slic3r::Test;
using namespace Slic3r;


#include "GCode/CoolingBuffer.hpp"
#include "GCode.hpp"

SCENARIO("Cooling buffer speed factor rewrite enforces precision") {
    GIVEN("GCode line of set speed") {
        std::string gcode_string = "G1 F1000000.000";
        WHEN("40% speed factor is applied to a speed of 1000000 with 3-digit precision") {
            Slic3r::apply_speed_factor(gcode_string, (1.0f/3.0f), 30.0);
            REQUIRE_THAT(gcode_string, Catch::Equals("G1 F333333.344"));
        }
    }
}

SCENARIO( "Test of COG calculation") {
    GIVEN("A default configuration and a print test object") {
        auto config {Slic3r::Config::new_from_defaults()};
        auto gcode {std::stringstream("")};

        WHEN("the output is executed with no support material") {
            Slic3r::Model model;
            auto print {Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config)};
            print->process();
            Slic3r::Test::gcode(gcode, print);
            auto exported {gcode.str()};

            THEN("Some text output is generated.") {
                REQUIRE(exported.size() > 0);
            }

            THEN("COG values are contained in output") {
                REQUIRE(exported.find("; cog_x") != std::string::npos);
                REQUIRE(exported.find("; cog_y") != std::string::npos);
                REQUIRE(exported.find("; cog_z") != std::string::npos);
            }

            THEN("Check if COG values are correct") {

                int cog_x_start = exported.find("; cog_x = ");
                int cog_x_len = exported.substr(cog_x_start).find('\n');
                int cog_y_start = exported.find("; cog_y = ");
                int cog_y_len = exported.substr(cog_y_start).find('\n');
                int cog_z_start = exported.find("; cog_z = ");
                int cog_z_len = exported.substr(cog_z_start).find('\n');

                float val_x, val_y, val_z;
                // crop cog_x text
                val_x = std::stof(exported.substr(cog_x_start + 10, cog_x_len - 10));
                val_y = std::stof(exported.substr(cog_y_start + 10, cog_y_len - 10));
                val_z = std::stof(exported.substr(cog_z_start + 10, cog_z_len - 10));

                REQUIRE(abs(val_x-100.0) <= 0.5);
                REQUIRE(abs(val_y-100.0) <= 0.5);
                REQUIRE(abs(val_z-10.0) <= 0.5);
            }
        }

        gcode.clear();
    }
}

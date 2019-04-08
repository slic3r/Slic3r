#include <catch.hpp>


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

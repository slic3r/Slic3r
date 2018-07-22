#include <catch.hpp>
#include <sstream>

#include "test_data.hpp"

using namespace Slic3r::Test;

SCENARIO("init_print functionality") {
    GIVEN("A default config") {
        config_ptr config {Slic3r::Config::new_from_defaults()};
        std::stringstream gcode;
        WHEN("init_print is called with a single mesh.") {
            Slic3r::Model model;
            auto print = init_print({TestMesh::cube_20x20x20}, model, config, true); 
            gcode.clear();
            THEN("One mesh/printobject is in the resulting Print object.") {
                REQUIRE(print->objects.size() == 1);
            }
            THEN("print->process() doesn't crash.") {
                REQUIRE_NOTHROW(print->process());
            }
            THEN("Export gcode functions outputs text.") {
                print->process();
                print->export_gcode(gcode, true);
                REQUIRE(gcode.str().size() > 0);
            }
        }
    }
}

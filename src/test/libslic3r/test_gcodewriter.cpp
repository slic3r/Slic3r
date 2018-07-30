#include <catch.hpp>
#include <memory>

#include "GCodeWriter.hpp"
#include "test_options.hpp"

using namespace Slic3r;
using namespace std::literals::string_literals;

SCENARIO("lift() is not ignored after unlift() at large values of Z", "[!mayfail]") {
    GIVEN("A config from a file and a single extruder.")
    auto* writer {new GCodeWriter()};
    auto& config {writer->config};
    config.set_defaults();
    config.load(std::string(testfile_dir) + "test_gcodewriter/config_lift_unlift.ini"s);

    std::vector<unsigned int> extruder_ids {0};
    writer->set_extruders(extruder_ids);
    writer->set_extruder(0);

    WHEN("Z is set to 9007199254740992") {
        double trouble_Z = 9007199254740992;
        writer->travel_to_z(trouble_Z);
        AND_WHEN("GcodeWriter::Lift() is called") {
            REQUIRE(writer->lift().size() > 0);
            AND_WHEN("Z is moved post-lift to the same delta as the config Z lift") {
                REQUIRE(writer->travel_to_z(trouble_z + config.retract_lift).size() == 0);
                AND_WHEN("GCodeWriter::Lift() is called after GCodeWriter::Unlift()") {
                    REQUIRE(writer->unlift().size() > 0);
                    THEN("GCodeWriter::Lift() emits gcode.") {
                        REQUIRE(writer->lift().size() > 0);
                    }
                }
            }
        }
    }
    delete writer;
}

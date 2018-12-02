#include <catch.hpp>
#include "GCodeWriter.hpp"
#include "GCodeReader.hpp"
#include "test_options.hpp"
#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;
using namespace std::literals::string_literals;

SCENARIO( "Small perimeter gcode speeds" ) {
  auto gcode {std::stringstream("")};
  GIVEN( "A cylinder with diameter 10mm, and a config that sets 1 perimeter, no infill, external speed = 100mm/s and small perimeter speed = 15mm/s") {
    config_ptr test_config = Slic3r::Config::new_from_defaults();
    test_config->set("small_perimeter_length", 10); 
    test_config->set("external_perimeter_speed", 100);
    test_config->set("perimeters", 1);
    test_config->set("fill_density", "0%");
    test_config->set("top_solid_layers", 0);
    test_config->set("gcode_comments", true);
    test_config->set("cooling", false);
    Model test_model;
    auto print {Slic3r::Test::init_print({TriangleMesh::make_cylinder(10, 10)}, test_model, test_config)};
    WHEN( "slicing the model with small_perimeter_length = 6.5" ) {
      print->process();
      Slic3r::Test::gcode(gcode, print);
      auto exported {gcode.str()};
      THEN( "the speed chosen for the second layer's perimeter is 6000mm/minute in gcode (F6000)." ) {
        int speed = 0;
        auto reader {GCodeReader()};
        reader.apply_config(print-> config);
        reader.parse(exported, [&speed] (GCodeReader& self, const GCodeReader::GCodeLine& line) 
          {
            speed = self.F;
            // code to read speed from gcode goes here, see
            // https://github.com/slic3r/Slic3r/blob/master/xs/src/libslic3r/GCodeReader.hpp
            // for documentation of GCodeReader and GCodeLine
            // and test_printgcode.cpp for examples in how it's used.
          });
        REQUIRE(speed == 6000);
      }
    }
    // WHEN( "slicing the model with small_perimeter_length = 15" ) {
    //   test_config->set("small_perimeter_length", 15); 
    //   THEN( "the speed used for the second layer's perimeter is 900mm/minute in gcode (F900)." ) {
    //     int speed = 0;
    //     auto reader {GCodeReader()};
    //     reader.apply_config(print->config);
    //     reader.parse(exported, [&speed] (GCodeReader& self, const GCodeReader::GCodeLine& line) 
    //       {
    //         speed = self.F;
    //         // code to read speed from gcode goes here, see
    //         // https://github.com/slic3r/Slic3r/blob/master/xs/src/libslic3r/GCodeReader.hpp
    //         // for documentation of GCodeReader and GCodeLine
    //         // and test_printgcode.cpp for examples in how it's used.
    //       });
    //     REQUIRE(speed == 900);
    //   }
    // }// when
  }
} 
#include <catch.hpp>
#include "test_data.hpp" // get access to init_print, etc
#include "Config.hpp"
#include "SlicingAdaptive.hpp"
#include "GCodeReader.hpp"
#include "Model.hpp"

using namespace Slic3r::Test;
using namespace Slic3r;

// common test method to get the layer heights 
std::vector<double> get_computed_z(config_ptr config) {
    Slic3r::Model model;
    auto gcode {std::stringstream("")};
    auto print {Slic3r::Test::init_print({TestMesh::slopy_cube}, model, config)};
    Slic3r::Test::gcode(gcode, print);

    std::vector<double> z, increments;
    auto parser {Slic3r::GCodeReader()};
    parser.parse_stream(gcode, [&z, &increments] (Slic3r::GCodeReader& self, const Slic3r::GCodeReader::GCodeLine& line) 
    {
        if (line.dist_Z() > 0) {
            z.push_back(1.0*line.new_Z());
            increments.push_back(line.dist_Z());
        }
    });
    return z;

}

void horizontal_test_case(config_ptr config) {
    std::vector<double> z = get_computed_z(config);
    CHECK(z.size() > 2);
    CHECK(z[0] == Approx(config->get<ConfigOptionFloat>("first_layer_height").value + config->getFloat("z_offset")).margin(0.0001));
    CHECK(z[1] == Approx(config->get<ConfigOptionFloat>("first_layer_height").value + config->get<ConfigOptionFloats>("max_layer_height").values[0] + config->getFloat("z_offset")).margin(0.0001));

    std::cerr << "\n";
    CHECK(std::find_if(z.cbegin()+1, z.cend(), [](const double& f) -> bool { return f == Approx(10.0).margin(0.0001); }) != z.cend());
}

void height_gradation_test(config_ptr config) {
    std::vector<double> z {get_computed_z(config)};
    double gradation {1.0 / config->getFloat("z_steps_per_mm")};
    CHECK(z.size() > 0);

    // check that the layer heights match
    // because of rounding from the scaling, may not completely line up.
    // +1 is a "dirty fix" to avoid rounding issues.
    // Then suppress 1e-06 for the later accumulation (which is present because 1 is added to everything, including
    // where the rounding worked out right.
    std::for_each(z.begin(), z.end(), [gradation](double& f) { f = unscale((scale_(f)+1) % scale_(gradation)); f = (f == 1e-06 ? 0 : f);} );

    // layer z is a multiple of gradation
    CHECK(std::accumulate(z.begin(), z.end(), 0.0) == Approx(0));
}

// spline smoothing prevents exact facet matching
TEST_CASE("Adaptive Slicing: Object facet matching", "[!shouldfail]") {
    auto config = Slic3r::Config::new_from_defaults();
    // shrink current layer to fit another layer under horizontal facet
    config->set("start_gcode", "");  // to avoid dealing with the nozzle lift in start G-code
    config->set("z_offset", 0);
    config->set("adaptive_slicing", true);
    config->set("first_layer_height", 0.42893); // to catch lower slope edge
    config->set("nozzle_diameter", "0.5");
    config->set("min_layer_height", "0.1");
    config->set("max_layer_height", "0.5");

    SECTION("Shrink to match horizontal facets") {
        // slope height: 7,07107 (2.92893 to 10)
        config->set("adaptive_slicing_quality", "81%");
        horizontal_test_case(config);
    }

    SECTION("widen to match horizontal facets") {
        config->set("adaptive_slicing_quality", "10%");
        horizontal_test_case(config);
    }
}

TEST_CASE("Adaptive Slicing: Height Gradation") {
    auto config = Slic3r::Config::new_from_defaults();
    // shrink current layer to fit another layer under horizontal facet
    config->set("start_gcode", "");  // to avoid dealing with the nozzle lift in start G-code
    config->set("z_offset", 0);
    config->set("adaptive_slicing", true);
    config->set("first_layer_height", 0.42893); // to catch lower slope edge
    config->set("nozzle_diameter", "0.5");
    config->set("min_layer_height", "0.1");
    config->set("max_layer_height", "0.5");

    SECTION("height gradation") {
        config->set("adaptive_slicing_quality", "10%");
        for (double gradation : {1.0/.001, 1.0/.01, 1.0/.02, 1.0/.08}) {
            config->set("z_steps_per_mm", gradation);
            height_gradation_test(config);
        }
    }
}

TEST_CASE("Adaptive Slicing: layer_height calculation") {
    auto adaptive_slicing {SlicingAdaptive()};
    auto mesh {Slic3r::Test::mesh(TestMesh::slopy_cube)};
    adaptive_slicing.add_mesh(&mesh);
    adaptive_slicing.prepare(20);

    SECTION("max layer_height limited by extruder capabilities") {
        CHECK(adaptive_slicing.next_layer_height(1, 20, 0.1, 0.15) == Approx(0.15));
        CHECK(adaptive_slicing.next_layer_height(1, 20, 0.1, 0.4) == Approx(0.4));
        CHECK(adaptive_slicing.next_layer_height(1, 20, 0.1, 0.65) == Approx(0.65));
    }
    SECTION("min layer_height limited by extruder capabilities") {
        CHECK(adaptive_slicing.next_layer_height(4, 99, 0.1, 0.15) == Approx(0.1));
        CHECK(adaptive_slicing.next_layer_height(4, 98, 0.2, 0.4) == Approx(0.2));
        CHECK(adaptive_slicing.next_layer_height(4, 99, 0.3, 0.65) == Approx(0.3));
    }
    SECTION("correct layer_height depending on the facet normals") {
        CHECK(adaptive_slicing.next_layer_height(1, 10, 0.1, 0.5) == Approx(0.5));
        CHECK(adaptive_slicing.next_layer_height(4, 80, 0.1, 0.5) == Approx(0.1546).margin(0.005));
        CHECK(adaptive_slicing.next_layer_height(4, 50, 0.1, 0.5) == Approx(0.3352).margin(0.005));
    }

    SECTION("reducing layer_height due to higher slopy facet") {
        CHECK(adaptive_slicing.next_layer_height(2.798, 80, 0.1, 0.5) == Approx(0.1546).margin(0.00005));
    }
    SECTION("reducing layer_height to z-diff") {
        CHECK(adaptive_slicing.next_layer_height(2.6289, 85, 0.1, 0.5) == Approx(0.3).margin(0.005));
    }
}

SCENARIO("Adaptive Slicing: Edge cases") {
    auto config = Slic3r::Config::new_from_defaults();
    GIVEN("slopy_cube and adaptive slicing") {
        config->set("start_gcode", "");  // to avoid dealing with the nozzle lift in start G-code
        config->set("z_offset", 0);
        config->set("adaptive_slicing", true);
        config->set("first_layer_height", 0.42893); // to catch lower slope edge
        config->set("nozzle_diameter", "0.5");
        WHEN("min_layer_height = max_layer_height") {
            config->set("min_layer_height", "0.1");
            config->set("max_layer_height", "0.1");
            THEN("No exception is thrown.") {
                CHECK_NOTHROW(get_computed_z(config));
            }
        }

    }
}

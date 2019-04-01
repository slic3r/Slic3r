#include <catch.hpp>
#include <regex>

#include "libslic3r.h"
#include "SupportMaterial.hpp"
#include "Flow.hpp"
#include "GCodeReader.hpp"
#include "test_data.hpp"

using namespace Slic3r;
using namespace Slic3r::Test;


/// Check support layer particulars with the provided config.
void check_support_layers(config_ptr config, std::vector<double> contact_z = std::vector<double>(), std::vector<double> top_z = std::vector<double>()) {
    Slic3r::Model model;
    config->set("support_material", true);
    auto print {Slic3r::Test::init_print({TestMesh::cube_20x20x20}, model, config)};
    const double layer_height { config->getFloat("layer_height") };
    const double first_layer_height { config->get<ConfigOptionFloatOrPercent>("first_layer_height").get_abs_value(layer_height) };
    const double nozzle_diameter {config->get<ConfigOptionFloats>("nozzle_diameter").values[0]};

    auto flow { print->objects[0]->_support_material_flow(frSupportMaterial)};
    SupportMaterial support {SupportMaterial(&print->config, &print->objects[0]->config, flow, flow, flow)};
    auto support_z {support.support_layers_z(contact_z, top_z, first_layer_height )};
    auto expected_top_spacing {support.contact_distance(layer_height, nozzle_diameter)};

    // first layer height is honored
    CHECK(support_z.at(0) == Approx(first_layer_height));

    std::vector<double> support_height;
    std::transform(support_z.begin(), support_z.end()-1, support_z.begin()+1, std::back_inserter(support_height), 
            [nozzle_diameter] (double lower, double higher) -> double { return higher - lower; });

    // Verify that all support layers are not null/negative and that they are bigger than  the nozzle diameter.
    for (const auto& s_h : support_height) {
        CHECK(s_h > 0);
        CHECK(s_h < nozzle_diameter + EPSILON);
    }

    for (auto tz : top_z) {
        // get this top surface support_z
        const auto first { std::find_if(support_z.cbegin(), support_z.cend(), [tz] (const double sz) -> bool {
                    return std::abs(sz - tz) < EPSILON;
                })};
        CHECK(((*(first+1) - *first) == Approx(expected_top_spacing) || (*(first+2) - *first) == Approx(expected_top_spacing)));
    }
}

SCENARIO("Support: Layer heights and spacing.") {
    auto config {Config::new_from_defaults()};
    GIVEN("A slic3r config and a test model, initial contact [1.9], top_z [1.9]") {
        std::vector<double> contact_z {1.9};
        std::vector<double> top_z {1.1};
        WHEN("layer height is 0.2, first_layer_height is 0.3") {
            config->set("layer_height", 0.2);
            config->set("first_layer_height", 0.3);
            check_support_layers(config, contact_z, top_z);
        }
        WHEN("first_layer_height is 0.4, layer_height 0.2") {
            config->set("layer_height", 0.2);
            config->set("first_layer_height", 0.4);
            std::vector<double> contact_z {1.9};
            std::vector<double> top_z {1.1};
            check_support_layers(config, contact_z, top_z);
        }
        WHEN("first_layer_height is 0.4, layer_height is nozzle diameter") {
            auto nozzle_diameter { config->get<ConfigOptionFloats>("nozzle_diameter").values[0] };
            config->set("layer_height", nozzle_diameter);
            config->set("first_layer_height", 0.4);
            std::vector<double> top_z {1.1};
            check_support_layers(config, contact_z, top_z);
        }
    }
}

SCENARIO("Support: Raft Generation", "[!mayfail]") {
    auto config {Config::new_from_defaults()};
    Slic3r::Model model;
    auto parser {Slic3r::GCodeReader()};
    auto gcode {std::stringstream("")};
    gcode.clear();
    GIVEN("Overhang model with some raft layers, no brim") {
        config->set("raft_layers", 3);
        config->set("brim_width",  0);
        config->set("skirts", 0);
        config->set("support_material_extruder", 2);
        config->set("support_material_interface_extruder", 2);
        config->set("layer_height", 0.4);
        config->set("first_layer_height", 0.4);
        WHEN("Gcode is generated") {
            auto print {Slic3r::Test::init_print({TestMesh::overhang}, model, config)};
            REQUIRE_NOTHROW(Slic3r::Test::gcode(gcode, print));
            bool found_support = false;
            THEN("Raft is extruded with the support material extruder.") {
                int tool {-1};
                std::regex tool_regex("^T(\\d+)");
                std::smatch m;
                double raft_layers {config->getInt("raft_layers")};
                double layer_height {config->getFloat("layer_height")};
                parser.parse_stream(gcode, [&tool, &m, tool_regex, config, raft_layers, layer_height, &found_support] (Slic3r::GCodeReader& self, const Slic3r::GCodeReader::GCodeLine& line)
                        {
                            if (std::regex_match(line.cmd, m, tool_regex)) {
                                tool = std::stoi(m[1].str());
                            } else if (line.extruding()) {
                                if (self.Z <= (raft_layers * layer_height)) {
                                    CHECK(tool == config->getInt("support_material_extruder")-1);
                                    found_support = true;
                                } else {
                                    CHECK(tool != config->getInt("support_material_extruder")-1);
                                    // TODO: Test that full support is generated with raft.
                                }
                            }
                        });
                // make sure that support did get generated
                REQUIRE(found_support);
            }
        }
    }
}

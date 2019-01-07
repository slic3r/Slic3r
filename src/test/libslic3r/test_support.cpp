#include <catch.hpp>

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
    const double nozzle_diameter {config->get<ConfigOptionFloats>("nozzle_diameter").values[0]};

    auto gcode {std::stringstream("")};
    Slic3r::Test::gcode(gcode, print);
    auto flow { print->objects[0]->_support_material_flow(frSupportMaterial)};
    SupportMaterial support {SupportMaterial(&print->config, &print->objects[0]->config, flow, flow, flow)};
    auto support_z {support.support_layers_z(contact_z, top_z, config->get<ConfigOptionFloatOrPercent>("layer_height").get_abs_value(layer_height))};
    auto expected_top_spacing {support.contact_distance(layer_height, nozzle_diameter)};

    // first layer height is honored
    CHECK(support_z.at(0) == Approx(config->get<ConfigOptionFloatOrPercent>("first_layer_height").get_abs_value(layer_height)));

    std::vector<double> support_height;
    std::transform(support_z.begin(), support_z.end()-1, support_z.begin()+1, std::back_inserter(support_height), 
            [nozzle_diameter] (double lower, double higher) -> double { return higher - lower; });

    // Verify that all support layers are not null/negative and that they are bigger than  the nozzle diameter.
    for (const auto& s_h : support_height) {
        CHECK(s_h > 0);
        CHECK(s_h > nozzle_diameter + EPSILON);
    }

    for (auto tz : top_z) {
        // get this top surface support_z
        const auto first { std::find_if(support_z.cbegin(), support_z.cend(), [tz] (const double sz) -> bool {
                    return std::abs(sz - tz) < EPSILON;
                })};
        CHECK(((*(first+1) - *first) == Approx(expected_top_spacing) || (*(first+2) - *first) == Approx(expected_top_spacing)));
    }
}

SCENARIO("Support: Layer heights and spacing.", "[!mayfail]") {
    auto config {Config::new_from_defaults()};
    GIVEN("A slic3r config and a test model.") {
        WHEN("layer height is 0.2, first_layer_height is 0.3, and intital contact 1.9 and top_z 1.1") {
            config->set("layer_height", 0.2);
            config->set("first_layer_height", 0.3);
            std::vector<double> contact_z {1.9};
            std::vector<double> top_z {1.1};
            check_support_layers(config, contact_z, top_z);
        }
        WHEN("first_layer_height is 0.4, layer_height 0.2") {
            config->set("layer_height", 0.2);
            config->set("first_layer_height", 0.4);
            check_support_layers(config);
        }
        WHEN("first_layer_height is 0.4, layer_height is nozzle diameter") {
            auto nozzle_diameter { config->get<ConfigOptionFloats>("nozzle_diameter").values[0] };
            config->set("layer_height", nozzle_diameter);
            config->set("first_layer_height", 0.4);
            check_support_layers(config);
        }
    }
}
/*
{
    my $config = Slic3r::Config->new_from_defaults;
    $config->set('support_material', 1);
    my @contact_z = my @top_z = ();
    
    my $test = sub {
        my $print = Slic3r::Test::init_print('20mm_cube', config => $config);
        my $flow = $print->print->objects->[0]->support_material_flow(FLOW_ROLE_SUPPORT_MATERIAL);
        my $support = Slic3r::Print::SupportMaterial->new(
            object_config       => $print->print->objects->[0]->config,
            print_config        => $print->print->config,
            flow                => $flow,
            interface_flow      => $flow,
            first_layer_flow    => $flow,
        );
        my $support_z = $support->support_layers_z(\@contact_z, \@top_z, $config->layer_height);
        my $expected_top_spacing = $support->contact_distance($config->layer_height, $config->nozzle_diameter->[0]);
        
        is $support_z->[0], $config->first_layer_height,
            'first layer height is honored';
        is scalar(grep { $support_z->[$_]-$support_z->[$_-1] <= 0 } 1..$#$support_z), 0,
            'no null or negative support layers';
        is scalar(grep { $support_z->[$_]-$support_z->[$_-1] > $config->nozzle_diameter->[0] + epsilon } 1..$#$support_z), 0,
            'no layers thicker than nozzle diameter';
        
        my $wrong_top_spacing = 0;
        foreach my $top_z (@top_z) {

        }
        ok !$wrong_top_spacing, 'layers above top surfaces are spaced correctly';
    };
    
    $config->set('layer_height', 0.2);
    $config->set('first_layer_height', 0.3);
    @contact_z = (1.9);
    @top_z = (1.1);
    $test->();
    
    $config->set('first_layer_height', 0.4);
    $test->();
    
    $config->set('layer_height', $config->nozzle_diameter->[0]);
    $test->();
}
*/

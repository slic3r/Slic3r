
#include <catch.hpp>
#include "../test_data.hpp"
#include "../../libslic3r/libslic3r.h"

using namespace Slic3r;
using namespace Slic3r::Geometry;


SCENARIO("denser infills: ")
{


    GIVEN("center hole")
    {
        WHEN("creating the medial axis"){
            Model model{};
            Print print{};
            DynamicPrintConfig *config = Slic3r::DynamicPrintConfig::new_from_defaults();
            Slic3r::Test::init_print(print, { Slic3r::Test::TestMesh::di_5mm_center_notch }, model, config, false);
            print.process();
            PrintObject& object = *(print.objects().at(0));

            THEN("67 layers exist in the model") {
                REQUIRE(object.layers().size() == 67);
            }
            THEN("at layer 13 , there are 1 region") {
                REQUIRE(object.layers()[13]->region_count() == 2);
            }
            THEN("at layer 14 , there are 2 region") {
                REQUIRE(object.layers()[14]->region_count() == 2);
            }
            THEN("at layer 15 , there are 2 region") {
                REQUIRE(object.layers()[15]->region_count() == 2);
            }
        }
    }

}

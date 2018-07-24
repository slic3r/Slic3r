#include <catch.hpp>
#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/sizer.h"
    #include "wx/uiaction.h"
#endif // WX_PRECOMP
#include <iostream>

#include "OptionsGroup.hpp"
#include "ConfigBase.hpp"
#include "Config.hpp"

using namespace Slic3r::GUI;

SCENARIO("OptionsGroup: Construction.") {
}
SCENARIO("ConfigOptionsGroup: Factory methods") {
    GIVEN("A default Slic3r config") {
        OptionsGroup optgroup;
        WHEN("add_single_option is called for avoid_crossing_perimeters") {
            THEN("a UI_Checkbox is added to the field map") {
                REQUIRE(optgroup.get_field("avoid_crossing_perimeters") != nullptr);
            }
            THEN("a ConfigOptionBool is added to the option map") {
                REQUIRE(optgroup.get_field("avoid_crossing_perimeters") != nullptr);
            }
        }
    }
}

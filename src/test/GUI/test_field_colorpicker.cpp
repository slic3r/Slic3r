#include <catch.hpp>

#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/uiaction.h>
#include <wx/colour.h>
#include <wx/clrpicker.h>
#endif // WX_PRECOMP

#include <iostream>
#include "testableframe.h"
#include "OptionsGroup/Field.hpp"
#include "ConfigBase.hpp"
using namespace std::string_literals;

SCENARIO("UI_Color: default values from options and basic accessor methods") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    auto simple_option {ConfigOptionDef()};
    auto* default_color {new ConfigOptionString("#FFFF00")};
    auto event_count {0};
    auto changefunc {[&event_count] (const std::string& opt_id, const std::string& color) { event_count++; }};
    GIVEN("A Color Picker") {
        simple_option.default_value = default_color;
        auto test_field {Slic3r::GUI::UI_Color(wxTheApp->GetTopWindow(), simple_option)};

        test_field.on_change = changefunc;
        WHEN("Object is constructed with default_value of '#FFFF00'.") {
            THEN("get_string() returns '#FFFF00'") {
                REQUIRE(test_field.get_string() == "#FFFF00"s);
            }
            THEN("get_int() returns 0") {
                REQUIRE(test_field.get_int() == 0);
            }
        }
        WHEN("Color picker receives a color picked event") {
            event_count = 0;
            test_field.disable_change_event = false;
            auto ev {wxFocusEvent(wxEVT_COLOURPICKER_CHANGED, test_field.picker()->GetId())};
            ev.SetEventObject(test_field.picker());
            test_field.picker()->ProcessWindowEvent(ev);
            THEN("_on_change fires.") {
                REQUIRE(event_count == 1);
            }
        }
    }
}
SCENARIO( "Color string value tests") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    auto simple_option {ConfigOptionDef()};
    auto* default_color {new ConfigOptionString("#FFFFFF")};
    GIVEN("A Color Picker") {
        auto test_field {Slic3r::GUI::UI_Color(wxTheApp->GetTopWindow(), simple_option)};
        WHEN("Set_value is called with a string of '#FFFFFF'") {
            test_field.set_value("#FFFFFF");
            THEN("Internal wxColor is equal to wxWhite") {
                REQUIRE(test_field.picker()->GetColour() == wxColour(*wxWHITE));
            }
        }
        WHEN("Set_value is called with a string of '#FFAACC'") {
            test_field.set_value("#FFAACC");
            THEN("Internal wxColor is equal to wxColor(255, 170, 204)") {
                REQUIRE(test_field.picker()->GetColour() == wxColour(255, 170, 204));
            }
        }
        WHEN("Set_value is called with a string of '#3020FF'") {
            test_field.set_value("#3020FF");
            THEN("Internal wxColor is equal to wxColor(48, 32, 255)") {
                REQUIRE(test_field.picker()->GetColour() == wxColour(48,32,255));
            }
        }
        WHEN("Set_value is called with a string of '#01A06D'") {
            test_field.set_value("#01A06D");
            THEN("Internal wxColor is equal to wxColor(01, 160, 109)") {
                REQUIRE(test_field.picker()->GetColour() == wxColour(1,160,109));
            }
        }
        WHEN("Internal color is set to wxWHITE") {
            test_field.picker()->SetColour(wxColour(*wxWHITE));
            THEN("String value is #FFFFFF") {
                REQUIRE(test_field.get_string() == "#FFFFFF"s);
            }
        }
        WHEN("Internal color is set to wxRED") {
            test_field.picker()->SetColour(wxColour(*wxRED));
            THEN("String value is #FF0000") {
                REQUIRE(test_field.get_string() == "#FF0000"s);
            }
        }
        WHEN("Internal color is set to wxGREEN") {
            test_field.picker()->SetColour(wxColour(*wxGREEN));
            THEN("String value is #00FF00") {
                REQUIRE(test_field.get_string() == "#00FF00"s);
            }
        }
        WHEN("Internal color is set to wxBLUE") {
            test_field.picker()->SetColour(wxColour(*wxBLUE));
            THEN("String value is #0000FF") {
                REQUIRE(test_field.get_string() == "#0000FF"s);
            }
        }
    }
    delete default_color;
}


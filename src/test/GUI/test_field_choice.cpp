#include <catch.hpp>

#ifndef WX_PRECOMP
#include "wx/app.h"
#include "wx/uiaction.h"
#endif // WX_PRECOMP

#include <iostream>
#include "testableframe.h"
#include "OptionsGroup/Field.hpp"
#include "ConfigBase.hpp"

using namespace std::string_literals;

SCENARIO( "UI_Choice: default values from options") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);

    GIVEN( "I have a UI Choice with 3 options from ConfigOptionDef and a default_value that is not in the enumeration.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_string {new ConfigOptionString("A")};

        simple_option.default_value = default_string; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("B");
        simple_option.enum_values.push_back("C");
        simple_option.enum_values.push_back("D");

        auto test_field {Slic3r::GUI::UI_Choice(wxTheApp->GetTopWindow(), simple_option)};
        WHEN( "I don't explicitly select any option in the drop-down") {
            THEN( "get_value() returns the the value associuted with the listed default option in the related ConfigOptionDef") {
                REQUIRE(simple_option.default_value->getString() == test_field.get_string());
            }
        }
        WHEN( "I select the first option in the drop-down") {
            test_field.choice()->SetSelection(0);
            THEN( "get_value() returns the the value associuted with the first option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[0] == test_field.get_string());
            }
        }
        WHEN( "I select the second option in the drop-down") {
            test_field.choice()->SetSelection(1);
            THEN( "get_value() returns the the value associuted with the second option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[1] == test_field.get_string());
            }
        }
        WHEN( "I select the third option in the drop-down") {
            test_field.choice()->SetSelection(2);
            THEN( "get_value() returns the the value associuted with the third option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[2] == test_field.get_string());
            }
        }
    }
    GIVEN( "I have a UI Choice with 3 options from ConfigOptionDef and a default_value that is in the enumeration.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_string {new ConfigOptionString("B"s)};

        simple_option.default_value = default_string; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("B"s);
        simple_option.enum_values.push_back("C"s);
        simple_option.enum_values.push_back("D"s);

        auto test_field {Slic3r::GUI::UI_Choice(wxTheApp->GetTopWindow(), simple_option)};
        WHEN( "I don't explicitly select any option in the drop-down") {
            THEN( "get_value() returns the first value in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[0] == test_field.get_string());
                REQUIRE(test_field.choice()->FindString(simple_option.enum_values[0]) == 0);
            }
        }
        WHEN( "I set the string value to another item in the enumeration") {
            test_field.combo()->SetValue("C"s);
            THEN( "get_string() returns the matching item in ConfigOptionDef") {
                REQUIRE(test_field.get_string() == "C"s);
                REQUIRE(test_field.choice()->FindString(simple_option.enum_values[1]) == 1);
            }
        }
        WHEN( "I set the string value to another item that is not in the enumeration") {
            test_field.combo()->SetValue("F"s);
            THEN( "get_string() returns the matching item in ConfigOptionDef") {
                REQUIRE(test_field.get_string() == "F"s);
                REQUIRE(test_field.choice()->GetSelection() == wxNOT_FOUND);
            }
        }
    }
}


SCENARIO( "UI_Choice: event handling for on_change and on_kill_focus") {
    auto event_count {0};
    auto killfunc {[&event_count](const std::string& opt_id) { event_count += 1; }};
    auto changefunc {[&event_count](const std::string& opt_id, std::string value) { event_count += 1; }};
    GIVEN( "I have a UI Choice with 2 options from ConfigOptionDef, no default value, and an on_change handler and on_kill_focus handler.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_string {new ConfigOptionString("B")};

        simple_option.default_value = default_string; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("B");
        simple_option.enum_values.push_back("C");

        auto test_field {Slic3r::GUI::UI_Choice(wxTheApp->GetTopWindow(), simple_option)};

        test_field.on_kill_focus = killfunc;
        test_field.on_change = changefunc;

        WHEN( "I receive a wxEVT_COMBOBOX event") {
            event_count = 0;
            wxMilliSleep(250);

            auto ev {wxCommandEvent(wxEVT_COMBOBOX, test_field.choice()->GetId())};
            ev.SetEventObject(test_field.choice());
            test_field.choice()->ProcessWindowEvent(ev);
            THEN( "on_change handler is executed.") {
                REQUIRE(event_count == 1);
            }
        }
        WHEN( "I receive a wxEVT_TEXT_ENTER event") {
            event_count = 0;
            wxMilliSleep(250);

            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.choice()->GetId())};
            ev.SetEventObject(test_field.choice());
            test_field.choice()->ProcessWindowEvent(ev);
            THEN( "on_change handler is executed.") {
                REQUIRE(event_count == 1);
            }
        }
        WHEN( "My control loses focus.") {
            event_count = 0;
            test_field.choice()->SetFocus();
            wxMilliSleep(250);

            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.choice()->GetId())};
            ev.SetEventObject(test_field.choice());
            test_field.choice()->ProcessWindowEvent(ev);

            THEN( "on_change handler is executed and on_kill_focus handler is executed.") {
                REQUIRE(event_count == 2);
            }
        }
    }
}

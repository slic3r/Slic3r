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

SCENARIO( "UI_NumChoice: default values from options") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);

    GIVEN( "I have a UI NumChoice with 3 options from ConfigOptionDef that has only values and a default_value that is not in the enumeration.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_string {new ConfigOptionString("1")};

        simple_option.default_value = default_string; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("2");
        simple_option.enum_values.push_back("3");
        simple_option.enum_values.push_back("4");

        auto test_field {Slic3r::GUI::UI_NumChoice(wxTheApp->GetTopWindow(), simple_option)};
        
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN( "I don't explicitly select any option in the drop-down") {
            THEN( "get_value() returns the the value associated with the listed default option in the related ConfigOptionDef") {
                REQUIRE(test_field.get_string() == simple_option.default_value->getString());
            }
        }
        WHEN( "I select the first option in the drop-down") {
            test_field.choice()->SetSelection(0);
            THEN( "get_value() returns the the value associated with the first option in the related ConfigOptionDef") {
                REQUIRE(test_field.get_string() == simple_option.enum_values[0]);
            }
        }
        WHEN( "I select the second option in the drop-down") {
            test_field.choice()->SetSelection(1);
            THEN( "get_value() returns the the value associated with the second option in the related ConfigOptionDef") {
                REQUIRE(test_field.get_string() == simple_option.enum_values[1]);
            }
        }
        WHEN( "I select the third option in the drop-down") {
            test_field.choice()->SetSelection(2);
            THEN( "get_value() returns the the value associated with the third option in the related ConfigOptionDef") {
                REQUIRE(test_field.get_string() == simple_option.enum_values[2]);
            }
        }
    }
    GIVEN( "I have a UI NumChoice with 3 options from ConfigOptionDef that has only values and a default_value that is in the enumeration.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_string {new ConfigOptionString("2"s)};

        simple_option.default_value = default_string; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("2");
        simple_option.enum_values.push_back("3");
        simple_option.enum_values.push_back("4");

        auto test_field {Slic3r::GUI::UI_NumChoice(wxTheApp->GetTopWindow(), simple_option)};
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN( "I don't explicitly select any option in the drop-down") {
            THEN( "get_value() returns the first value in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[0] == test_field.get_string());
                REQUIRE(test_field.choice()->FindString(simple_option.enum_values[0]) == 0);
            }
        }
        WHEN( "I set the string value to another item in the enumeration") {
            test_field.choice()->SetValue("3"s);
            THEN( "get_string() returns the matching item in ConfigOptionDef") {
                REQUIRE(test_field.get_string() == "3"s);
            }
            THEN( "get_int() returns the matching item in ConfigOptionDef as an integer") {
                REQUIRE(test_field.get_int() == 3);
            }
            THEN( "get_double() returns the matching item in ConfigOptionDef as a double") {
                REQUIRE(test_field.get_double() == 3.0);
            }
            THEN( "Combobox string shows up as the first enumeration value.") {
                REQUIRE(test_field.choice()->GetValue() == simple_option.enum_values[1]);
            }
        }
        WHEN( "I set the string value to another item that is not in the enumeration") {
            test_field.choice()->SetValue("7"s);
            THEN( "get_string() returns the matching item in ConfigOptionDef") {
                REQUIRE(test_field.get_string() == "7"s);
                REQUIRE(test_field.get_int() == 7);
                REQUIRE(test_field.get_double() == 7.0);
                REQUIRE(test_field.choice()->GetSelection() == wxNOT_FOUND);
            }
        }
    }

    GIVEN( "I have a UI NumChoice with 3 options from ConfigOptionDef that has values that are doubles and labels and a default_value that is not in the enumeration.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_int {new ConfigOptionFloat(1.0)};

        simple_option.default_value = default_int; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("2.2");
        simple_option.enum_values.push_back("3.3");
        simple_option.enum_values.push_back("4.4");

        simple_option.enum_labels.push_back("B");
        simple_option.enum_labels.push_back("C");
        simple_option.enum_labels.push_back("D");

        auto test_field {Slic3r::GUI::UI_NumChoice(wxTheApp->GetTopWindow(), simple_option)};
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN( "I don't explicitly select any option in the drop-down") {
            THEN( "get_value() returns the the value associated with the listed default option in the related ConfigOptionDef") {
                REQUIRE(simple_option.default_value->getString() == test_field.get_string());
            }
        }
        WHEN( "I select the first option in the drop-down") {
            test_field.choice()->SetSelection(0);
            THEN( "get_value() returns the the value associated with the first option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[0] == test_field.get_string());
            }
        }
        WHEN( "I select the second option in the drop-down") {
            test_field.choice()->SetSelection(1);
            THEN( "get_value() returns the the value associated with the second option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[1] == test_field.get_string());
            }
        }
        WHEN( "I select the third option in the drop-down") {
            test_field.choice()->SetSelection(2);
            THEN( "get_value() returns the the value associated with the third option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[2] == test_field.get_string());
            }
        }
    }

    GIVEN( "I have a UI NumChoice with 3 options from ConfigOptionDef that has values and labels and a default_value that is not in the enumeration.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_int {new ConfigOptionInt(1)};

        simple_option.default_value = default_int; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("2");
        simple_option.enum_values.push_back("3");
        simple_option.enum_values.push_back("4");

        simple_option.enum_labels.push_back("B");
        simple_option.enum_labels.push_back("C");
        simple_option.enum_labels.push_back("D");

        auto test_field {Slic3r::GUI::UI_NumChoice(wxTheApp->GetTopWindow(), simple_option)};
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN( "I don't explicitly select any option in the drop-down") {
            THEN( "get_value() returns the the value associated with the listed default option in the related ConfigOptionDef") {
                REQUIRE(simple_option.default_value->getString() == test_field.get_string());
            }
        }
        WHEN( "I select the first option in the drop-down") {
            test_field.choice()->SetSelection(0);
            THEN( "get_value() returns the the value associated with the first option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[0] == test_field.get_string());
            }
        }
        WHEN( "I select the second option in the drop-down") {
            test_field.choice()->SetSelection(1);
            THEN( "get_value() returns the the value associated with the second option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[1] == test_field.get_string());
            }
        }
        WHEN( "I select the third option in the drop-down") {
            test_field.choice()->SetSelection(2);
            THEN( "get_value() returns the the value associated with the third option in the related ConfigOptionDef") {
                REQUIRE(simple_option.enum_values[2] == test_field.get_string());
            }
        }
    }
    GIVEN( "I have a UI NumChoice with 3 options from ConfigOptionDef that has values and labels and a default_value that is in the enumeration.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_string {new ConfigOptionString("2"s)};

        simple_option.default_value = default_string; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("2");
        simple_option.enum_values.push_back("3");
        simple_option.enum_values.push_back("4");

        simple_option.enum_labels.push_back("B");
        simple_option.enum_labels.push_back("C");
        simple_option.enum_labels.push_back("D");

        auto test_field {Slic3r::GUI::UI_NumChoice(wxTheApp->GetTopWindow(), simple_option)};

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN( "I don't explicitly select any option in the drop-down") {
            THEN( "get_value functions return the first value in the related ConfigOptionDef") {
                REQUIRE(test_field.get_string() == simple_option.enum_values[0]);
            }
            THEN( "get_int() returns the matching item in ConfigOptionDef as an integer") {
                REQUIRE(test_field.get_int() == 2);
            }
            THEN( "get_double() returns the matching item in ConfigOptionDef as a double") {
                REQUIRE(test_field.get_double() == 2.0);
            }
            THEN( "choice.FindString returns the label matching the item") {
                REQUIRE(test_field.choice()->FindString(simple_option.enum_labels[0]) == 0);
            }
        }
        WHEN( "I set the string value to another item in the enumeration") {
            test_field.set_value(3);
            THEN( "get_string() returns the matching item in ConfigOptionDef") {
                REQUIRE(test_field.get_string() == "3"s);
            }
            THEN( "get_int() returns the matching item in ConfigOptionDef as an integer") {
                REQUIRE(test_field.get_int() == 3);
            }
            THEN( "get_double() returns the matching item in ConfigOptionDef as a double") {
                REQUIRE(test_field.get_double() == 3.0);
            }
            THEN( "choice.GetValue() returns the label.") {
                REQUIRE(test_field.choice()->GetValue() == "C"s); 
                REQUIRE(test_field.choice()->FindString(simple_option.enum_labels[1]) == 1);
            }
        }
        WHEN( "I set the string value to another item that is not in the enumeration") {
            test_field.set_value(7);
            THEN( "get_string() returns the entered value.") {
                REQUIRE(test_field.get_string() == "7"s);
            }
            THEN( "get_int() returns the entered value as an integer.") {
                REQUIRE(test_field.get_int() == 7);
            }
            THEN( "get_double() returns the entered value as a double.") {
                REQUIRE(test_field.get_double() == 7.0);
            }
            THEN( "Underlying selection is wxNOT_FOUND") {
                REQUIRE(test_field.choice()->GetSelection() == wxNOT_FOUND);
            }
        }
    }
}



SCENARIO( "UI_NumChoice: event handling for on_change and on_kill_focus") {
    auto event_count {0};
    auto killfunc {[&event_count](const std::string& opt_id) { event_count += 1; }};
    auto changefunc {[&event_count](const std::string& opt_id, std::string value) { event_count += 1; }};
    GIVEN( "I have a UI NumChoice with 2 options from ConfigOptionDef, no default value, and an on_change handler and on_kill_focus handler.") {
        auto  simple_option {ConfigOptionDef()};
        auto* default_string {new ConfigOptionString("2")};

        simple_option.default_value = default_string; // owned by ConfigOptionDef
        simple_option.enum_values.push_back("2");
        simple_option.enum_values.push_back("3");

        auto test_field {Slic3r::GUI::UI_NumChoice(wxTheApp->GetTopWindow(), simple_option)};

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

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

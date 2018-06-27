#include <catch.hpp>

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/sizer.h"
    #include "wx/uiaction.h"
#endif // WX_PRECOMP
#include <iostream>
#include <string>
#include "testableframe.h"
#include "OptionsGroup/Field.hpp"
#include "ConfigBase.hpp"

using namespace Slic3r::GUI;
using namespace std::string_literals;

SCENARIO( "TextCtrl initializes with default value if available.") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 

    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(250);

    Slic3r::ConfigOptionDef simple_option;
    simple_option.type = coString;
    auto* stropt { new Slic3r::ConfigOptionString("7") };
    simple_option.default_value = stropt; // no delete, it's taken care of by ConfigOptionDef
    GIVEN ( "A UI Textctrl") {
        auto test_field {Slic3r::GUI::UI_TextCtrl(wxTheApp->GetTopWindow(), simple_option)};

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        REQUIRE(test_field.get_string() == "7"s);
    }
}

SCENARIO( "Receiving a Textctrl event") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(250);
    GIVEN ( "A UI Textctrl") {
        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, std::string value) { exec_counter++; }};
        auto test_field {Slic3r::GUI::UI_TextCtrl(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};

        test_field.on_change = changefunc;

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        WHEN( "A text event occurs") {
            exec_counter = 0;
            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.textctrl()->GetId())};
            ev.SetEventObject(test_field.textctrl());
            test_field.textctrl()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
        WHEN( "A text event occurs and change event is disabled") {
            exec_counter = 0;
            test_field.disable_change_event = false;
            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.textctrl()->GetId())};
            ev.SetEventObject(test_field.textctrl());
            test_field.textctrl()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN( "on_change is not executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
    }
}

SCENARIO( "TextCtrl: Changing the text via entry works on pressing enter") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    GIVEN ( "A UI Textctrl") {
        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, std::string value) { exec_counter++; }};
        auto test_field {Slic3r::GUI::UI_TextCtrl(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};

        test_field.on_change = changefunc;

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        WHEN( "A number is entered followed by enter key") {
            exec_counter = 0;
            test_field.textctrl()->SetFocus();
            wxYield();
            wxMilliSleep(250);
            sim.Char('3');
            wxMilliSleep(250);
            sim.Char(WXK_RETURN);
            wxMilliSleep(250);
            wxYield();
            THEN( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
            THEN( "get_string returns entered value.") {
                REQUIRE(test_field.get_string() == "3"s);
            }
        }
        WHEN( "A number is entered followed by enter key and change event is disabled") {
            exec_counter = 0;
            test_field.disable_change_event = true;
            test_field.textctrl()->SetFocus();
            wxYield();
            wxMilliSleep(250);
            sim.Char('3');
            wxMilliSleep(250);
            sim.Char(WXK_RETURN);
            wxMilliSleep(250);
            wxYield();
            THEN( "on_change is not executed.") {
                REQUIRE(exec_counter == 0);
            }
            THEN( "get_string returns entered value.") {
                REQUIRE(test_field.get_string() == "3"s);
            }
        }
        WHEN( "A number is entered and focus is lost") {
            auto killfunc {[&exec_counter](const std::string& opt_id) { exec_counter += 1; }};
            test_field.on_kill_focus = killfunc;
            
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.textctrl()->GetId())};
            ev.SetEventObject(test_field.textctrl());
            
            exec_counter = 0;
            test_field.textctrl()->SetValue("3");
            test_field.textctrl()->SetFocus();
            wxYield();
            wxMilliSleep(250);
            sim.Char('7');
            wxYield();
            wxMilliSleep(250);
            test_field.textctrl()->ProcessWindowEvent(ev);
            wxMilliSleep(250);
            wxYield();
            THEN( "on_kill_focus is executed and on_change are both executed.") {
                REQUIRE(exec_counter == 2);
            }
            THEN( "get_string returns updated value.") {
                REQUIRE(test_field.get_string() == "7"s);
            }
            THEN( "get_bool returns 0.") {
                REQUIRE(test_field.get_bool() == 0);
            }
            THEN( "get_int returns 0.") {
                REQUIRE(test_field.get_int() == 0);
            }
        }


    }
}

SCENARIO( "Multiline doesn't update other than on focus change.") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);

    Slic3r::ConfigOptionDef simple_option;
    simple_option.type = coString;
    simple_option.multiline = true;
    auto* stropt { new Slic3r::ConfigOptionString("7") };
    GIVEN ( "A multiline UI Textctrl") {
        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, std::string value) { exec_counter++; }};
        auto test_field {Slic3r::GUI::UI_TextCtrl(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};

        test_field.on_change = changefunc;

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        WHEN( "text is entered and focus is lost") {
            auto killfunc {[&exec_counter](const std::string& opt_id) { exec_counter += 1; }};
            test_field.on_kill_focus = killfunc;
            
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.textctrl()->GetId())};
            ev.SetEventObject(test_field.textctrl());
            
            exec_counter = 0;
            test_field.textctrl()->SetFocus();
            wxYield();
            wxMilliSleep(250);
            sim.Char(WXK_LEFT);
            sim.Char('7');
            wxYield();
            sim.Char('7');
            wxYield();
            wxMilliSleep(250);
            test_field.textctrl()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            wxYield();
            THEN( "on_kill_focus is executed and on_change are both executed.") {
                REQUIRE(exec_counter == 2);
            }
            THEN( "get_string returns updated value.") {
                REQUIRE(test_field.get_string() == "77"s);
            }
            THEN( "get_bool returns 0.") {
                REQUIRE(test_field.get_bool() == 0);
            }
            THEN( "get_int returns 0.") {
                REQUIRE(test_field.get_int() == 0);
            }
        }
    }
}

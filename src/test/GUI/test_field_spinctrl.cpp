#include <catch.hpp>

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/sizer.h"
    #include "wx/checkbox.h"
    #include "wx/uiaction.h"
#endif // WX_PRECOMP
#include <iostream>
#include "testableframe.h"
#include "OptionsGroup/Field.hpp"
#include "ConfigBase.hpp"

using namespace Slic3r::GUI;
SCENARIO( "Spinctrl initializes with default value if available.") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 

    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(250);

    Slic3r::ConfigOptionDef simple_option;
    simple_option.type = coInt;
    auto* intopt { new Slic3r::ConfigOptionInt(7) };
    simple_option.default_value = intopt; // no delete, it's taken care of by ConfigOptionDef
    GIVEN ( "A UI Spinctrl") {
        auto test_field {Slic3r::GUI::UI_SpinCtrl(wxTheApp->GetTopWindow(), simple_option)};

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        REQUIRE(test_field.get_int() == 7);
    }
}

SCENARIO( "Receiving a Spinctrl event") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(250);
    GIVEN ( "A UI Spinctrl") {
        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, int value) { exec_counter++; }};
        auto test_field {Slic3r::GUI::UI_SpinCtrl(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};

        test_field.on_change = changefunc;

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        WHEN( "A spin event occurs") {
            exec_counter = 0;
            auto ev {wxSpinEvent(wxEVT_SPINCTRL, test_field.spinctrl()->GetId())};
            ev.SetEventObject(test_field.spinctrl());
            test_field.spinctrl()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
        WHEN( "A spin event occurs and change event is disabled") {
            exec_counter = 0;
            test_field.disable_change_event = false;
            auto ev {wxSpinEvent(wxEVT_SPINCTRL, test_field.spinctrl()->GetId())};
            ev.SetEventObject(test_field.spinctrl());
            test_field.spinctrl()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN( "on_change is not executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
    }
}

SCENARIO( "Changing the text via entry works on pressing enter") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    GIVEN ( "A UI Spinctrl") {
        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, int value) { exec_counter++; }};
        auto test_field {Slic3r::GUI::UI_SpinCtrl(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};

        test_field.on_change = changefunc;

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        WHEN( "A number is entered followed by enter key") {
            exec_counter = 0;
            test_field.spinctrl()->SetFocus();
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
            THEN( "get_int returns entered value.") {
                REQUIRE(test_field.get_int() == 3);
            }
        }
        WHEN( "A number is entered followed by enter key and change event is disabled") {
            exec_counter = 0;
            test_field.disable_change_event = true;
            test_field.spinctrl()->SetFocus();
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
            THEN( "get_int returns entered value.") {
                REQUIRE(test_field.get_int() == 3);
            }
        }
        WHEN( "A number is entered and focus is lost") {
            auto killfunc {[&exec_counter](const std::string& opt_id) { exec_counter += 1; }};
            test_field.on_kill_focus = killfunc;
            
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.spinctrl()->GetId())};
            ev.SetEventObject(test_field.spinctrl());
            
            exec_counter = 0;
            test_field.spinctrl()->SetValue(3);
            test_field.spinctrl()->SetFocus();
            wxYield();
            wxMilliSleep(250);
            sim.Char('7');
            wxYield();
            wxMilliSleep(250);
            test_field.spinctrl()->ProcessWindowEvent(ev);
            wxMilliSleep(250);
            wxYield();
            THEN( "on_kill_focus is executed and on_change are both executed.") {
                REQUIRE(exec_counter == 2);
            }
            THEN( "get_int returns updated value.") {
                REQUIRE(test_field.get_int() == 7);
            }
            THEN( "get_bool returns 0.") {
                REQUIRE(test_field.get_bool() == 0);
            }
        }
    }
}


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
using namespace Slic3r::GUI;

SCENARIO( "GUI Checkbox option items fire their on_kill_focus when focus leaves the checkbox." ) {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    GIVEN( "A checkbox field item exists on a window") {
        auto exec_counter {0};
        auto* test_field {new UI_Checkbox(wxTheApp->GetTopWindow(), wxID_ANY)};
        
        auto killfunc {[&exec_counter](const std::string& opt_id) { exec_counter += 1; }};

        test_field->on_kill_focus = killfunc;
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN ( "focus leaves the checkbox") {
            exec_counter = 0;

            test_field->check()->SetFocus();
            wxMilliSleep(500);

            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field->check()->GetId())};
            ev.SetEventObject(test_field->check());
            test_field->check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(500);
            THEN( "on_focus_kill is executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
        delete test_field;
    }
}

SCENARIO( "GUI Checkbox option items fire their on_change event when clicked and appropriate." ) {
    wxUIActionSimulator sim;
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(500);
    GIVEN( "A checkbox field item and disable_change = false") {
        auto exec_counter {0};
       
        auto changefunc {[&exec_counter](const std::string& opt_id, bool value) { exec_counter += 1; }};
        auto* test_field {new UI_Checkbox(wxTheApp->GetTopWindow(), wxID_ANY)};
        
        test_field->disable_change_event = false;
        test_field->on_change = changefunc;

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN ( "the box is checked") {
            exec_counter = 0;
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            THEN ( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
        WHEN ( "the box is unchecked") {
            exec_counter = 0;
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            THEN ( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
        delete test_field;
    }
    GIVEN( "A checkbox field item and disable_change = true") {
        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, bool value) { exec_counter++; }};
        auto* test_field {new Slic3r::GUI::UI_Checkbox(wxTheApp->GetTopWindow(), wxID_ANY)};
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        test_field->disable_change_event = true;
        test_field->on_change = changefunc;

        WHEN ( "the box is checked") {
            exec_counter = 0;
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            THEN ( "on_change is not executed.") {
                REQUIRE(exec_counter == 0);
            }
        }
        WHEN ( "the box is unchecked") {
            exec_counter = 0;
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            THEN ( "on_change is not executed.") {
                REQUIRE(exec_counter == 0);
            }
        }
        delete test_field;
    }
}

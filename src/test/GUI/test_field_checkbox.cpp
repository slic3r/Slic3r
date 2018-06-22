#include <catch.hpp>

#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/checkbox.h"
    #include "wx/uiaction.h"
#endif // WX_PRECOMP
#include <iostream>
#include "testableframe.h"
#include "OptionsGroup/Field.hpp"
using namespace Slic3r::GUI;
SCENARIO( "GUI Checkbox option items fire their on_change event when clicked and appropriate." ) {
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    GIVEN( "A checkbox field item and disable_change = false") {
        auto* exec_counter = new int(0);
        auto* test_field {new UI_Checkbox(wxTheApp->GetTopWindow(), wxID_ANY)};
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        test_field->disable_change_event = false;
        auto changefunc {[exec_counter](const std::string& opt_id, bool value) { std::cout << *exec_counter << " " ; *exec_counter += 1; std::cout << "Executing On_Action " << *exec_counter << "\n"; }};
        test_field->on_change = changefunc;

        WHEN ( "the box is checked") {
            *exec_counter = 0;
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            THEN ( "on_change is executed.") {
                REQUIRE(*exec_counter == 1);
            }
        }
        WHEN ( "the box is unchecked") {
            *exec_counter = 0;
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseMove(test_field->check()->GetScreenPosition() + wxPoint(10, 10));
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            sim.MouseClick(wxMOUSE_BTN_LEFT);
            wxYield();
            THEN ( "on_change is executed.") {
                REQUIRE(*exec_counter == 1);
            }
        }
        delete test_field;
        delete exec_counter;
    }
}

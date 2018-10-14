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

SCENARIO( "GUI Checkbox option items fire their on_kill_focus when focus leaves the checkbox." ) {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    GIVEN( "A checkbox field item exists on a window") {
        auto exec_counter {0};
        auto test_field {Slic3r::GUI::UI_Checkbox(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};
        
        auto killfunc {[&exec_counter](const std::string& opt_id) { exec_counter += 1; }};

        test_field.on_kill_focus = killfunc;
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN ( "focus leaves the checkbox") {
            exec_counter = 0;

            test_field.check()->SetFocus();
            wxMilliSleep(500);

            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.check()->GetId())};
            ev.SetEventObject(test_field.check());
            test_field.check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(500);
            THEN( "on_focus_kill is executed.") {
                REQUIRE(exec_counter == 1);
            }
        }

        WHEN ( "focus leaves the checkbox and no callback is assigned") {
            test_field.on_kill_focus = nullptr;
            exec_counter = 0;

            test_field.check()->SetFocus();
            wxMilliSleep(500);

            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.check()->GetId())};
            ev.SetEventObject(test_field.check());
            test_field.check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(500);
            THEN( "on_focus_kill doesn't try to execute nullptr") {
                REQUIRE(1 == 1);
                REQUIRE(exec_counter == 0);
            }
        }
    }
}
SCENARIO( "GUI Checkbox set_value and get_bool work as expected." ) {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(500);
    
    auto test_field {Slic3r::GUI::UI_Checkbox(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};

    GIVEN( "A checkbox field item exists on a window") {
        WHEN ( "set_value is an bool and true") {
            test_field.set_value(true);
            THEN( " Result is converted correctly.") {
                REQUIRE( test_field.get_bool() == true);
            }
        }
        WHEN ( "set_value is an bool and false") {
            test_field.set_value(false);
            THEN( " Result is converted correctly.") {
                REQUIRE( test_field.get_bool() == false);
            }
        }
        WHEN ( "set_value is a floating point number > 0") {
            test_field.set_value(true);
            try { 
                test_field.set_value(10.2);
            } catch (boost::bad_any_cast &e) {
                THEN( " Nothing happens; exception was thrown (and caught).") {
                    REQUIRE(true);
                }
            }
            THEN( " Value did not change.") {
                REQUIRE( test_field.get_bool() == true);
            }
        }
    }
}

SCENARIO( "GUI Checkbox option respond to EVT_CHECKBOX when appropriate." ) {
    wxUIActionSimulator sim;
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    auto* boolopt { new Slic3r::ConfigOptionBool(true) };

    wxMilliSleep(500);
    GIVEN( "A checkbox field item and disable_change = false") {
        auto exec_counter {0};
       
        auto changefunc {[&exec_counter](const std::string& opt_id, bool value) { exec_counter += 1; }};
        auto test_field {Slic3r::GUI::UI_Checkbox(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};
        
        test_field.disable_change_event = false;
        test_field.on_change = changefunc;

        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();

        WHEN ( "CHECKBOX event is received") {
            exec_counter = 0;
            auto ev {wxCommandEvent(wxEVT_CHECKBOX, test_field.check()->GetId())};
            ev.SetEventObject(test_field.check());
            test_field.check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN ( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
        }
    }
    GIVEN( "A checkbox field item and disable_change = true") {
        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, bool value) { exec_counter++; }};
        auto test_field {Slic3r::GUI::UI_Checkbox(wxTheApp->GetTopWindow(), Slic3r::ConfigOptionDef())};
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        test_field.disable_change_event = true;
        test_field.on_change = changefunc;

        WHEN ( "CHECKBOX event is received") {
            exec_counter = 0;
            auto ev {wxCommandEvent(wxEVT_CHECKBOX, test_field.check()->GetId())};
            ev.SetEventObject(test_field.check());
            test_field.check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN ( "on_change is not executed.") {
                REQUIRE(exec_counter == 0);
            }
        }
    }
    GIVEN( "A checkbox field item and readonly") {

        struct Slic3r::ConfigOptionDef simple_option;
        simple_option.default_value = boolopt;
        simple_option.readonly = true;

        auto exec_counter {0};
        auto changefunc {[&exec_counter] (const std::string& opt_id, bool value) { exec_counter++; }};
        auto* test_field {new Slic3r::GUI::UI_Checkbox(wxTheApp->GetTopWindow(), simple_option)};
        wxTheApp->GetTopWindow()->Show();
        wxTheApp->GetTopWindow()->Fit();
        test_field->disable_change_event = false; // don't disable :D
        test_field->on_change = changefunc;

        WHEN ( "CHECKBOX event is received") {
            exec_counter = 0;
            test_field->set_value(false);
            auto ev {wxCommandEvent(wxEVT_CHECKBOX, test_field->check()->GetId())};
            ev.SetEventObject(test_field->check());
            test_field->check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN ( "on_change is not executed.") {
                REQUIRE(exec_counter == 0);
            }
        }
        WHEN ( "the box is clicked and enabled") {
            exec_counter = 0;
            test_field->enable();
            test_field->set_value(true); 

            auto ev {wxCommandEvent(wxEVT_CHECKBOX, test_field->check()->GetId())};
            ev.SetEventObject(test_field->check());
            test_field->check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN ( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
            THEN ( "Box is checked.") {
                REQUIRE(test_field->get_bool() == true);
            }
        }
        WHEN ( "the box is clicked and toggled true") {
            exec_counter = 0;
            test_field->set_value(true); 
            test_field->toggle(true);
            auto ev {wxCommandEvent(wxEVT_CHECKBOX, test_field->check()->GetId())};
            ev.SetEventObject(test_field->check());
            test_field->check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN ( "on_change is executed.") {
                REQUIRE(exec_counter == 1);
            }
            THEN ( "Box is checked.") {
                REQUIRE(test_field->get_bool() == true);
            }
        }
        WHEN ( "the box is clicked and toggled false") {
            exec_counter = 0;
            test_field->set_value(true); 
            test_field->toggle(false);
            auto ev {wxCommandEvent(wxEVT_CHECKBOX, test_field->check()->GetId())};
            ev.SetEventObject(test_field->check());
            test_field->check()->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(250);
            THEN ( "on_change is not executed.") {
                REQUIRE(exec_counter == 0);
            }
            THEN ( "Box is checked.") {
                REQUIRE(test_field->get_bool() == true);
            }
        }
        delete test_field;
    }
}

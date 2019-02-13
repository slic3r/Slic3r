#include <catch.hpp>

#ifndef WX_PRECOMP
#include "wx/app.h"
#include "wx/sizer.h"
#include "wx/uiaction.h"
#endif // WX_PRECOMP

#include <iostream>
#include <tuple>

#include "testableframe.h"
#include "OptionsGroup/Field.hpp"
#include "ConfigBase.hpp"
#include "Point.hpp"

using namespace std::string_literals;

SCENARIO( "UI_Point: default values from options and basic accessor methods") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);

    GIVEN( "A UI point method and a X,Y coordinate (3.2, 10.2) as the default_value") {
        auto simple_option {ConfigOptionDef()};
        auto* default_point {new ConfigOptionPoint(Pointf(3.2, 10.2))};
        simple_option.default_value = default_point;
        auto test_field {Slic3r::GUI::UI_Point(wxTheApp->GetTopWindow(), simple_option)};

        THEN( "get_string() returns '3.2;10.2'.") {
            REQUIRE(test_field.get_string() == "3.2;10.2"s);
        }
        THEN( "get_point() yields a Pointf structure with x = 3.2, y = 10.2") {
            REQUIRE(test_field.get_point().x == 3.2);
            REQUIRE(test_field.get_point().y == 10.2);
        }
    }
    GIVEN( "A UI point method and a tooltip in simple_option") {
        auto simple_option {ConfigOptionDef()};
        auto* default_point {new ConfigOptionPoint(Pointf(3.2, 10.2))};
        simple_option.default_value = default_point;

        auto test_field {Slic3r::GUI::UI_Point(wxTheApp->GetTopWindow(), simple_option)};
        THEN( "Tooltip for both labels and textctrls matches simple_option") {
            REQUIRE(test_field.ctrl_x()->GetToolTipText().ToStdString() == simple_option.tooltip );
            REQUIRE(test_field.lbl_x()->GetToolTipText().ToStdString() == simple_option.tooltip );
            REQUIRE(test_field.ctrl_y()->GetToolTipText().ToStdString() == simple_option.tooltip );
            REQUIRE(test_field.lbl_y()->GetToolTipText().ToStdString() == simple_option.tooltip );
        }
        THEN( "get_point() yields a Pointf structure with x = 3.2, y = 10.2") {
            REQUIRE(test_field.get_point().x == 3.2);
            REQUIRE(test_field.get_point().y == 10.2);
        }
    }
}

SCENARIO( "UI_Point: set_value works with several types of inputs") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    GIVEN( "A UI point method with no default value.") {
        auto  simple_option {ConfigOptionDef()};
        auto test_field {Slic3r::GUI::UI_Point(wxTheApp->GetTopWindow(), simple_option)};
        WHEN( "set_value is called with a Pointf(19.0, 2.1)") {
            test_field.set_value(Pointf(19.0, 2.1));
            THEN( "get_point() returns a Pointf(19.0, 2.1)") {
                REQUIRE(test_field.get_point() == Pointf(19.0, 2.1));
            }
            THEN( "get_string() returns '19.0;2.1'") {
                REQUIRE(test_field.get_string() == "19.0;2.1"s);
            }
            THEN( "X TextCtrl contains X coordinate") {
                REQUIRE(test_field.ctrl_x()->GetValue() == wxString("19.0"s));
            }
            THEN( "Y TextCtrl contains Y coordinate") {
                REQUIRE(test_field.ctrl_y()->GetValue() == wxString("2.1"s));
            }
        }
        WHEN( "set_value is called with a Pointf3(19.0, 2.1, 0.2)") {
            test_field.set_value(Pointf3(19.0, 2.1, 0.2));
            THEN( "get_point() returns a Pointf(19.0, 2.1)") {
                REQUIRE(test_field.get_point() == Pointf(19.0, 2.1));
            }
            THEN( "get_point3() returns a Pointf3(19.0, 2.1, 0.0)") {
                REQUIRE(test_field.get_point3() == Pointf3(19.0, 2.1, 0.0));
            }
            THEN( "get_string() returns '19.0;2.1'") {
                REQUIRE(test_field.get_string() == "19.0;2.1"s);
            }
            THEN( "X TextCtrl contains X coordinate") {
                REQUIRE(test_field.ctrl_x()->GetValue() == wxString("19.0"s));
            }
            THEN( "Y TextCtrl contains Y coordinate") {
                REQUIRE(test_field.ctrl_y()->GetValue() == wxString("2.1"s));
            }
        }
        WHEN( "set_value is called with a string of the form '30.9;211.2'") {
            test_field.set_value("30.9;211.2"s);
            THEN( "get_point() returns a Pointf(30.9, 211.2)") {
                REQUIRE(test_field.get_point() == Pointf(30.9, 211.2));
            }
            THEN( "get_string() returns '30.9;211.2'") {
                REQUIRE(test_field.get_string() == "30.9;211.2"s);
            }
            THEN( "X TextCtrl contains X coordinate") {
                REQUIRE(test_field.ctrl_x()->GetValue() == wxString("30.9"s));
            }
            THEN( "Y TextCtrl contains Y coordinate") {
                REQUIRE(test_field.ctrl_y()->GetValue() == wxString("211.2"s));
            }
        }
        WHEN( "set_value is called with a wxString of the form '30.9;211.2'") {
            test_field.set_value(wxString("30.9;211.2"s));
            THEN( "get_point() returns a Pointf(30.9, 211.2)") {
                REQUIRE(test_field.get_point() == Pointf(30.9, 211.2));
            }
            THEN( "get_string() returns '30.9;211.2'") {
                REQUIRE(test_field.get_string() == "30.9;211.2"s);
            }
            THEN( "X TextCtrl contains X coordinate") {
                REQUIRE(test_field.ctrl_x()->GetValue() == wxString("30.9"s));
            }
            THEN( "Y TextCtrl contains Y coordinate") {
                REQUIRE(test_field.ctrl_y()->GetValue() == wxString("211.2"s));
            }
        }
    }
}

SCENARIO( "UI_Point: Event responses") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(250);
    GIVEN ( "A UI_Point with no default value and a registered on_change method that increments a counter.") {
        auto simple_option {ConfigOptionDef()};
        auto test_field {Slic3r::GUI::UI_Point(wxTheApp->GetTopWindow(), simple_option)};
        auto event_count {0};
        auto changefunc {[&event_count] (const std::string& opt_id, std::tuple<std::string, std::string> value) { event_count++; }};
        auto killfunc {[&event_count](const std::string& opt_id) { event_count++; }};

        test_field.on_change = changefunc;
        test_field.on_kill_focus = killfunc;

        test_field.disable_change_event = false;

        WHEN( "kill focus event is received on X") {
            event_count = 0;
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.ctrl_x()->GetId())};
            ev.SetEventObject(test_field.ctrl_x());
            test_field.ctrl_x()->ProcessWindowEvent(ev);
            THEN( "on_kill_focus is executed.") {
                REQUIRE(event_count == 2);
            }
        }
        WHEN( "kill focus event is received on Y") {
            event_count = 0;
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.ctrl_y()->GetId())};
            ev.SetEventObject(test_field.ctrl_y());
            test_field.ctrl_y()->ProcessWindowEvent(ev);
            THEN( "on_kill_focus is executed.") {
                REQUIRE(event_count == 2);
            }
        }
        WHEN( "enter key pressed event is received on X") {
            event_count = 0;
            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.ctrl_x()->GetId())};
            ev.SetEventObject(test_field.ctrl_x());
            test_field.ctrl_x()->ProcessWindowEvent(ev);
            THEN( "on_change is executed.") {
                REQUIRE(event_count == 1);
            }
        }
        WHEN( "enter key pressed event is received on Y") {
            event_count = 0;
            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.ctrl_y()->GetId())};
            ev.SetEventObject(test_field.ctrl_y());
            test_field.ctrl_y()->ProcessWindowEvent(ev);
            THEN( "on_change is executed.") {
                REQUIRE(event_count == 1);
            }
        }
    }
    GIVEN ( "A UI_Point with no default value and a registered on_change method that increments a counter only when disable_change_event = false.") {
        auto simple_option {ConfigOptionDef()};
        auto test_field {Slic3r::GUI::UI_Point(wxTheApp->GetTopWindow(), simple_option)};
        auto event_count {0};
        auto changefunc {[&event_count] (const std::string& opt_id, std::tuple<std::string, std::string> value) { event_count++; }};
        auto killfunc {[&event_count](const std::string& opt_id) { event_count += 1; }};

        test_field.on_change = changefunc;
        test_field.on_kill_focus = killfunc;
        test_field.disable_change_event = true;

        WHEN( "kill focus event is received on X") {
            event_count = 0;
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.ctrl_x()->GetId())};
            ev.SetEventObject(test_field.ctrl_x());
            test_field.ctrl_x()->ProcessWindowEvent(ev);
            THEN( "on_kill_focus is executed.") {
                REQUIRE(event_count == 1);
            }
        }
        WHEN( "kill focus event is received on Y") {
            event_count = 0;
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.ctrl_y()->GetId())};
            ev.SetEventObject(test_field.ctrl_y());
            test_field.ctrl_y()->ProcessWindowEvent(ev);
            THEN( "on_kill_focus is executed.") {
                REQUIRE(event_count == 1);
            }
        }
        WHEN( "enter key pressed event is received on X") {
            event_count = 0;
            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.ctrl_x()->GetId())};
            ev.SetEventObject(test_field.ctrl_x());
            test_field.ctrl_x()->ProcessWindowEvent(ev);
            THEN( "on_change is not executed.") {
                REQUIRE(event_count == 0);
            }
        }
        WHEN( "enter key pressed event is received on Y") {
            event_count = 0;
            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.ctrl_y()->GetId())};
            ev.SetEventObject(test_field.ctrl_y());
            test_field.ctrl_y()->ProcessWindowEvent(ev);
            THEN( "on_change is not executed.") {
                REQUIRE(event_count == 0);
            }
        }
    }
}

SCENARIO( "UI_Point: Enable/Disable") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(250);
    GIVEN ( "A UI_Point with no default value.") {
        auto simple_option {ConfigOptionDef()};
        auto test_field {Slic3r::GUI::UI_Point(wxTheApp->GetTopWindow(), simple_option)};
        WHEN( "disable() is called") {
            test_field.disable();
            THEN( "IsEnabled == False for X and Y textctrls") {
                REQUIRE(test_field.ctrl_x()->IsEnabled() == false);
                REQUIRE(test_field.ctrl_y()->IsEnabled() == false);
            }
        }
        WHEN( "enable() is called") {
            test_field.enable();
            THEN( "IsEnabled == True for X and Y textctrls") {
                REQUIRE(test_field.ctrl_x()->IsEnabled() == true);
                REQUIRE(test_field.ctrl_y()->IsEnabled() == true);
            }
        }
        WHEN( "toggle() is called with false argument") {
            test_field.toggle(false);
            THEN( "IsEnabled == False for X and Y textctrls") {
                REQUIRE(test_field.ctrl_x()->IsEnabled() == false);
                REQUIRE(test_field.ctrl_y()->IsEnabled() == false);
            }
        }
        WHEN( "toggle() is called with true argument") {
            test_field.toggle(true);
            THEN( "IsEnabled == True for X and Y textctrls") {
                REQUIRE(test_field.ctrl_x()->IsEnabled() == true);
                REQUIRE(test_field.ctrl_y()->IsEnabled() == true);
            }
        }
    }
}

SCENARIO( "UI_Point: get_sizer()") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxMilliSleep(250);
    GIVEN ( "A UI_Point with no default value.") {
        auto simple_option {ConfigOptionDef()};
        auto test_field {Slic3r::GUI::UI_Point(wxTheApp->GetTopWindow(), simple_option)};
        WHEN( "get_sizer() is called") {
            THEN( "get_sizer() returns a wxSizer that has 4 direct children in it that are Windows.") {
                REQUIRE(test_field.get_sizer()->GetItemCount() == 4);
                auto tmp {test_field.get_sizer()->GetChildren().begin()};
                REQUIRE((*tmp)->IsWindow() == true);
                tmp++;
                REQUIRE((*tmp)->IsWindow() == true);
                tmp++;
                REQUIRE((*tmp)->IsWindow() == true);
                tmp++;
                REQUIRE((*tmp)->IsWindow() == true);
            }
        }
    }
}

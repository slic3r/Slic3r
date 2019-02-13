#include <catch.hpp>
#ifndef WX_PRECOMP
#include <wx/app.h>
#include <wx/sizer.h>
#include <wx/uiaction.h>
#include <wx/slider.h>
#endif // WX_PRECOMP

#include "testableframe.h"
#include "OptionsGroup/Field.hpp"
#include "ConfigBase.hpp"

using namespace std::string_literals;

SCENARIO( "UI_Slider: Defaults, Min/max handling, accessors.") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    auto simple_option {ConfigOptionDef()};
    auto* default_color {new ConfigOptionFloat(30.0)};
    simple_option.min = 0;
    simple_option.max = 60;
    simple_option.default_value = default_color;
    GIVEN("A UI Slider with default scale") {
        auto event_count {0};
        auto changefunc {[&event_count] (const std::string& opt_id, const double& color) { event_count++; }};
        WHEN("Option min is 0") {
            simple_option.default_value = default_color;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
            THEN("default of 0 is used.") {
                REQUIRE(test_field.slider()->GetMin() == 0);
            }
        }
        WHEN("Option max is 0") {
            simple_option.max = 0;
            simple_option.default_value = default_color;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
            THEN("default of 100*10(scale) is used.") {
                REQUIRE(test_field.slider()->GetMax() == 1000);
            }
        }
        WHEN("Default value is used.") {
            simple_option.default_value = default_color;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
            THEN("Raw slider value is 30 * scale (10) = 300") {
                REQUIRE(test_field.slider()->GetValue() == 300);
            }
        }
        WHEN("set_scale is called with 25 for argument") {
            simple_option.default_value = default_color;
            simple_option.max = 100;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
            test_field.set_scale(25);
            THEN("Slider min is 0") {
                REQUIRE(test_field.slider()->GetMin() == 0);
            }
            THEN("Slider max is 100*25 = 2500") {
                REQUIRE(test_field.slider()->GetMax() == 2500);
            }
            THEN("Slider raw value is 30*25 = 750") {
                REQUIRE(test_field.slider()->GetValue() == 750);
            }
            THEN("UI_Slider get_double still reads 30") {
                REQUIRE(test_field.get_double() == Approx(30));
            }
            THEN("UI_Slider get_int reads 30") {
                REQUIRE(test_field.get_int() == 30);
            }
            THEN("Textctrl raw value still reads 30") {
                REQUIRE(test_field.textctrl()->GetValue() == "30.0"s);
            }
            test_field.on_change = changefunc;
            THEN("on_change does not fire.") {
                REQUIRE(event_count == 0);
            }
        }
    }
    GIVEN("A UI Slider with default scale") {
        WHEN("No default value is given from config.") {
            simple_option.default_value = nullptr;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
            THEN("Initial value of slider is 0.") {
                REQUIRE(test_field.get_int() == 0);
                REQUIRE(test_field.get_double() == 0);
                REQUIRE(test_field.slider()->GetValue() == 0);
                REQUIRE(test_field.textctrl()->GetValue() == "0.0");
            }
        }
        WHEN("disable() is called") {
            simple_option.default_value = default_color;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
            test_field.slider()->Enable();
            test_field.textctrl()->Enable();
            test_field.textctrl()->SetEditable(true);
            
            test_field.disable();
            THEN("Internal slider is disabled.") {
                REQUIRE(test_field.slider()->IsEnabled() == false);
            }
            THEN("Internal textctrl is disabled.") {
                REQUIRE(test_field.textctrl()->IsEnabled() == false);
                REQUIRE(test_field.textctrl()->IsEditable() == false);
            }
        }
        WHEN("enable() is called") {
            simple_option.default_value = default_color;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
            test_field.slider()->Disable();
            test_field.textctrl()->Disable();
            test_field.textctrl()->SetEditable(false);

            test_field.enable();
            THEN("Internal slider is enabled.") {
                REQUIRE(test_field.slider()->IsEnabled() == true);
            }
            THEN("Internal textctrl is enabled.") {
                REQUIRE(test_field.textctrl()->IsEnabled() == true);
                REQUIRE(test_field.textctrl()->IsEditable() == true);
            }
        }
    }

    GIVEN("A UI Slider with scale of 1") {
        WHEN("Option min is 0") {
            simple_option.default_value = default_color;
            simple_option.min = 0;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option, 1)};
            WHEN("default of 0 is used.") {
                REQUIRE(test_field.slider()->GetMin() == 0);
            }
        }
        WHEN("Option max is 0") {
            simple_option.default_value = default_color;
            simple_option.max = 0;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option, 1)};
            WHEN("default of 10 * 100 = 1000 is used.") {
                REQUIRE(test_field.slider()->GetMax() == 100);
            }
        }
        WHEN("Default value is used.") {
            simple_option.default_value = default_color;
            auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option, 1)};
            THEN("Raw slider value is 30 * scale (1)") {
                REQUIRE(test_field.slider()->GetValue() == 30);
            }
        }
    }
}

SCENARIO( "UI_Slider: Event handlers") {
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());
    wxUIActionSimulator sim;
    wxMilliSleep(500);
    auto simple_option {ConfigOptionDef()};
    auto* default_color {new ConfigOptionString("30")};
    simple_option.min = 0;
    simple_option.max = 60;
    simple_option.default_value = default_color;
    auto event_count {0};
    auto changefunc {[&event_count] (const std::string& opt_id, const double& color) { event_count++; }};
    auto killfunc {[&event_count](const std::string& opt_id) { event_count += 1; }};
    GIVEN("A UI Slider") {
        auto test_field {Slic3r::GUI::UI_Slider(wxTheApp->GetTopWindow(), simple_option)};
        test_field.on_change = changefunc;
        test_field.on_kill_focus = killfunc;
        WHEN("UI Slider receives a text change event (enter is pressed).") {
            event_count = 0;
            auto ev {wxCommandEvent(wxEVT_TEXT_ENTER, test_field.textctrl()->GetId())};
            ev.SetEventObject(test_field.textctrl());
            test_field.textctrl()->ProcessWindowEvent(ev);
            THEN("_on_change fires") {
                REQUIRE(event_count == 1);
            }
        }
        WHEN("UI Slider textbox receives a text change event (enter is not pressed).") {
            event_count = 0;
            auto ev {wxCommandEvent(wxEVT_TEXT, test_field.textctrl()->GetId())};
            ev.SetEventObject(test_field.textctrl());
            test_field.textctrl()->ProcessWindowEvent(ev);
            THEN("Nothing happens") {
                REQUIRE(event_count == 0);
            }
        }
        WHEN("UI Slider receives a slider changed event.") {
            event_count = 0;
            auto ev {wxCommandEvent(wxEVT_SLIDER, test_field.slider()->GetId())};
            ev.SetEventObject(test_field.slider());
            test_field.slider()->ProcessWindowEvent(ev);
            THEN("on_change fires") {
                REQUIRE(event_count == 1);
            }
        }
        WHEN("UI_Slider text ctrl receives a kill focus event.") {
            event_count = 0;
            auto ev {wxFocusEvent(wxEVT_KILL_FOCUS, test_field.textctrl()->GetId())};
            ev.SetEventObject(test_field.textctrl());
            test_field.textctrl()->ProcessWindowEvent(ev);
            THEN("_kill_focus and on_change fires") {
                REQUIRE(event_count == 2);
            }
        }
    }
}

#include <catch.hpp>
#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/sizer.h"
    #include "wx/uiaction.h"
#endif // WX_PRECOMP
#include <iostream>
#include "testableframe.h"

#include "Log.hpp"
#include "Print.hpp"

#include "Preset.hpp"
#include "Plater/PresetChooser.hpp"
#include "Config.hpp"
#include <test_options.hpp>

using namespace Slic3r::GUI;
using namespace std::literals::string_literals;


std::array<Presets, preset_types> defaults() {
    std::array<Presets, preset_types> default_presets;
    default_presets[get_preset(preset_t::Print)].push_back(Preset(true, "- default -", preset_t::Print));
    default_presets[get_preset(preset_t::Material)].push_back(Preset(true, "- default -", preset_t::Material));
    default_presets[get_preset(preset_t::Printer)].push_back(Preset(true, "- default -", preset_t::Printer));
    return default_presets;
}

std::array<Presets, preset_types> sample() {
    std::array<Presets, preset_types> preset_list;
    preset_list[get_preset(preset_t::Print)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "print-profile.ini", preset_t::Print));
    preset_list[get_preset(preset_t::Print)].push_back(Preset(true, "- default -", preset_t::Print));

    preset_list[get_preset(preset_t::Material)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "material-profile.ini", preset_t::Material));
    preset_list[get_preset(preset_t::Material)].push_back(Preset(true, "- default -", preset_t::Material));

    preset_list[get_preset(preset_t::Printer)].push_back(Preset(true, "- default -", preset_t::Printer));
    preset_list[get_preset(preset_t::Printer)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "printer-profile.ini", preset_t::Printer));

    return preset_list;
}

std::array<Presets, preset_types> default_compatible_reversion() {
    std::array<Presets, preset_types> preset_list;
    preset_list[get_preset(preset_t::Print)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "print-profile.ini", preset_t::Print));
    preset_list[get_preset(preset_t::Print)].push_back(Preset(true, "- default -", preset_t::Print));

    // set the material to be compatible to printer-profile-2 only
    preset_list[get_preset(preset_t::Material)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "incompat-material-profile.ini", preset_t::Material));
    preset_list[get_preset(preset_t::Material)].push_back(Preset(true, "- default -", preset_t::Material));

    preset_list[get_preset(preset_t::Printer)].push_back(Preset(true, "- default -", preset_t::Printer));
    preset_list[get_preset(preset_t::Printer)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "printer-profile-2.ini", preset_t::Printer));
    preset_list[get_preset(preset_t::Printer)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "printer-profile.ini", preset_t::Printer));

    return preset_list;
}

std::array<Presets, preset_types> compatible_reversion() {
    std::array<Presets, preset_types> preset_list;
    preset_list[get_preset(preset_t::Print)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "print-profile.ini", preset_t::Print));
    preset_list[get_preset(preset_t::Print)].push_back(Preset(true, "- default -", preset_t::Print));

    // set the material to be compatible to printer-profile-2 only
    preset_list[get_preset(preset_t::Material)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "incompat-material-profile.ini", preset_t::Material));
    preset_list[get_preset(preset_t::Material)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "material-profile.ini", preset_t::Material));
    preset_list[get_preset(preset_t::Material)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "other-material-profile.ini", preset_t::Material));
    preset_list[get_preset(preset_t::Material)].push_back(Preset(true, "- default -", preset_t::Material));

    preset_list[get_preset(preset_t::Printer)].push_back(Preset(true, "- default -", preset_t::Printer));
    preset_list[get_preset(preset_t::Printer)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "printer-profile-2.ini", preset_t::Printer));
    preset_list[get_preset(preset_t::Printer)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "printer-profile.ini", preset_t::Printer));

    return preset_list;
}

// Tests to cover behavior when the printer profile is changed.
// System should update its selected choosers based on changed print profile,
// update settings, etc.
SCENARIO( "PresetChooser changed printer") {
    std::shared_ptr<Print> fake_print {std::make_shared<Print>()};
    Settings default_settings;
    wxUIActionSimulator sim;
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow());
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());

    GIVEN( "A PresetChooser with printer-profile selected." ) {
        Settings test_settings;
        test_settings.default_presets.at(get_preset(preset_t::Printer)).push_back(wxString("printer-profile"));
        auto preset_list {default_compatible_reversion()};
        PresetChooser cut(wxTheApp->GetTopWindow(), fake_print, &test_settings, preset_list);
        cut.load();

        WHEN( "Printer profile is changed to printer-profile-2 via select_preset_by_name" ) {
            cut.select_preset_by_name("printer-profile-2", preset_t::Printer, 0);
            THEN( "Selected printer profile entry is \"printer-profile-2\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Printer)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("printer-profile-2"));
                }
            }
            THEN( "Print profile chooser has 1 entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Print)]) {
                    REQUIRE(chooser->GetCount() == 1);
                }
            }
            THEN( "Selected print profile entry is \"print-profile\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Print)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("print-profile"));
                }
            }
            THEN( "Material profile chooser has one entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->GetCount() == 1);
                }
            }
            THEN( "Selected material profile entry is \"material-profile\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("incompat-material-profile"));
                }
            }
        }
        WHEN( "Printer profile is changed to printer-profile-2 via combobox event" ) {
            auto* printer_chooser = cut.preset_choosers[get_preset(preset_t::Printer)].at(0);
            printer_chooser->SetSelection(0);

            auto ev {wxCommandEvent(wxEVT_COMBOBOX, printer_chooser->GetId())};
            ev.SetEventObject(printer_chooser);
            printer_chooser->ProcessWindowEvent(ev);
            wxYield();
            wxMilliSleep(150);

            THEN( "Selected printer profile entry is \"printer-profile-2\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Printer)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("printer-profile-2"));
                }
            }
            THEN( "Print profile chooser has 1 entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Print)]) {
                    REQUIRE(chooser->GetCount() == 1);
                }
            }
            THEN( "Selected print profile entry is \"print-profile\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Print)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("print-profile"));
                }
            }
            THEN( "Material profile chooser has one entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->GetCount() == 1);
                }
            }
            THEN( "Selected material profile entry is \"material-profile\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("incompat-material-profile"));
                }
            }
        }
    }
    GIVEN( "A PresetChooser with printer-profile selected and 2+ non-default entries for material ." ) {
        Settings test_settings;
        test_settings.default_presets.at(get_preset(preset_t::Printer)).push_back(wxString("printer-profile"));
        auto preset_list {compatible_reversion()};
        PresetChooser cut(wxTheApp->GetTopWindow(), fake_print, &test_settings, preset_list);
        cut.load();
        WHEN( "Printer profile has only 2 compatible materials" ) {
            THEN( "Material profile chooser has two entries" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->GetCount() == 2);
                }
            }
            THEN( "incompat-material-profile is not in the chooser" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->FindString("incompat-material-profile") == wxNOT_FOUND);
                }
            }
        }

        WHEN( "Printer profile is changed to printer-profile-2 via select_preset_by_name" ) {
            cut.select_preset_by_name("printer-profile-2", preset_t::Printer, 0);
            THEN( "Selected printer profile entry is \"printer-profile-2\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Printer)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("printer-profile-2"));
                }
            }
            THEN( "Print profile chooser has 1 entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Print)]) {
                    REQUIRE(chooser->GetCount() == 1);
                }
            }
            THEN( "Selected print profile entry is \"print-profile\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Print)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("print-profile"));
                }
            }
            THEN( "Material profile chooser has one entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->GetCount() == 3);
                }
            }
        }
    }
}

SCENARIO( "PresetChooser Preset loading" ) {
    std::shared_ptr<Print> fake_print {std::make_shared<Print>()};
    Settings default_settings;
    auto& settings_presets = default_settings.default_presets;
    wxUIActionSimulator sim;
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());

    GIVEN( "A PresetChooser object." ) {
        WHEN( "load() is called with only default presets" ) {
            auto preset_list {defaults()};
            PresetChooser cut(wxTheApp->GetTopWindow(), fake_print, &default_settings, preset_list);
            cut.load();
            THEN( "Number of preset choosers created is 3" ) {
                REQUIRE(cut.preset_choosers.size() == 3);
            }
            THEN( "Each profile chooser has 1 entry" ) {
                for (auto chooser_list : cut.preset_choosers) {
                    REQUIRE(chooser_list.size() == 1);
                    for (auto* chooser : chooser_list) {
                        REQUIRE(chooser->GetCount() == 1);
                    }
                }
            }
            THEN( "Selection mapping table has 3 profile entries, all named \"- default - \"") {
                for (const auto& group : { preset_t::Print, preset_t::Material, preset_t::Printer }) {
                    REQUIRE(cut._chooser_names()[get_preset(group)].at(0) == wxString("- default -"));
                }
            }
        }
        WHEN( "load is called with non-default presets and default presets" ) {
            auto preset_list {sample()};
            PresetChooser cut(wxTheApp->GetTopWindow(), fake_print, &default_settings, preset_list);
            cut.load();
            THEN( "Number of preset choosers created is 3" ) {
                REQUIRE(cut.preset_choosers.size() == 3);
            }
            THEN( "Each profile chooser has 1 entry" ) {
                for (auto chooser_list : cut.preset_choosers) {
                    REQUIRE(chooser_list.size() == 1);
                    for (auto* chooser : chooser_list) {
                        REQUIRE(chooser->GetCount() == 1);
                    }
                }
            }
            THEN( "Selection mapping table has 3 profile entries, none named \"- default - \"") {
                for (const auto& group : { preset_t::Print, preset_t::Material, preset_t::Printer }) {
                    REQUIRE(cut._chooser_names()[get_preset(group)].at(0) != wxString("- default -"));
                }
            }
            THEN( "Settings are updated to match selected." ) {
                REQUIRE(settings_presets[get_preset(preset_t::Print)].at(0) == wxString("print-profile"));
                REQUIRE(settings_presets[get_preset(preset_t::Printer)].at(0) == wxString("printer-profile"));
                REQUIRE(settings_presets[get_preset(preset_t::Material)].at(0) == wxString("material-profile"));
            }
        }
    }
    GIVEN( "A PresetChooser object and a Settings indicating that print-profile is the default option." ) {
        Settings test_settings;
        test_settings.default_presets.at(get_preset(preset_t::Printer)).push_back(wxString("printer-profile"));
        auto preset_list {default_compatible_reversion()};
        PresetChooser cut(wxTheApp->GetTopWindow(), fake_print, &test_settings, preset_list);
        WHEN( "load is called with non-default presets and default presets and the material is listed with an incompatible printer" ) {
            cut.load();
            THEN( "Number of preset choosers created is 3" ) {
                REQUIRE(cut.preset_choosers.size() == 3);
            }
            THEN( "Print profile chooser has 1 entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Print)]) {
                    REQUIRE(chooser->GetCount() == 1);
                }
            }
            THEN( "Printer profile chooser has 2 entries" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Printer)]) {
                    REQUIRE(chooser->GetCount() == 2);
                }
            }
            THEN( "Material profile chooser has one entry" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Material)]) {
                    REQUIRE(chooser->GetCount() == 1);
                }
            }

            THEN( "Selected printer profile entry is \"printer-profile\"" ) {
                for (auto* chooser : cut.preset_choosers[get_preset(preset_t::Printer)]) {
                    REQUIRE(chooser->GetString(chooser->GetSelection()) == wxString("printer-profile"));
                }
            }
            THEN( "Print profile entry has one entry named \"print-profile\"" ) {
                REQUIRE(cut._chooser_names()[get_preset(preset_t::Print)].at(0) == wxString("print-profile"));
            }
            THEN( "Printer profile entry has an entry named \"printer-profile\"" ) {
                REQUIRE(cut._chooser_names()[get_preset(preset_t::Printer)].at(1) == wxString("printer-profile"));
            }
            THEN( "Printer profile entry has an entry named \"printer-profile\"" ) {
                REQUIRE(cut._chooser_names()[get_preset(preset_t::Printer)].at(0) == wxString("printer-profile-2"));
            }
            THEN( "Material profile entry has one entry named \"- default -\"" ) {
                REQUIRE(cut._chooser_names()[get_preset(preset_t::Material)].at(0) == wxString("- default -"));
            }
        }
    }
}

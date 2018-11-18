#include <catch.hpp>
#ifndef WX_PRECOMP
    #include "wx/app.h"
    #include "wx/sizer.h"
    #include "wx/uiaction.h"
#endif // WX_PRECOMP
#include <iostream>
#include "testableframe.h"


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

std::array<Presets, preset_types> sample_compatible() {
    std::array<Presets, preset_types> preset_list;
    preset_list[get_preset(preset_t::Print)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "print-profile.ini", preset_t::Print));
    preset_list[get_preset(preset_t::Print)].push_back(Preset(true, "- default -", preset_t::Print));

    preset_list[get_preset(preset_t::Material)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "material-profile.ini", preset_t::Material));
    preset_list[get_preset(preset_t::Material)][0].config().lock()->get_ptr<ConfigOptionStrings>("compatible_printers"s)->append("not-printer-profile"s);
    preset_list[get_preset(preset_t::Material)].push_back(Preset(true, "- default -", preset_t::Material));

    preset_list[get_preset(preset_t::Printer)].push_back(Preset(true, "- default -", preset_t::Printer));
    preset_list[get_preset(preset_t::Printer)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "printer-profile.ini", preset_t::Printer));
    preset_list[get_preset(preset_t::Printer)].push_back(Preset(testfile_dir + "test_preset_chooser"s, "printer-profile-2.ini", preset_t::Printer));

    return preset_list;
}

SCENARIO( "PresetChooser Preset loading" ) {
    Print fake_print;
    Settings default_settings;
    wxUIActionSimulator sim;
    wxTestableFrame* old = dynamic_cast<wxTestableFrame*>(wxTheApp->GetTopWindow()); 
    old->Destroy();
    wxTheApp->SetTopWindow(new wxTestableFrame());

    GIVEN( "A PresetChooser object." ) {
        PresetChooser cut(wxTheApp->GetTopWindow(), fake_print, default_settings);
        WHEN( "load() is called with only default presets" ) {
            cut.load(defaults());
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
            cut.load(sample());
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
        }
    }
    GIVEN( "A PresetChooser object and a Settings indicating that print-profile is the default option." ) {
        Settings test_settings;
        test_settings.default_presets.at(get_preset(preset_t::Printer)).push_back(wxString("printer-profile"));
        PresetChooser cut(wxTheApp->GetTopWindow(), fake_print, test_settings);
        WHEN( "load is called with non-default presets and default presets and the material is listed with an incompatible printer" ) {
            cut.load(sample_compatible());
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
                REQUIRE(cut._chooser_names()[get_preset(preset_t::Printer)].at(0) == wxString("printer-profile"));
            }
            THEN( "Printer profile entry has an entry named \"printer-profile\"" ) {
                REQUIRE(cut._chooser_names()[get_preset(preset_t::Printer)].at(1) == wxString("printer-profile-2"));
            }
            THEN( "Material profile entry has one entry named \"- default -\"" ) {
                REQUIRE(cut._chooser_names()[get_preset(preset_t::Material)].at(0) == wxString("- default -"));
            }
        }
    }
}

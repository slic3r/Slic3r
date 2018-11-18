#include <catch.hpp>
#include "Preset.hpp"
#include "Config.hpp"
#include <test_options.hpp>

using namespace Slic3r::GUI;
using namespace std::literals::string_literals;

SCENARIO( "Preset construction" ) {
    GIVEN("A preset file with at least one configuration") {
        WHEN( "Preset is constructed." ) {
            Preset item {std::string(testfile_dir) + "test_preset", "preset_load_numeric.ini"s, preset_t::Print};
            THEN("Name is preset_load_numeric") {
                REQUIRE(item.name == "preset_load_numeric"s);
            }
            THEN("group is Print") {
                REQUIRE(item.group == preset_t::Print);
            }
            THEN("preset default is false.") {
                REQUIRE(item.default_preset == false);
            }
        }
    }
    GIVEN("A default preset construction for Print") {
        WHEN( "Preset is constructed." ) {
            Preset item {true, "- default -", preset_t::Print};
            THEN("Name is - default -") {
                REQUIRE(item.name == "- default -"s);
            }
            THEN("group is Print") {
                REQUIRE(item.group == preset_t::Print);
            }
            THEN("preset default is true.") {
                REQUIRE(item.default_preset == true);
            }
            THEN("file_exists() is false") {
                REQUIRE(item.file_exists() == false);
            }
            THEN("Does not contain filament_colour (a material option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(!config->has("filament_colour"));
                }
            }
            THEN("Does not contain gcode_flavor (a printer option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(!config->has("gcode_flavor"));
                }
            }
            THEN("Does contain layer_height (a print option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(config->has("layer_height"));
                }
            }
        }
    }
    GIVEN("A default preset construction for Material") {
        WHEN( "Preset is constructed." ) {
            Preset item {true, "- default -", preset_t::Material};
            THEN("Name is - default -") {
                REQUIRE(item.name == "- default -"s);
            }
            THEN("group is Material") {
                REQUIRE(item.group == preset_t::Material);
            }
            THEN("preset default is true.") {
                REQUIRE(item.default_preset == true);
            }
            THEN("file_exists() is false") {
                REQUIRE(item.file_exists() == false);
            }
            THEN("Does contain filament_colour (a material option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(config->has("filament_colour"));
                }
            }
            THEN("Does not contain gcode_flavor (a printer option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(!config->has("gcode_flavor"));
                }
            }
            THEN("Does not contain layer_height (a print option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(!config->has("layer_height"));
                }
            }
        }
    }
    GIVEN("A default preset construction for Printer") {
        WHEN( "Preset is constructed." ) {
            Preset item {true, "- default -", preset_t::Printer};
            THEN("Name is - default -") {
                REQUIRE(item.name == "- default -"s);
            }
            THEN("group is Printer") {
                REQUIRE(item.group == preset_t::Printer);
            }
            THEN("preset default is true.") {
                REQUIRE(item.default_preset == true);
            }
            THEN("file_exists() is false") {
                REQUIRE(item.file_exists() == false);
            }
            THEN("Does not contain filament_colour (a material option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(!config->has("filament_colour"));
                }
            }
            THEN("Does contain gcode_flavor (a printer option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(config->has("gcode_flavor"));
                }
            }
            THEN("Does not contain layer_height (a print option)") {
                if (auto config = item.config().lock()) {
                    REQUIRE(!config->has("layer_height"));
                }
            }
        }
    }
}
SCENARIO( "Preset loading" ) {
    GIVEN("A preset file with a config item that has adaptive_slicing = 1") {
        Preset item {std::string(testfile_dir) + "test_preset", "preset_load_numeric.ini"s, preset_t::Print};
        THEN("file_exists() returns true") {
            REQUIRE(item.file_exists() == true);
        }
        config_ptr ref;
        WHEN("The preset file with one item is loaded") {
            ref = item.load_config();
            THEN("Config is not dirty.") {
                REQUIRE(item.dirty() == false);
            }
            THEN("adaptive_slicing = 1 in the preset config") {
                REQUIRE(item.dirty_config().get<ConfigOptionBool>("adaptive_slicing").getBool() == true);
            }
        }
        auto config { Slic3r::Config::new_from_defaults() };
        ref = item.load_config();
        WHEN("Option is changed in the config via loading a config") {
            // precondition: adaptive_slicing is still true
            REQUIRE(item.dirty_config().get<ConfigOptionBool>("adaptive_slicing").getBool() == true);
            // precondition: default option for adaptice_slicing is false
            REQUIRE(config->get<ConfigOptionBool>("adaptive_slicing").getBool() == false);
            ref->apply(config);
            THEN("Config is dirty.") {
                REQUIRE(item.dirty() == true);
            }
            THEN("adaptive_slicing = 0 in the preset config") {
                REQUIRE(item.dirty_config().get<ConfigOptionBool>("adaptive_slicing").getBool() == false);
            }
            THEN("subsequent calls yield the same reference") {
                REQUIRE(item.load_config() == ref);
            }
        }
    }
    GIVEN("A preset file with a config item that has adaptive_slicing = 1 and default_preset = true") {
        Preset item {std::string(testfile_dir) + "test_preset", "preset_load_numeric.ini"s, preset_t::Print};
        item.default_preset = true;
        config_ptr ref;
        WHEN("The preset file with one item is loaded") {
            ref = item.load_config();
            THEN("Config is not dirty.") {
                REQUIRE(item.dirty() == false);
            }
            THEN("adaptive_slicing = 1 in the preset config") {
                REQUIRE(item.dirty_config().get<ConfigOptionBool>("adaptive_slicing").getBool() == true);
            }
            THEN("loaded is true") {
                REQUIRE(item.loaded() == true);
            }
            THEN("subsequent calls yield the same reference") {
                REQUIRE(item.load_config() == ref);
            }
        }
        WHEN("Option is changed in the config") {
            ref = item.load_config();
            ref->set("adaptive_slicing", false);
            THEN("Config is dirty.") {
                REQUIRE(item.dirty() == true);
            }
            THEN("adaptive_slicing = 0 in the preset config") {
                REQUIRE(item.dirty_config().get<ConfigOptionBool>("adaptive_slicing").getBool() == false);
            }
        }
    }
    GIVEN("An invalid preset file") {
        Preset item {std::string(testfile_dir) + "test_preset", "___invalid__preset_load_numeric.ini"s, preset_t::Print};
        THEN("file_exists() returns false") {
            REQUIRE(item.file_exists() == false);
        }
        THEN("loaded is false") {
            REQUIRE(item.loaded() == false);
        }
    }
}

#include <catch.hpp>

#include "Config.hpp"
#include <test_options.hpp>

#include <string>

using namespace Slic3r;
using namespace std::literals::string_literals;

SCENARIO("Generic config validation performs as expected.") {
    GIVEN("A config generated from default options") {
        auto config {Slic3r::Config::new_from_defaults()};
        WHEN( "perimeter_extrusion_width is set to 250%, a valid value") {
            config->set("perimeter_extrusion_width", "250%");
            THEN( "The config is read as valid.") {
                REQUIRE(config->validate() == true);
            }
        }
        WHEN( "perimeter_extrusion_width is set to -10, an invalid value") {
            config->set("perimeter_extrusion_width", -10);
            THEN( "An InvalidOptionValue exception is thrown.") {
                auto except_thrown {false};
                try {
                    config->validate();
                } catch (const InvalidOptionValue& e) {
                    except_thrown = true;
                }
                REQUIRE(except_thrown == true);
            }
        }

        WHEN( "perimeters is set to -10, an invalid value") {
            config->set("perimeters", -10);
            THEN( "An InvalidOptionValue exception is thrown.") {
                auto except_thrown {false};
                try {
                    config->validate();
                } catch (const InvalidOptionValue& e) {
                    except_thrown = true;
                }
                REQUIRE(except_thrown == true);
            }
        }
    }
}

SCENARIO("Config accessor functions perform as expected.") {
    GIVEN("A config generated from default options") {
        auto config {Slic3r::Config::new_from_defaults()};
        WHEN("A boolean option is set through the bool interface") {
            config->set("gcode_comments", true);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionBool>("gcode_comments").getBool() == true);
            }
        }
        WHEN("A boolean option is set through the string interface") {
            config->set("gcode_comments", "1");
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionBool>("gcode_comments").getBool() == true);
            }
        }
        WHEN("A boolean option is set through the int interface") {
            config->set("gcode_comments", 1);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionBool>("gcode_comments").getBool() == true);
            }
        }
        WHEN("A numeric option is set through the string interface") {
            config->set("bed_temperature", "100");
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionInt>("bed_temperature").getInt() == 100);
            }
        }

        WHEN("An integer-based option is set through the integer interface") {
            config->set("bed_temperature", 100);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionInt>("bed_temperature").getInt() == 100);
            }
        }
        WHEN("An floating-point option is set through the integer interface") {
            config->set("perimeter_speed", 10);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionFloat>("perimeter_speed").getFloat() == 10.0);
            }
        }
        WHEN("A floating-point option is set through the double interface") {
            config->set("perimeter_speed", 5.5);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionFloat>("perimeter_speed").getFloat() == 5.5);
            }
        }
        WHEN("An integer-based option is set through the double interface") {
            config->set("bed_temperature", 5.5);
            THEN("The underlying value is set, rounded to the nearest integer.") {
                REQUIRE(config->get<ConfigOptionInt>("bed_temperature").getInt() == 6);
            }
        }
        WHEN("A numeric option is set to a non-numeric value.") {
            THEN("An InvalidOptionValue exception is thown.") {
                REQUIRE_THROWS_AS(config->set("perimeter_speed", "zzzz"), InvalidOptionValue);
            }
            THEN("The value does not change.") {
                REQUIRE(config->get<ConfigOptionFloat>("perimeter_speed").getFloat() == 60.0);
            }
        }
        WHEN("A string option is set through the string interface") {
            config->set("octoprint_apikey", "100");
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionString>("octoprint_apikey").getString() == "100");
            }
        }
        WHEN("A string option is set through the integer interface") {
            config->set("octoprint_apikey", 100);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionString>("octoprint_apikey").getString() == "100");
            }
        }
        WHEN("A string option is set through the double interface") {
            config->set("octoprint_apikey", 100.5);
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionString>("octoprint_apikey").getString() == std::to_string(100.5));
            }
        }
        WHEN("A float or percent is set as a percent through the string interface.") {
            config->set("first_layer_extrusion_width", "100%");
            THEN("Value and percent flag are 100/true") {
                auto tmp {config->get<ConfigOptionFloatOrPercent>("first_layer_extrusion_width")};
                REQUIRE(tmp.percent == true);
                REQUIRE(tmp.value == 100);
            }
        }
        WHEN("A float or percent is set as a float through the string interface.") {
            config->set("first_layer_extrusion_width", "100");
            THEN("Value and percent flag are 100/false") {
                auto tmp {config->get<ConfigOptionFloatOrPercent>("first_layer_extrusion_width")};
                REQUIRE(tmp.percent == false);
                REQUIRE(tmp.value == 100);
            }
        }
        WHEN("A float or percent is set as a float through the int interface.") {
            config->set("first_layer_extrusion_width", 100);
            THEN("Value and percent flag are 100/false") {
                auto tmp {config->get<ConfigOptionFloatOrPercent>("first_layer_extrusion_width")};
                REQUIRE(tmp.percent == false);
                REQUIRE(tmp.value == 100);
            }
        }
        WHEN("A float or percent is set as a float through the double interface.") {
            config->set("first_layer_extrusion_width", 100.5);
            THEN("Value and percent flag are 100.5/false") {
                auto tmp {config->get<ConfigOptionFloatOrPercent>("first_layer_extrusion_width")};
                REQUIRE(tmp.percent == false);
                REQUIRE(tmp.value == 100.5);
            }
        }
        WHEN("An invalid option is requested during set.") {
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", 1), InvalidOptionType);
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", 1.0), InvalidOptionType);
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", "1"), InvalidOptionType);
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", true), InvalidOptionType);
            }
        }

        WHEN("An invalid option is requested during get.") {
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE_THROWS_AS(config->get<ConfigOptionString>("deadbeef_invalid_option", false), InvalidOptionType);
                REQUIRE_THROWS_AS(config->get<ConfigOptionFloat>("deadbeef_invalid_option", false), InvalidOptionType);
                REQUIRE_THROWS_AS(config->get<ConfigOptionInt>("deadbeef_invalid_option", false), InvalidOptionType);
                REQUIRE_THROWS_AS(config->get<ConfigOptionBool>("deadbeef_invalid_option", false), InvalidOptionType);
            }
        }
        WHEN("An invalid option is requested during get_ptr.") {
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionString>("deadbeef_invalid_option", false), InvalidOptionType);
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionFloat>("deadbeef_invalid_option", false), InvalidOptionType);
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionInt>("deadbeef_invalid_option", false), InvalidOptionType);
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionBool>("deadbeef_invalid_option", false), InvalidOptionType);
            }
        }

        WHEN("getX called on an unset option.") {
            THEN("The default is returned.") {
                REQUIRE(config->getFloat("layer_height") == 0.3);
                REQUIRE(config->getString("layer_height") == "0.3");
                REQUIRE(config->getString("layer_height") == "0.3");
                REQUIRE(config->getInt("raft_layers") == 0);
                REQUIRE(config->getBool("support_material") == false);
            }
        }

        WHEN("getFloat called on an option that has been set.") {
            config->set("layer_height", 0.5);
            THEN("The set value is returned.") {
                REQUIRE(config->getFloat("layer_height") == 0.5);
                REQUIRE(config->getString("layer_height") == "0.5");
            }
        }
    }
}

SCENARIO("Config ini load/save interface", "[!mayfail]") {
    WHEN("new_from_ini is called") {
        auto config {Slic3r::Config::new_from_ini(std::string(testfile_dir) + "test_config/new_from_ini.ini"s) };
        THEN("Config object contains ini file options.") {
        }
    }
    REQUIRE(false);
}

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
                REQUIRE_NOTHROW(config->validate());
            }
        }
        WHEN( "perimeter_extrusion_width is set to -10, an invalid value") {
            config->set("perimeter_extrusion_width", -10);
            THEN( "An InvalidOptionException exception is thrown.") {
                auto except_thrown {false};
                try {
                    config->validate();
                } catch (const InvalidOptionException& e) {
                    except_thrown = true;
                }
                REQUIRE(except_thrown == true);
            }
        }

        WHEN( "perimeters is set to -10, an invalid value") {
            config->set("perimeters", -10);
            THEN( "An InvalidOptionException exception is thrown.") {
                auto except_thrown {false};
                try {
                    config->validate();
                } catch (const InvalidOptionException& e) {
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
        WHEN("A boolean option is set to a boolean value") {
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config->set("gcode_comments", true), BadOptionTypeException);
            }
        }
        WHEN("A boolean option is set to a string value") {
            config->set("gcode_comments", "1");
            THEN("The underlying value is set correctly.") {
                REQUIRE(config->get<ConfigOptionBool>("gcode_comments").getBool() == true);
            }
        }
        WHEN("A string option is set to an int value") {
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config->set("gcode_comments", 1), BadOptionTypeException);
            }
        }
        WHEN("A numeric option is set from serialized string") {
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
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config->set("bed_temperature", 5.5), BadOptionTypeException);
            }
        }
        WHEN("A numeric option is set to a non-numeric value.") {
            THEN("A BadOptionTypeException exception is thown.") {
                REQUIRE_THROWS_AS(config->set("perimeter_speed", "zzzz"), BadOptionTypeException);
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
            THEN("A BadOptionTypeException exception is thrown.") {
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", 1), UnknownOptionException);
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", 1.0), UnknownOptionException);
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", "1"), UnknownOptionException);
                REQUIRE_THROWS_AS(config->set("deadbeef_invalid_option", true), UnknownOptionException);
            }
        }

        WHEN("An invalid option is requested during get.") {
            THEN("A UnknownOptionException exception is thrown.") {
                REQUIRE_THROWS_AS(config->get<ConfigOptionString>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config->get<ConfigOptionFloat>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config->get<ConfigOptionInt>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config->get<ConfigOptionBool>("deadbeef_invalid_option", false), UnknownOptionException);
            }
        }
        WHEN("An invalid option is requested during get_ptr.") {
            THEN("A UnknownOptionException exception is thrown.") {
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionString>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionFloat>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionInt>("deadbeef_invalid_option", false), UnknownOptionException);
                REQUIRE_THROWS_AS(config->get_ptr<ConfigOptionBool>("deadbeef_invalid_option", false), UnknownOptionException);
            }
        }

        WHEN("getX called on an unset option.") {
            THEN("The default is returned.") {
                REQUIRE(config->getFloat("layer_height") == 0.3);
                REQUIRE(config->getInt("raft_layers") == 0);
                REQUIRE(config->getBool("support_material") == false);
            }
        }

        WHEN("getFloat called on an option that has been set.") {
            config->set("layer_height", 0.5);
            THEN("The set value is returned.") {
                REQUIRE(config->getFloat("layer_height") == 0.5);
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

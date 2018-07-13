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
            auto except_thrown {false};
            try {
                config->set("perimeter_speed", "zzzz");
            } catch (const InvalidOptionValue& e) {
                except_thrown = true;
            }
            THEN("An InvalidOptionValue exception is thown.") {
                REQUIRE(except_thrown == true);
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
        WHEN("An invalid option is requested during set (string).") {
            auto except_thrown {false};
            try {
                config->set("deadbeef_invalid_option", "1");
            } catch (const InvalidOptionType& e) {
                except_thrown = true;
            }
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE(except_thrown == true);
            }
        }
        WHEN("An invalid option is requested during set (double).") {
            auto except_thrown {false};
            try {
                config->set("deadbeef_invalid_option", 1.0);
            } catch (const InvalidOptionType& e) {
                except_thrown = true;
            }
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE(except_thrown == true);
            }
        }
        WHEN("An invalid option is requested during set (int).") {
            auto except_thrown {false};
            try {
                config->set("deadbeef_invalid_option", 1);
            } catch (const InvalidOptionType& e) {
                except_thrown = true;
            }
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE(except_thrown == true);
            }
        }
        WHEN("An invalid option is requested during get.") {
            auto except_thrown {false};
            try {
                config->get<std::string>("deadbeef_invalid_option", false);
            } catch (const InvalidOptionType& e) {
                except_thrown = true;
            }
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE(except_thrown == true);
            }
        }
        WHEN("An invalid option is requested during get_ptr.") {
            auto except_thrown {false};
            try {
                config->get_ptr<std::string>("deadbeef_invalid_option", false);
            } catch (const InvalidOptionType& e) {
                except_thrown = true;
            }
            THEN("An InvalidOptionType exception is thrown.") {
                REQUIRE(except_thrown == true);
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

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
            config->set("perimeter_extrusion_width", "250\%");
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
    }
}

SCENARIO("Config accessor functions perform as expected.", "[!mayfail]") {
    GIVEN("A config generated from default options") {
        auto config {Slic3r::Config::new_from_defaults()};
        WHEN("A numeric option is set through the string interface") {
            THEN("The underlying value is set correctly.") {
                REQUIRE(false);
            }
        }
        WHEN("An integer-based option is set through the integer interface") {
            THEN("The underlying value is set correctly.") {
                REQUIRE(false);
            }
        }
        WHEN("An floating-point option is set through the integer interface") {
            THEN("The underlying value is set correctly.") {
                REQUIRE(false);
            }
        }
        WHEN("A floating-point option is set through the double interface") {
            THEN("The underlying value is set correctly.") {
                REQUIRE(false);
            }
        }
        WHEN("An integer-based option is set through the double interface") {
            THEN("The underlying value is set, rounded to the nearest integer.") {
                REQUIRE(false);
            }
        }
        WHEN("A numeric option is set to a non-numeric value.") {
            auto except_thrown {false};
            try {
                config->set("perimeter_extrusion_width", "zzzz");
            } catch (const InvalidOptionType& e) {
                except_thrown = true;
            }
            THEN("An exception is thown.") {
                REQUIRE(except_thrown == true);
            }
            THEN("The value does not change.") {
                REQUIRE(false);
            }
        }
        WHEN("An invalid option is requested during set.") {
            auto except_thrown {false};
            try {
                config->set("deadbeef_invalid_option", "1");
            } catch (const InvalidOptionType& e) {
                except_thrown = true;
            }
            THEN("An exception is thrown.") {
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
            THEN("An exception is thrown.") {
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

#include <catch.hpp>
#include <test_options.hpp>
#include "Log.hpp"

using namespace std::literals::string_literals;
using namespace Slic3r;

SCENARIO( "_Log output with std::string methods" ) {
    GIVEN("A log stream and a _Log object") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic", "This");
            THEN("Output string is Topic  FERR: This\\n") {
                REQUIRE(log.str() == "Topic  FERR: This\n");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic", "This");
            THEN("Output string is Topic   ERR: This\\n") {
                REQUIRE(log.str() == "Topic   ERR: This\n");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", "This");
            THEN("Output string is Topic  INFO: This\\n") {
                REQUIRE(log.str() == "Topic  INFO: This\n");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", "This");
            THEN("Output string is Topic  WARN: This\\n") {
                REQUIRE(log.str() == "Topic  WARN: This\n");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", "This");
            THEN("Output string is Topic  INFO: This\\n") {
                REQUIRE(log.str() == "Topic  INFO: This\n");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is Topic DEBUG: This\\n") {
                REQUIRE(log.str() == "Topic DEBUG: This\n");
            }
        }
        WHEN("msg is called with text \"This\"") {
            log.clear();
            cut->raw("This");
            THEN("Output string is This\\n") {
                REQUIRE(log.str() == "This\n");
            }
        }
    }
}
SCENARIO( "_Log output with << methods" ) {
    GIVEN("A log stream and a _Log object") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic") << "This";
            THEN("Output string is Topic  FERR: This") {
                REQUIRE(log.str() == "Topic  FERR: This");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic") << "This";
            THEN("Output string is Topic   ERR: This") {
                REQUIRE(log.str() == "Topic   ERR: This");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic") << "This";
            THEN("Output string is Topic  INFO: This") {
                REQUIRE(log.str() == "Topic  INFO: This");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic") << "This";
            THEN("Output string is Topic  WARN: This") {
                REQUIRE(log.str() == "Topic  WARN: This");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic") << "This";
            THEN("Output string is Topic  INFO: This") {
                REQUIRE(log.str() == "Topic  INFO: This");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic") << "This";
            THEN("Output string is Topic DEBUG: This") {
                REQUIRE(log.str() == "Topic DEBUG: This");
            }
        }
        WHEN("msg is called with text \"This\"") {
            log.clear();
            cut->raw() << "This";
            THEN("Output string is This") {
                REQUIRE(log.str() == "This");
            }
        }

    }
}


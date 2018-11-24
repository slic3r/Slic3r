#include <catch.hpp>
#include <test_options.hpp>
#include "Log.hpp"

using namespace std::literals::string_literals;
using namespace Slic3r;

SCENARIO( "_Log output with std::string methods" ) {
    GIVEN("A log stream and a _Log object") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_level(log_t::DEBUG);
        cut->set_inclusive(true);
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
SCENARIO( "_Log output with std::wstring methods" ) {
    GIVEN("A log stream and a _Log object") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_level(log_t::DEBUG);
        cut->set_inclusive(true);
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic", L"This");
            THEN("Output string is Topic  FERR: This\\n") {
                REQUIRE(log.str() == "Topic  FERR: This\n");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic", L"This");
            THEN("Output string is Topic   ERR: This\\n") {
                REQUIRE(log.str() == "Topic   ERR: This\n");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", L"This");
            THEN("Output string is Topic  INFO: This\\n") {
                REQUIRE(log.str() == "Topic  INFO: This\n");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", L"This");
            THEN("Output string is Topic  WARN: This\\n") {
                REQUIRE(log.str() == "Topic  WARN: This\n");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", L"This");
            THEN("Output string is Topic  INFO: This\\n") {
                REQUIRE(log.str() == "Topic  INFO: This\n");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", L"This");
            THEN("Output string is Topic DEBUG: This\\n") {
                REQUIRE(log.str() == "Topic DEBUG: This\n");
            }
        }
        WHEN("msg is called with text \"This\"") {
            log.clear();
            cut->raw(L"This");
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
        cut->set_level(log_t::DEBUG);
        cut->set_inclusive(true);
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

SCENARIO( "_Log output inclusive filtering with std::string methods" ) {
    GIVEN("Single, inclusive log level of FERR (highest)") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->clear_level(log_t::FERR);
        cut->set_inclusive(true);
        cut->set_level(log_t::FERR);
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
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
    GIVEN("Single, inclusive log level of ERR (second-highest)") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(true);
        cut->set_level(log_t::ERR);
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
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
    GIVEN("Single, inclusive log level of WARN (third-highest)") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(true);
        cut->set_level(log_t::WARN);
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
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
    GIVEN("Single, inclusive log level of INFO (fourth-highest)") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(true);
        cut->set_level(log_t::INFO);
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
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
    GIVEN("Single, inclusive log level of DEBUG (fifth-highest)") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(true);
        cut->set_level(log_t::DEBUG);
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
    }
}

SCENARIO( "_Log output set filtering with std::string methods" ) {

    GIVEN("log level of DEBUG only") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(false);
        cut->clear_level(log_t::ALL);
        cut->set_level(log_t::DEBUG);
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is Topic DEBUG: This\\n") {
                REQUIRE(log.str() == "Topic DEBUG: This\n");
            }
        }
    }
    GIVEN("log level of INFO only") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(false);
        cut->clear_level(log_t::ALL);
        cut->set_level(log_t::INFO);
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
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
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
    GIVEN("log level of WARN only") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(false);
        cut->clear_level(log_t::ALL);
        cut->set_level(log_t::WARN);
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
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
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
    GIVEN("log level of FERR only") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(false);
        cut->clear_level(log_t::ALL);
        cut->set_level(log_t::FERR);
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
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
    GIVEN("log level of DEBUG and ERR") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(false);
        cut->clear_level(log_t::ALL);
        cut->set_level(log_t::DEBUG);
        cut->set_level(log_t::ERR);
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic", "This");
            THEN("Output string is Topic   ERR: This\\n") {
                REQUIRE(log.str() == "Topic   ERR: This\n");
            }
        }
        WHEN("warn is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->warn("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("info is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->info("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("debug is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->debug("Topic", "This");
            THEN("Output string is Topic DEBUG: This\\n") {
                REQUIRE(log.str() == "Topic DEBUG: This\n");
            }
        }
    }
    GIVEN("log level of INFO and WARN") {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(false);
        cut->clear_level(log_t::ALL);
        cut->set_level(log_t::INFO);
        cut->set_level(log_t::WARN);
        cut->set_inclusive(false);
        WHEN("fatal_error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->fatal_error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
        WHEN("error is called with topic \"Topic\" and text \"This\"") {
            log.clear();
            cut->error("Topic", "This");
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
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
            THEN("Output string is blank") {
                REQUIRE(log.str() == "");
            }
        }
    }
}

SCENARIO( "_Log output filtering on topic name" ) {
        std::stringstream log;
        std::unique_ptr<_Log> cut { _Log::make_log(log) };
        cut->set_inclusive(true);
        cut->set_level(log_t::ALL);
        WHEN("Topic is \"t1\"") {
            cut->add_topic("t1");
            cut->debug("t1") << "TEXT FOR T1 ";
            cut->debug("t2") << "TEXT FOR T2 ";
            cut->debug("t3") << "TEXT FOR T3";
            THEN("Log text is \"TEXT FOR T1 \"") {
                REQUIRE(log.str() == "t1 DEBUG: TEXT FOR T1 ");
            }
        }

        WHEN("Topic is \"t2\"") {
            cut->add_topic("t2");
            cut->debug("t1") << "TEXT FOR T1 ";
            cut->debug("t2") << "TEXT FOR T2 ";
            cut->debug("t3") << "TEXT FOR T3";
            THEN("Log text is \"TEXT FOR T2 \"") {
                REQUIRE(log.str() == "t2 DEBUG: TEXT FOR T2 ");
            }
        }

        WHEN("Topic is \"t3\"") {
            cut->add_topic("t3");
            cut->debug("t1") << "TEXT FOR T1 ";
            cut->debug("t2") << "TEXT FOR T2 ";
            cut->debug("t3") << "TEXT FOR T3";
            THEN("Log text is \"TEXT FOR T3\"") {
                REQUIRE(log.str() == "t3 DEBUG: TEXT FOR T3");
            }
        }

        WHEN("Topic is \"t3\" and \"t2\"") {
            cut->add_topic("t2");
            cut->add_topic("t3");
            cut->debug("t1") << "TEXT FOR T1 ";
            cut->debug("t2") << "TEXT FOR T2 ";
            cut->debug("t3") << "TEXT FOR T3";
            THEN("Log text is \"TEXT FOR T2 TEXT FOR T3\"") {
                REQUIRE(log.str() == "t2 DEBUG: TEXT FOR T2 t3 DEBUG: TEXT FOR T3");
            }
        }
}

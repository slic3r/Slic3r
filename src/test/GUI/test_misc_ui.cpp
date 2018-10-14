#include <catch.hpp>
#include "misc_ui.hpp"
#include <string>

using namespace std::string_literals;
using namespace Slic3r::GUI;

SCENARIO( "trim_zeroes leaves parsable numbers.") {
    auto n192 {"19.200000000"s};
    auto n00 {"0.0"s};
    auto n0 {"0."s};
    REQUIRE(trim_zeroes(n00) == "0.0"s);
    REQUIRE(trim_zeroes(n0) == "0.0"s);
    REQUIRE(trim_zeroes(n192) == "19.2"s);
}

SCENARIO ( "trim_zeroes doesn't reduce precision.") {
    GIVEN( "A number with a long string of zeroes and a 0") {
        auto n12 {"19.200000002"s};
        auto n120 {"19.2000000020"s};
        auto n1200 {"19.20000000200"s};

        REQUIRE(trim_zeroes(n12) == "19.200000002"s);
        REQUIRE(trim_zeroes(n120) == "19.200000002"s);
        REQUIRE(trim_zeroes(n1200) == "19.200000002"s);
    }
}

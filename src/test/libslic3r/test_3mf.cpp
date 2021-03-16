#include <catch.hpp>
#include <test_options.hpp>
#include "Model.hpp"
#include "TMF.hpp"


using namespace Slic3r;

SCENARIO("Reading 3mf file") {
    GIVEN("umlauts in the path of the file") {
        auto model {new Slic3r::Model()};
        WHEN("3mf model is read") {
            auto ret = Slic3r::IO::TMF::read(testfile("test_3mf/Geräte/box.3mf"),model);
            THEN("read should not return 0") {
                REQUIRE(ret == 1);
            }
        }
    }
}

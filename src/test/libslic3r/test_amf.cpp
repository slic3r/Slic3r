#include <catch.hpp>
#include <test_options.hpp>
#include "Model.hpp"
#include "IO.hpp"


using namespace Slic3r;

SCENARIO("Reading AMF file") {
    GIVEN("badly formed AMF file (missing vertices)") {
        auto model {new Slic3r::Model()};
        WHEN("AMF model is read") {
            auto ret = Slic3r::IO::AMF::read(testfile("test_amf/5061-malicious.amf"),model);
            THEN("read should return True") {
                REQUIRE(ret);
            }
        }
    }
    GIVEN("Ok formed AMF file") {
        auto model {new Slic3r::Model()};
        WHEN("AMF model is read") {
            auto ret = Slic3r::IO::AMF::read(testfile("test_amf/read-amf.amf"),model);
            THEN("read should return True") {
                REQUIRE(ret);
            }
        }
    }
}

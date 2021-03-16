#include <catch2/catch.hpp>
#include "test_utils.hpp"
#include "libslic3r/Model.hpp"
#include "libslic3r/Format/AMF.hpp"
#include "libslic3r/PrintConfig.hpp"

using namespace Slic3r;
using namespace std::literals::string_literals;


SCENARIO("Reading AMF file") {
    auto z = DynamicPrintConfig{};
    GIVEN("badly formed AMF file (missing vertices)") {
        auto model {new Slic3r::Model()};
        WHEN("AMF model is read") {
            auto ret = Slic3r::load_amf(get_model_path("test_amf/5061-malicious.amf").c_str(), &z, model, false);
            THEN("read should return True") {
                REQUIRE(ret);
            }
        }
    }
    GIVEN("Ok formed AMF file") {
        auto model {new Slic3r::Model()};
        WHEN("AMF model is read") {
            std::cerr << "TEST_DATA_DIR/test_amf/read-amf.amf";
            auto ret = Slic3r::load_amf(get_model_path("test_amf/read-amf.amf").c_str(), &z, model, false);
            THEN("read should return True") {
                REQUIRE(ret);
            }
        }
    }
}

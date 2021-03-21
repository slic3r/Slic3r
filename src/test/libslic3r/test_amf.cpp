#include <catch.hpp>
#include <test_options.hpp>
#include "Model.hpp"
#include "IO.hpp"


using namespace Slic3r;
using namespace std::literals::string_literals;

SCENARIO("Reading deflated AMF files", "[AMF]") {
    GIVEN("Compressed AMF file of a 20mm cube") {
        auto model {new Slic3r::Model()};
        WHEN("file is read") {
            bool result_code = Slic3r::IO::AMF::read(testfile("test_amf/20mmbox_deflated.amf"), model);;
            THEN("Does not return false.") {
                REQUIRE(result_code == true);
            }
            THEN("Model object contains a single ModelObject.") {
                REQUIRE(model->objects.size() == 1);
            }
        }
        WHEN("single file is read with some subdirectories") {
            bool result_code = Slic3r::IO::AMF::read(testfile("test_amf/20mmbox_deflated-in_directories.amf"), model);;
            THEN("Read returns false.") {
                REQUIRE(result_code == true);
            }
            THEN("Model object contains no ModelObjects.") {
                REQUIRE(model->objects.size() == 1);
            }
        }
        WHEN("file is read with multiple files in the archive") {
            bool result_code = Slic3r::IO::AMF::read(testfile("test_amf/20mmbox_deflated-mult_files.amf"), model);;
            THEN("Read returns true.") {
                REQUIRE(result_code == true);
            }
            THEN("Model object contains one ModelObject.") {
                REQUIRE(model->objects.size() == 1);
            }
        }
        delete model;
    }
    GIVEN("Uncompressed AMF file of a 20mm cube") {
        auto model {new Slic3r::Model()};
        WHEN("file is read") {
            bool result_code = Slic3r::IO::AMF::read(testfile("test_amf/20mmbox.amf"), model);;
            THEN("Does not return false.") {
                REQUIRE(result_code == true);
            }
            THEN("Model object contains a single ModelObject.") {
                REQUIRE(model->objects.size() == 1);
            }
        }
        WHEN("nonexistant file is read") {
            bool result_code = Slic3r::IO::AMF::read(testfile("test_amf/20mmbox-doesnotexist.amf"), model);;
            THEN("Read returns false.") {
                REQUIRE(result_code == false);
            }
            THEN("Model object contains no ModelObject.") {
                REQUIRE(model->objects.size() == 0);
            }
        }
        delete model;
    }
}

SCENARIO("Reading AMF file", "[AMF]") {
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

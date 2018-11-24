#include <catch.hpp>
#include <fstream>
#include <cstdio>

#include "test_options.hpp"

#include "slic3r.hpp"

using namespace Slic3r;
using namespace std::string_literals;

bool file_exists(const std::string& name, const std::string& ext) {
    std::string filename {""};
    filename.append(name);
    filename.append(".");
    filename.append(ext);

    std::ifstream f(testfile(filename));
    bool result {f.good()};
    f.close();

    return result;
}

void clean_file(const std::string& name, const std::string& ext, bool glob = false) {
    std::string filename {""};
    filename.append(name);
    filename.append(".");
    filename.append(ext);

    std::remove(testfile(filename).c_str());
}
char** to_cstr_array(std::vector<std::string> in, char** argv) {
    int i = 0;
    for (auto& str : in) {
        argv[i] = new char[str.size()];
        strcpy(argv[i], str.c_str()); 
        i++;
    }
    return argv;
}

void clean_array(size_t argc, char** argv) {
    for (size_t i = 0; i < argc; ++i) {
        delete[] argv[i];
    }
}

SCENARIO( "CLI Export Arguments", "[!mayfail]") {
    char* args_cli[20];
    std::vector<std::string> in_args;
    in_args.reserve(20);
    in_args.emplace_back("gui_test"s);
    in_args.emplace_back(testfile("test_cli/20mmbox.stl"s));

    GIVEN( "Default configuration and a simple 3D model" ) {
        WHEN ( "[ ACTION ] is export-gcode with long option") { 
            in_args.emplace(in_args.cend()-1, "--export-gcode");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("GCode file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox"s, "gcode"s));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "gcode");
        }
        WHEN ( "[ ACTION ] is export-gcode with short option") { 
            in_args.emplace(in_args.cend()-1, "-g");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("GCode file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox"s, "gcode"s));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "gcode");
        }
        WHEN ( "[ ACTION ] is export-obj") { 
            in_args.emplace(in_args.cend()-1, "--export-obj");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("OBJ file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox"s, "obj"s));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "obj");
        }
        WHEN ( "[ ACTION ] is export-pov") { 
            in_args.emplace(in_args.cend()-1, "--export-pov");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("POV file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox", "pov"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "pov");
        }
        WHEN ( "[ ACTION ] is export-amf") { 
            in_args.emplace(in_args.cend()-1, "--export-amf");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("AMF file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox", "amf"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "amf");
        }
        WHEN ( "[ ACTION ] is export-3mf") { 
            in_args.emplace(in_args.cend()-1, "--export-3mf");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("3MF file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox", "3mf"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "3mf");
        }
        WHEN ( "[ ACTION ] is export-svg") { 
            in_args.emplace(in_args.cend()-1, "--export-svg");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("SVG file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox", "svg"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "svg");
        }
        WHEN ( "[ ACTION ] is export-sla-svg") { 
            in_args.emplace(in_args.cend()-1, "--export-sla-svg");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("SVG file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox", "svg"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "svg");
        }
        WHEN ( "[ ACTION ] is sla") { 
            in_args.emplace(in_args.cend()-1, "--sla");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("SVG file is created.") {
                REQUIRE(file_exists("test_cli/20mmbox", "svg"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "svg", true);
        }
        WHEN ( "[ ACTION ] is sla and --output-file-format is output_[padded_layer_num].svg") { 
            in_args.emplace(in_args.cend()-1, "--sla");
            in_args.emplace(in_args.cend()-1, "--output-file-format output_[padded_layer_num].svg");
            in_args.emplace(in_args.cend()-1, "--layer-height 5");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("SVG files are created.") {
                REQUIRE(file_exists("test_cli/output_0", "svg"));
                REQUIRE(file_exists("test_cli/output_1", "svg"));
                REQUIRE(file_exists("test_cli/output_2", "svg"));
                REQUIRE(file_exists("test_cli/output_3", "svg"));
                REQUIRE(file_exists("test_cli/output_4", "svg"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/output_0", "svg", true);
            clean_file("test_cli/output_1", "svg", true);
            clean_file("test_cli/output_2", "svg", true);
            clean_file("test_cli/output_3", "svg", true);
            clean_file("test_cli/output_4", "svg", true);
        }
        WHEN ( "[ ACTION ] is save") { 
            in_args.emplace(in_args.cend()-1, "--save");
            in_args.emplace(in_args.cend()-1, testfile("cfg.ini"));
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("Configuration file is created.") {
                REQUIRE(file_exists("cfg", "ini"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("cfg", "ini");
        }
        WHEN ( "[ ACTION ] is export-stl and --output option is specified") { 
            in_args.emplace(in_args.cend()-1, "--export-stl");
            in_args.emplace(in_args.cend()-1, "--output");
            in_args.emplace(in_args.cend()-1, testfile("output.stl"));
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("STL file is created.") {
                REQUIRE(file_exists("output", "stl"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("output", "stl");
        }

    }
}

SCENARIO("CLI Transform arguments", "[!shouldfail]") {
    char* args_cli[20];
    std::vector<std::string> in_args;
    in_args.reserve(20);
    in_args.emplace_back("gui_test"s);
    in_args.emplace_back(testfile("test_cli/20mmbox.stl"s));
    WHEN("Tests are implemented for CLI model transform") {
        THEN ("Tests should not fail :D") {
            REQUIRE(false); 
        }
    }
}

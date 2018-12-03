#include <catch.hpp>
#include <fstream>
#include <cstdio>

#include "test_options.hpp"

#include "slic3r.hpp"
#include "GCodeReader.hpp"

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

std::string read_to_string(const std::string& name) {
    std::stringstream buf;
    std::ifstream f(testfile(name));
    if (!f.good()) return ""s;
    std::copy(std::istreambuf_iterator<char>(f),
            std::istreambuf_iterator<char>(),
            std::ostreambuf_iterator<char>(buf));
    f.close();
    return buf.str();
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
            THEN ("SVG files are created.") {
                REQUIRE(file_exists("test_cli/20mmbox_0", "svg"));
                REQUIRE(file_exists("test_cli/20mmbox_1", "svg"));
                REQUIRE(file_exists("test_cli/20mmbox_2", "svg"));
                REQUIRE(file_exists("test_cli/20mmbox_3", "svg"));
                REQUIRE(file_exists("test_cli/20mmbox_4", "svg"));
            }
            clean_file("test_cli/20mmbox_0", "svg", true);
            clean_file("test_cli/20mmbox_1", "svg", true);
            clean_file("test_cli/20mmbox_2", "svg", true);
            clean_file("test_cli/20mmbox_3", "svg", true);
            clean_file("test_cli/20mmbox_4", "svg", true);
            clean_array(in_args.size(), args_cli);
        }
        WHEN ( "[ ACTION ] is export-sla-svg") {
            in_args.emplace(in_args.cend()-1, "--export-sla-svg");
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("SVG files are created.") {
                REQUIRE(file_exists("test_cli/20mmbox", "svg"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("test_cli/20mmbox", "svg", true);
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
        WHEN ( "[ ACTION ] is sla and --output is output.svg") {
            in_args.emplace(in_args.cend()-1, "--sla");
            in_args.emplace(in_args.cend()-1, "--output");
            in_args.emplace(in_args.cend()-1, testfile("output.svg"));
            CLI().run(in_args.size(), to_cstr_array(in_args, args_cli));
            THEN ("SVG files are created.") {
                REQUIRE(file_exists("output", "svg"));
            }
            clean_array(in_args.size(), args_cli);
            clean_file("output", "svg", true);
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

// Test the --center and --dont-arrange parameters.
SCENARIO("CLI positioning arguments") {
    char* args_cli[20];
    std::vector<std::string> in_args;
    in_args.reserve(20);
    in_args.emplace_back("gui_test"s);
    GIVEN( " 3D Model for a 20mm box, centered around 0,0 and gcode export" ) {
        in_args.emplace_back(testfile("test_cli/20mmbox.stl"s));
        in_args.emplace(in_args.cend()-1, "-g");
        in_args.emplace(in_args.cend()-1, "--load"s);
        in_args.emplace(in_args.cend()-1, testfile("test_cli/20mmbox_config.ini"));
        CLI cut;

        WHEN("--center is supplied with 40,40") {
            in_args.emplace(in_args.cend()-1, "--center");
            in_args.emplace(in_args.cend()-1, "40,40");
            cut.run(in_args.size(), to_cstr_array(in_args, args_cli));

            THEN ("The first layer of the print should be centered around 0,0") {
                std::string exported { read_to_string("test_cli/20mmbox.gcode"s)};
                auto reader {GCodeReader()};
                REQUIRE(exported != ""s);
                double min_x = 50.0, max_x = -50.0, min_y = 50.0, max_y = -50.0;
                reader.apply_config(cut.full_print_config_ref());
                reader.parse(exported, [&min_x, &min_y, &max_x, &max_y] (GCodeReader& self, const GCodeReader::GCodeLine& line)
                {
                    if (self.X != 0.0 && self.Y != 0.0) { // avoid the first pass
                        if (self.Z <= 0.6 and self.Z > 0.3) {
                            min_x = std::min(min_x, static_cast<double>(self.X));
                            min_y = std::min(min_y, static_cast<double>(self.Y));
                            max_x = std::max(max_x, static_cast<double>(self.X));
                            max_y = std::max(max_y, static_cast<double>(self.Y));
                        }
                    }
                    });
                AND_THEN("Minimum X encountered is about 30.1") {
                    REQUIRE(min_x == Approx(30.1));
                }
                AND_THEN("Minimum Y encountered is about 30.1") {
                    REQUIRE(min_y == Approx(30.1));
                }
                AND_THEN("Maximum X encountered is about 49.9") {
                    REQUIRE(max_x == Approx(49.9));
                }
                AND_THEN("Maximum Y encountered is about 49.9") {
                    REQUIRE(max_y == Approx(49.9));
                }
                }
            }
            WHEN("--dont-arrange is supplied") {
                in_args.emplace(in_args.cend()-1, "--dont-arrange");
                cut.run(in_args.size(), to_cstr_array(in_args, args_cli));

                THEN ("The first layer of the print should be centered around 0,0") {
                    auto reader {GCodeReader()};
                    std::string exported { read_to_string("test_cli/20mmbox.gcode"s)};
                    REQUIRE(exported != ""s);
                    double min_x, max_x, min_y, max_y;
                    reader.apply_config(cut.full_print_config_ref());
                    reader.parse(exported, [&min_x, &min_y, &max_x, &max_y] (GCodeReader& self, const GCodeReader::GCodeLine& line)
                    {
                        if (self.Z < 0.6) {
                            min_x = std::min(min_x, static_cast<double>(self.X));
                            min_y = std::min(min_y, static_cast<double>(self.Y));
                            max_x = std::max(max_x, static_cast<double>(self.X));
                            max_y = std::max(max_y, static_cast<double>(self.Y));
                        }
                    });
                AND_THEN("Minimum X encountered is about -9.9") {
                    REQUIRE(min_x == Approx(-9.9));
                }
                AND_THEN("Minimum Y encountered is about -9.9") {
                    REQUIRE(min_y == Approx(-9.9));
                }
                AND_THEN("Maximum X encountered is about 9.9") {
                    REQUIRE(max_x == Approx(9.9));
                }
                AND_THEN("Maximum Y encountered is about 9.9") {
                    REQUIRE(max_y == Approx(9.9));
                }
            }
        }
        clean_array(in_args.size(), args_cli);
        clean_file("test_cli/20mmbox", "gcode");
    }
}

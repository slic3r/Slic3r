#include "slic3r.hpp"
#include "GCodeSender.hpp"
#include "Geometry.hpp"
#include "IO.hpp"
#include "Log.hpp"
#include "SLAPrint.hpp"
#include "Print.hpp"
#include "SimplePrint.hpp"
#include "TriangleMesh.hpp"
#include "libslic3r.h"
#include <cmath>
#include <chrono>
#include <cstdio>
#include <string>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/nowide/iostream.hpp>
#include <stdexcept>
#include <sstream>
#include <string>

#ifdef USE_WX
    #include "GUI/GUI.hpp"
#endif

using namespace Slic3r;

#ifndef BUILD_TEST
int
main(int argc, char **argv) {
    return CLI().run(argc, argv);
}
#endif // BUILD_TEST

int CLI::run(int argc, char **argv) {
    #ifdef SLIC3R_DEBUG
        slic3r_log->set_level(log_t::DEBUG);
    #endif
    // Convert arguments to UTF-8 (needed on Windows).
    // argv then points to memory owned by a.
    boost::nowide::args a(argc, argv);
    // parse all command line options into a DynamicConfig
    t_config_option_keys opt_order;
    this->config_def.merge(cli_actions_config_def);
    this->config_def.merge(cli_transform_config_def);
    this->config_def.merge(cli_misc_config_def);
    this->config_def.merge(print_config_def);
    this->config.def = &this->config_def;
    Slic3r::Log::debug("CLI")<<"Configs merged.\n";
    // if any option is unsupported, print usage and abort immediately
    if (!this->config.read_cli(argc, argv, &this->input_files, &opt_order)) {
        this->print_help();
        exit(EXIT_FAILURE);
    }
    // parse actions and transform options
    for (auto const &opt_key : opt_order) {
        if (cli_actions_config_def.has(opt_key)) this->actions.push_back(opt_key);
        if (cli_transform_config_def.has(opt_key)) this->transforms.push_back(opt_key);
    }
    // load config files supplied via --load
    for (auto const &file : config.getStrings("load", {})) {
        try{
            if (!boost::filesystem::exists(file)) {
                if (config.getBool("ignore_nonexistent_file", false)) {
                    continue;
                } else {
                    throw std::invalid_argument("No such file");
                }
            }
            DynamicPrintConfig c;
            c.load(file);
            c.normalize();
            this->print_config.apply(c);
        } catch (std::exception &e){
            Slic3r::Log::error("CLI") << "Error with the config file '" << file << "': " << e.what() <<std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    // apply command line options to a more specific DynamicPrintConfig which provides normalize()
    // (command line options override --load files)
    this->print_config.apply(config, true);
    this->print_config.normalize();
    Slic3r::Log::debug("CLI") << "Print config normalized" << std::endl;
    // create a static (full) print config to be used in our logic
    this->full_print_config.apply(this->print_config, true);
    Slic3r::Log::debug("CLI") << "Full print config created" << std::endl;
    // validate config
    try {
        this->full_print_config.validate();
    } catch (std::exception &e) {
        Slic3r::Log::error("CLI") << "Config validation error: "<< e.what() << std::endl;
        exit(EXIT_FAILURE);
    }
    Slic3r::Log::debug("CLI") << "Config validated" << std::endl;

    // read input file(s) if any
    for (auto const &file : input_files) {
        Model model;
        try {
            model = Model::read_from_file(file);
        } catch (std::exception &e) {
            Slic3r::Log::error("CLI") << file << ": " << e.what() << std::endl;
            exit(EXIT_FAILURE);
        }
        if (model.objects.empty()) {
            Slic3r::Log::error("CLI") << "Error: file is empty: " << file << std::endl;
            continue;
        }
        this->models.push_back(model);
    }
    
    // loop through transform options
    for (auto const &opt_key : this->transforms) {
        if (opt_key == "merge") {
            Model m;
            for (auto &model : this->models)
                m.merge(model);
            
            // Rearrange instances unless --dont-arrange is supplied
            if (!this->config.getBool("dont_arrange")) {
                m.add_default_instances();
                const BoundingBoxf bb{ this->full_print_config.bed_shape.values };
                m.arrange_objects(
                    this->full_print_config.min_object_distance(),
                    // if we are going to use the merged model for printing, honor
                    // the configured print bed for arranging, otherwise do it freely
                    this->has_print_action() ? &bb : nullptr
                );
            }
            
            this->models = {m};
        } else if (opt_key == "duplicate") {
            const BoundingBoxf bb{ this->full_print_config.bed_shape.values };
            for (auto &model : this->models) {
                const bool all_objects_have_instances = std::none_of(
                    model.objects.begin(), model.objects.end(),
                    [](ModelObject* o){ return o->instances.empty(); }
                );
                if (all_objects_have_instances) {
                    // if all input objects have defined position(s) apply duplication to the whole model
                    model.duplicate(this->config.getInt("duplicate"), this->full_print_config.min_object_distance(), &bb);
                } else {
                    model.add_default_instances();
                    model.duplicate_objects(this->config.getInt("duplicate"), this->full_print_config.min_object_distance(), &bb);
                }
            }
        } else if (opt_key == "duplicate_grid") {
            auto &ints = this->config.opt<ConfigOptionInts>("duplicate_grid")->values;
            const int x = ints.size() > 0 ? ints.at(0) : 1;
            const int y = ints.size() > 1 ? ints.at(1) : 1;
            const double distance = this->full_print_config.duplicate_distance.value;
            for (auto &model : this->models)
                model.duplicate_objects_grid(x, y, (distance > 0) ? distance : 6);  // TODO: this is not the right place for setting a default
        } else if (opt_key == "center") {
            for (auto &model : this->models) {
                model.add_default_instances();
                // this affects instances:
                model.center_instances_around_point(config.opt<ConfigOptionPoint>("center")->value);
                // this affects volumes:
                model.align_to_ground();
            }
        } else if (opt_key == "align_xy") {
            const Pointf p{ this->config.opt<ConfigOptionPoint>("align_xy")->value };
            for (auto &model : this->models) {
                BoundingBoxf3 bb{ model.bounding_box() };
                // this affects volumes:
                model.translate(-(bb.min.x - p.x), -(bb.min.y - p.y), -bb.min.z);
            }
        } else if (opt_key == "dont_arrange") {
            // do nothing - this option alters other transform options
        } else if (opt_key == "rotate") {
            for (auto &model : this->models)
                for (auto &o : model.objects)
                    // this affects volumes:
                    o->rotate(Geometry::deg2rad(config.getFloat(opt_key)), Z);
        } else if (opt_key == "rotate_x") {
            for (auto &model : this->models)
                for (auto &o : model.objects)
                    // this affects volumes:
                    o->rotate(Geometry::deg2rad(config.getFloat(opt_key)), X);
        } else if (opt_key == "rotate_y") {
            for (auto &model : this->models)
                for (auto &o : model.objects)
                    // this affects volumes:
                    o->rotate(Geometry::deg2rad(config.getFloat(opt_key)), Y);
        } else if (opt_key == "scale") {
            for (auto &model : this->models)
                for (auto &o : model.objects)
                    // this affects volumes:
                    o->scale(config.get_abs_value(opt_key, 1));
        } else if (opt_key == "scale_to_fit") {
            const auto opt = config.opt<ConfigOptionPoint3>(opt_key);
            if (!opt->is_positive_volume()) {
                Slic3r::Log::error("CLI") << "--scale-to-fit requires a positive volume" << std::endl;
                exit(EXIT_FAILURE);
            }
            for (auto &model : this->models)
                for (auto &o : model.objects)
                    // this affects volumes:
                    o->scale_to_fit(opt->value);
        } else if (opt_key == "cut" || opt_key == "cut_x" || opt_key == "cut_y") {
            std::vector<Model> new_models;
            for (auto &model : this->models) {
                model.repair();
                model.translate(0, 0, -model.bounding_box().min.z);  // align to z = 0
                
                Model out;
                for (auto &o : model.objects) {
                    if (opt_key == "cut_x") {
                        o->cut(X, config.getFloat("cut_x"), &out);
                    } else if (opt_key == "cut_y") {
                        o->cut(Y, config.getFloat("cut_y"), &out);
                    } else if (opt_key == "cut") {
                        o->cut(Z, config.getFloat("cut"), &out);
                    }
                }
                
                // add each resulting object as a distinct model
                Model upper, lower;
                auto upper_obj = upper.add_object(*out.objects[0]);
                auto lower_obj = lower.add_object(*out.objects[1]);
                
                if (upper_obj->facets_count() > 0) new_models.push_back(upper);
                if (lower_obj->facets_count() > 0) new_models.push_back(lower);
            }
            
            // TODO: copy less stuff around using pointers
            this->models = new_models;
            
            if (this->actions.empty())
                this->actions.push_back("export_stl");
        } else if (opt_key == "cut_grid") {
            std::vector<Model> new_models;
            for (auto &model : this->models) {
                TriangleMesh mesh = model.mesh();
                mesh.repair();
            
                TriangleMeshPtrs meshes = mesh.cut_by_grid(config.opt<ConfigOptionPoint>("cut_grid")->value);
                size_t i = 0;
                for (TriangleMesh* m : meshes) {
                    Model out;
                    auto o = out.add_object();
                    o->add_volume(*m);
                    o->input_file += "_" + std::to_string(i++);
                    delete m;
                }
            }
            
            // TODO: copy less stuff around using pointers
            this->models = new_models;
            
            if (this->actions.empty())
                this->actions.push_back("export_stl");
        } else if (opt_key == "split") {
            for (auto &model : this->models)
                model.split();
        } else if (opt_key == "repair") {
            for (auto &model : this->models)
                model.repair();
        } else {
	    Slic3r::Log::error("CLI") << " option not implemented yet: " << opt_key << std::endl;
	    exit(EXIT_FAILURE);
        }
    }
    
    // loop through action options
    for (auto const &opt_key : this->actions) {
        if (opt_key == "help") {
            this->print_help();
        } else if (opt_key == "help_options") {
            this->print_help(true);
        } else if (opt_key == "save") {
            this->print_config.save(config.getString("save"));
        } else if (opt_key == "info") {
            // --info works on unrepaired model
            for (Model &model : this->models) {
                model.add_default_instances();
                model.print_info();
            }
        } else if (opt_key == "export_stl") {
            for (auto &model : this->models)
                model.add_default_instances();
            this->export_models(IO::STL);
        } else if (opt_key == "export_obj") {
            for (auto &model : this->models)
                model.add_default_instances();
            this->export_models(IO::OBJ);
        } else if (opt_key == "export_pov") {
            for (auto &model : this->models)
                model.add_default_instances();
            this->export_models(IO::POV);
        } else if (opt_key == "export_amf") {
            this->export_models(IO::AMF);
        } else if (opt_key == "export_3mf") {
            this->export_models(IO::TMF);
        } else if (opt_key == "export_sla") {
            Slic3r::Log::error("CLI") << "--export-sla is not implemented yet" << std::endl;
        } else if (opt_key == "export_sla_svg") {
            for (const Model &model : this->models) {
                SLAPrint print(&model); // initialize print with model
                print.config.apply(this->print_config, true); // apply configuration
                print.slice(); // slice file
                const std::string outfile = this->output_filepath(model, IO::SVG);
                print.write_svg(outfile); // write SVG
                boost::nowide::cout << "SVG file exported to " << outfile << std::endl;
            }
        } else if (opt_key == "export_gcode") {
            for (const Model &model : this->models) {
                // If all objects have defined instances, their relative positions will be
                // honored when printing (they will be only centered, unless --dont-arrange
                // is supplied); if any object has no instances, it will get a default one
                // and all instances will be rearranged (unless --dont-arrange is supplied).
                SimplePrint print;
                print.status_cb = [](int ln, const std::string& msg) {
                    boost::nowide::cout << msg << std::endl;
                };
                print.apply_config(this->print_config);
                print.arrange = !this->config.getBool("dont_arrange", false);
                print.center = !this->config.has("center")
                    && !this->config.has("align_xy")
                    && print.arrange;
                Slic3r::Log::error("CLI") << "Arrange: " << print.arrange<< ", center: " << print.center << std::endl;
                print.set_model(model);
                
                // start chronometer
                typedef std::chrono::high_resolution_clock clock_;
                typedef std::chrono::duration<double, std::ratio<1> > second_;
                std::chrono::time_point<clock_> t0{ clock_::now() };
                
                const std::string outfile = this->output_filepath(model, IO::Gcode);
                try {
                    print.export_gcode(outfile);
                } catch (std::runtime_error &e) {
                    Slic3r::Log::error("CLI") << e.what() << std::endl;
                    exit(EXIT_FAILURE);
                }
                Slic3r::Log::info("CLI") << "G-code exported to " << outfile << std::endl;
                this->last_outfile = outfile;
                
                // output some statistics
                double duration { std::chrono::duration_cast<second_>(clock_::now() - t0).count() };
                boost::nowide::cout << std::fixed << std::setprecision(0)
                    << "Done. Process took " << (duration/60) << " minutes and "
                    << std::setprecision(3)
                    << std::fmod(duration, 60.0) << " seconds." << std::endl
                    << std::setprecision(2)
                    << "Filament required: " << print.total_used_filament() << "mm"
                    << " (" << print.total_extruded_volume()/1000 << "cm3)" << std::endl;
            }
        } else if (opt_key == "print") {
            if (this->models.size() > 1) {
                Slic3r::Log::error("CLI") <<  "error: --print is not supported for multiple jobs" << std::endl;
                exit(EXIT_FAILURE);
            }
            
            // Get last sliced G-code or the manually supplied one
            std::string gcode_file{ this->config.getString("gcode_file", "") };
            if (gcode_file.empty())
                gcode_file = this->last_outfile;
            
            if (gcode_file.empty()) {
                Slic3r::Log::error("CLI") <<  "error: no G-code file to send; supply a model to slice or --gcode-file" << std::endl;
                exit(EXIT_FAILURE);
            }

            // Check serial port options
            if (!this->print_config.has("serial_port") || !this->print_config.has("serial_speed")) {
                Slic3r::Log::error("CLI") <<  "error: missing required --serial-port and --serial-speed" << std::endl;
                exit(EXIT_FAILURE);
            }

            // Connect to printer
            Slic3r::GCodeSender sender;
            sender.connect(
                this->print_config.getString("serial_port"),
                this->print_config.getInt("serial_speed")
            );
            while (!sender.is_connected()) {}
            boost::nowide::cout << "Connected to printer" << std::endl;

            // Send file line-by-line
            std::ifstream infile(gcode_file);
            std::string line;
            while (std::getline(infile, line)) {
                sender.send(line);
            }

            // Print queue size
            while (sender.queue_size() > 0) {
                boost::nowide::cout << "Queue size: " << sender.queue_size() << std::endl;
            }
            boost::nowide::cout << "Print completed!" << std::endl;
        } else {
            Slic3r::Log::error("CLI") <<  "error: option not supported yet: " << opt_key << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    
    if (actions.empty()) {
#ifdef USE_WX
        GUI::App *gui = new GUI::App();
        try {
            gui->autosave = this->config.getString("autosave");
        } catch(Slic3r::UnknownOptionException &e) {
            // if no autosave, set a sensible default.
            gui->autosave = wxFileName::CreateTempFileName("slic3r_autosave_").ToStdString();
        }
        try {
            gui->datadir  = this->config.getString("datadir");
        } catch(Slic3r::UnknownOptionException &e) {
            // if no datadir on the CLI, set a default.
            gui->datadir = GUI::home().ToStdString();
        }
        GUI::App::SetInstance(gui);
        wxEntry(argc, argv);
#else
        Slic3r::Log::error("CLI") << "GUI support has not been built." << "\n";
#endif   
    }
    
    return 0;
}

void
CLI::print_help(bool include_print_options) const {
    boost::nowide::cout
        << "Slic3r " << SLIC3R_VERSION << " (build commit: " << BUILD_COMMIT << ")" << std::endl
        << "https://slic3r.org/ - https://github.com/slic3r/Slic3r" << std::endl << std::endl
        << "Usage: slic3r [ ACTIONS ] [ TRANSFORM ] [ OPTIONS ] [ file.stl ... ]" << std::endl
        << std::endl
        << "Actions:" << std::endl;
    cli_actions_config_def.print_cli_help(boost::nowide::cout, false);
    
    boost::nowide::cout
        << std::endl
        << "Transform options:" << std::endl;
        cli_transform_config_def.print_cli_help(boost::nowide::cout, false);
    
    boost::nowide::cout
        << std::endl
        << "Other options:" << std::endl;
        cli_misc_config_def.print_cli_help(boost::nowide::cout, false);
    
    if (include_print_options) {
        boost::nowide::cout << std::endl;
        print_config_def.print_cli_help(boost::nowide::cout, true);
    } else {
        boost::nowide::cout
            << std::endl
            << "Run --help-options to see the full listing of print/G-code options." << std::endl;
    }
}

void
CLI::export_models(IO::ExportFormat format) {
    for (const Model& model : this->models) {
        const std::string outfile = this->output_filepath(model, format);
        
        IO::write_model.at(format)(model, outfile);
        std::cout << "File exported to " << outfile << std::endl;
    }
}

std::string
CLI::output_filepath(const Model &model, IO::ExportFormat format) const {
    // get the --output-filename-format option
    std::string filename_format = this->print_config.getString("output_filename_format", "[input_filename_base]");
    
    // strip the file extension and add the correct one
    filename_format = filename_format.substr(0, filename_format.find_last_of("."));
    filename_format += "." + IO::extensions.at(format);
    
    // this is the same logic used in Print::output_filepath()
    // TODO: factor it out to a single place?
    
    // find the first input_file of the model
    boost::filesystem::path input_file;
    for (auto o : model.objects) {
        if (!o->input_file.empty()) {
            input_file = o->input_file;
            break;
        }
    }
    
    // compute the automatic filename
    PlaceholderParser pp;
    pp.set("input_filename", input_file.filename().string());
    pp.set("input_filename_base", input_file.stem().string());
    pp.apply_config(this->config);
    const std::string filename = pp.process(filename_format);
    
    // use --output when available
    std::string outfile{ this->config.getString("output", "") };
    if (!outfile.empty()) {
        // if we were supplied a directory, use it and append our automatically generated filename
        const boost::filesystem::path out(outfile);
        if (boost::filesystem::is_directory(out))
            outfile = (out / filename).string();
    } else {
        outfile = (input_file.parent_path() / filename).string();
    }
    
    return outfile;
}

#include "Config.hpp"
#include "Model.hpp"
#include "IO.hpp"
#include "TriangleMesh.hpp"
#include "SVGExport.hpp"
#include "libslic3r.h"
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>

using namespace Slic3r;

void confess_at(const char *file, int line, const char *func, const char *pat, ...){}

int
main(const int argc, const char **argv)
{
    // parse all command line options into a DynamicConfig
    ConfigDef config_def;
    config_def.merge(cli_config_def);
    config_def.merge(print_config_def);
    DynamicConfig config(&config_def);
    t_config_option_keys input_files;
    config.read_cli(argc, argv, &input_files);
    
    // apply command line options to a more specific DynamicPrintConfig which provides normalize()
    DynamicPrintConfig print_config;
    print_config.apply(config, true);
    print_config.normalize();
    
    // apply command line options to a more handy CLIConfig
    CLIConfig cli_config;
    cli_config.apply(config, true);
    
    /*  TODO: loop through the config files supplied on the command line (now stored in 
        cli_config), load each one, normalize it and apply it to print_config */
    
    // read input file(s) if any
    std::vector<Model> models;
    for (t_config_option_keys::const_iterator it = input_files.begin(); it != input_files.end(); ++it) {
        Model model;
        // TODO: read other file formats with Model::read_from_file()
        Slic3r::IO::STL::read(*it, &model);
        
        if (model.objects.empty()) {
            printf("Error: file is empty: %s\n", it->c_str());
            continue;
        }
        
        model.add_default_instances();
        
        // apply command line transform options
        for (ModelObjectPtrs::iterator o = model.objects.begin(); o != model.objects.end(); ++o) {
            (*o)->scale(cli_config.scale.value);
            (*o)->rotate(cli_config.rotate.value, Z);
        }
        
        // TODO: handle --merge
        models.push_back(model);
    }
    
    for (std::vector<Model>::iterator model = models.begin(); model != models.end(); ++model) {
        if (cli_config.info) {
            model->print_info();
        } else if (cli_config.export_obj) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = model->objects.front()->input_file + ".obj";
    
            TriangleMesh mesh = model->mesh();
            Slic3r::IO::OBJ::write(mesh, outfile);
            printf("File exported to %s\n", outfile.c_str());
        } else if (cli_config.export_pov) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = model->objects.front()->input_file + ".pov";
    
            TriangleMesh mesh = model->mesh();
            Slic3r::IO::POV::write(mesh, outfile);
            printf("File exported to %s\n", outfile.c_str());
        } else if (cli_config.export_svg) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = model->objects.front()->input_file + ".svg";

            SVGExport svg_export(model->mesh());
            svg_export.config.apply(print_config, true);
            svg_export.writeSVG(outfile);
            printf("SVG file exported to %s\n", outfile.c_str());
        } else {
            std::cerr << "error: only --export-svg and --export-obj are currently supported" << std::endl;
            return 1;
        }
    }
    
    return 0;
}

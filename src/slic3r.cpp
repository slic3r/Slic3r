#include "Config.hpp"
#include "Geometry.hpp"
#include "IO.hpp"
#include "Model.hpp"
#include "SLAPrint.hpp"
#include "TriangleMesh.hpp"
#include "libslic3r.h"
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include <math.h>
#include "boost/filesystem.hpp"

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
    
    // apply command line options to a more handy CLIConfig
    CLIConfig cli_config;
    cli_config.apply(config, true);
    
    DynamicPrintConfig print_config;
    
    // load config files supplied via --load
    for (std::vector<std::string>::const_iterator file = cli_config.load.values.begin();
        file != cli_config.load.values.end(); ++file) {
        if (!boost::filesystem::exists(*file)) {
            std::cout << "No such file: " << *file << std::endl;
            exit(1);
        }
        
        DynamicPrintConfig c;
        try {
            c.load(*file);
        } catch (std::exception &e) {
            std::cout << "Error while reading config file: " << e.what() << std::endl;
            exit(1);
        }
        c.normalize();
        print_config.apply(c);
    }
    
    // apply command line options to a more specific DynamicPrintConfig which provides normalize()
    // (command line options override --load files)
    print_config.apply(config, true);
    print_config.normalize();
    
    // write config if requested
    if (!cli_config.save.value.empty()) print_config.save(cli_config.save.value);
    
    // read input file(s) if any
    std::vector<Model> models;
    for (t_config_option_keys::const_iterator it = input_files.begin(); it != input_files.end(); ++it) {
        if (!boost::filesystem::exists(*it)) {
            std::cout << "No such file: " << *it << std::endl;
            exit(1);
        }
        
        Model model;
        // TODO: read other file formats with Model::read_from_file()
        try {
            IO::STL::read(*it, &model);
        } catch (std::exception &e) {
            std::cout << *it << ": " << e.what() << std::endl;
            exit(1);
        }
        
        if (model.objects.empty()) {
            printf("Error: file is empty: %s\n", it->c_str());
            continue;
        }
        
        model.add_default_instances();
        
        // apply command line transform options
        for (ModelObjectPtrs::iterator o = model.objects.begin(); o != model.objects.end(); ++o) {
            if (cli_config.scale_to_fit.is_positive_volume())
                (*o)->scale_to_fit(cli_config.scale_to_fit.value);
            
            // TODO: honor option order?
            (*o)->scale(cli_config.scale.value);
            (*o)->rotate(Geometry::deg2rad(cli_config.rotate_x.value), X);
            (*o)->rotate(Geometry::deg2rad(cli_config.rotate_y.value), Y);
            (*o)->rotate(Geometry::deg2rad(cli_config.rotate.value), Z);
        }
        
        // TODO: handle --merge
        models.push_back(model);
    }
    
    for (std::vector<Model>::iterator model = models.begin(); model != models.end(); ++model) {
        if (cli_config.info) {
            // --info works on unrepaired model
            model->print_info();
        } else if (cli_config.export_obj) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = model->objects.front()->input_file + ".obj";
    
            TriangleMesh mesh = model->mesh();
            mesh.repair();
            IO::OBJ::write(mesh, outfile);
            printf("File exported to %s\n", outfile.c_str());
        } else if (cli_config.export_pov) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = model->objects.front()->input_file + ".pov";
    
            TriangleMesh mesh = model->mesh();
            mesh.repair();
            IO::POV::write(mesh, outfile);
            printf("File exported to %s\n", outfile.c_str());
        } else if (cli_config.export_svg) {
            std::string outfile = cli_config.output.value;
            if (outfile.empty()) outfile = model->objects.front()->input_file + ".svg";
            
            SLAPrint print(&*model);
            print.config.apply(print_config, true);
            print.slice();
            print.write_svg(outfile);
            printf("SVG file exported to %s\n", outfile.c_str());
        } else if (cli_config.cut_x > 0 || cli_config.cut_y > 0 || cli_config.cut > 0) {
            model->repair();
            model->translate(0, 0, -model->bounding_box().min.z);
            
            if (!model->objects.empty()) {
                // FIXME: cut all objects
                Model out;
                if (cli_config.cut_x > 0) {
                    model->objects.front()->cut(X, cli_config.cut_x, &out);
                } else if (cli_config.cut_y > 0) {
                    model->objects.front()->cut(Y, cli_config.cut_y, &out);
                } else {
                    model->objects.front()->cut(Z, cli_config.cut, &out);
                }
                
                ModelObject &upper = *out.objects[0];
                ModelObject &lower = *out.objects[1];
            
                if (upper.facets_count() > 0) {
                    TriangleMesh m = upper.mesh();
                    IO::STL::write(m, upper.input_file + "_upper.stl");
                }
                if (lower.facets_count() > 0) {
                    TriangleMesh m = lower.mesh();
                    IO::STL::write(m, lower.input_file + "_lower.stl");
                }
            }
        } else if (cli_config.cut_grid.value.x > 0 && cli_config.cut_grid.value.y > 0) {
            TriangleMesh mesh = model->mesh();
            mesh.repair();
            
            TriangleMeshPtrs meshes = mesh.cut_by_grid(cli_config.cut_grid.value);
            for (TriangleMeshPtrs::iterator m = meshes.begin(); m != meshes.end(); ++m) {
                std::ostringstream ss;
                ss << model->objects.front()->input_file << "_" << (m - meshes.begin()) << ".stl";
                IO::STL::write(**m, ss.str());
                delete *m;
            }
        } else {
            std::cerr << "error: command not supported" << std::endl;
            return 1;
        }
    }
    
    return 0;
}

#include "Config.hpp"
#include "Model.hpp"
#include "IO.hpp"
#include "TriangleMesh.hpp"
#include "libslic3r.h"

using namespace Slic3r;

void confess_at(const char *file, int line, const char *func, const char *pat, ...){}

int
main(const int argc, const char **argv)
{
    // read config
    ConfigDef config_def;
    {
        ConfigOptionDef* def;
        
        def = config_def.add("offset", coFloat);
        def->label = "Offset from the lowest point (min thickness)";
        def->cli = "offset";
        def->default_value = new ConfigOptionFloat(1);
    
        def = config_def.add("output", coString);
        def->label = "Output File";
        def->tooltip = "The file where the output will be written (if not specified, it will be based on the input file).";
        def->cli = "output";
        def->default_value = new ConfigOptionString("");
    }
    DynamicConfig config(&config_def);
    t_config_option_keys input_files;
    config.read_cli(argc, argv, &input_files);
    
    for (t_config_option_keys::const_iterator it = input_files.begin(); it != input_files.end(); ++it) {
        TriangleMesh mesh;
        Slic3r::IO::STL::read(*it, &mesh);
        calculate_normals(&mesh.stl);
        
        if (mesh.facets_count() == 0) {
            printf("Error: file is empty: %s\n", it->c_str());
            continue;
        }
        
        float z = mesh.stl.stats.min.z - config.option("offset", true)->getFloat();
        printf("min.z = %f, z = %f\n", mesh.stl.stats.min.z, z);
        TriangleMesh mesh2 = mesh;
        
        for (int i = 0; i < mesh.stl.stats.number_of_facets; ++i) {
            const stl_facet &facet = mesh.stl.facet_start[i];
            
            if (facet.normal.z < 0) {
                printf("Invalid 2.5D mesh / TIN (one facet points downwards = %f)\n", facet.normal.z);
                exit(1);
            }
            
            for (int j = 0; j < 3; ++j) {
                if (mesh.stl.neighbors_start[i].neighbor[j] == -1) {
                    stl_facet new_facet;
                    float normal[3];
                    
                    // first triangle
                    new_facet.vertex[0] = new_facet.vertex[2] = facet.vertex[(j+1)%3];
                    new_facet.vertex[1] = facet.vertex[j];
                    new_facet.vertex[2].z = z;
                    stl_calculate_normal(normal, &new_facet);
                    stl_normalize_vector(normal);
                    new_facet.normal.x = normal[0];
                    new_facet.normal.y = normal[1];
                    new_facet.normal.z = normal[2];
                    stl_add_facet(&mesh2.stl, &new_facet);
                    
                    // second triangle
                    new_facet.vertex[0] = new_facet.vertex[1] = facet.vertex[j];
                    new_facet.vertex[2] = facet.vertex[(j+1)%3];
                    new_facet.vertex[1].z = new_facet.vertex[2].z = z;
                    new_facet.normal.x = normal[0];
                    new_facet.normal.y = normal[1];
                    new_facet.normal.z = normal[2];
                    stl_add_facet(&mesh2.stl, &new_facet);
                }
            }
        }
        
        mesh2.repair();
        
        std::string outfile = config.option("output", true)->getString();
        if (outfile.empty()) outfile = *it + "_extruded.stl";
        
        Slic3r::IO::STL::write(mesh2, outfile);
        printf("Extruded mesh written to %s\n", outfile.c_str());
    }
    
    return 0;
}

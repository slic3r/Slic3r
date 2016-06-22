#include "Model.hpp"
#include "IO.hpp"
#include "TriangleMesh.hpp"
#include "SVGExport.hpp"
#include "libslic3r.h"
#include <cstdio>
#include <string>
#include <cstring>
#include <iostream>
#include "tclap/CmdLine.h"

using namespace Slic3r;

void confess_at(const char *file, int line, const char *func, const char *pat, ...){}

void exportSVG(const Model &model, const std::string &outfile, float layerheight=0.2, float initialheight=0., float scale=1.0, float rotate=0.){
    TriangleMesh mesh = model.mesh();
    mesh.scale(scale);
    mesh.rotate_z(rotate);
    mesh.mirror_x();
    mesh.align_to_origin();
    SVGExport e(mesh, layerheight, initialheight);
    e.writeSVG(outfile);
    printf("writing: %s\n", outfile.c_str());
}

int main (int argc, char **argv){
    
    try {
        TCLAP::CmdLine cmd("Rudimentary commandline slic3r, currently only supports STL to SVG slice export.", ' ', SLIC3R_VERSION);
        TCLAP::ValueArg<std::string> outputArg("o","output","File to output results to",false,"","output file name");
        cmd.add( outputArg );
        TCLAP::ValueArg<float> scaleArg("","scale","Factor for scaling input object (default: 1)",false,1.0,"float");
        cmd.add( scaleArg );
        TCLAP::ValueArg<float> rotArg("","rotate","Rotation angle in degrees (0-360, default: 0)",false,0.0,"float");
        cmd.add( rotArg );
        TCLAP::ValueArg<float> flhArg("","first-layer-height","Layer height for first layer in mm (default: 0)",false,0.0,"float");
        cmd.add( flhArg );
        TCLAP::ValueArg<float> lhArg("","layer-height","Layer height in mm (default: 0.2)",false,0.2,"float");
        cmd.add( lhArg );
        TCLAP::SwitchArg expsvg("","export-svg","Export a SVG file containing slices", cmd, false);
        TCLAP::SwitchArg expobj("","export-obj","Export the input file as OBJ", cmd, false);
        
        TCLAP::UnlabeledValueArg<std::string> input("inputfile","Input STL file name", true, "", "input file name");
        cmd.add(input);
        cmd.parse(argc,argv);
        
        // read input file if any (TODO: read multiple)
        Model model;
        if (!input.getValue().empty()) {
            Slic3r::IO::STL::read(input.getValue(), &model);
            model.add_default_instances();
        }
        
        if (expobj.getValue()) {
            std::string outfile = outputArg.getValue();
            if (outfile.empty()) outfile = input.getValue() + ".obj";
            
            TriangleMesh mesh = model.mesh();
            printf("mesh has %zu facets\n", mesh.facets_count());
            Slic3r::IO::OBJ::write(mesh, outfile);
            printf("File exported to %s\n", outfile.c_str());
        } else if (expsvg.getValue()) {
            std::string outfile = outputArg.getValue();
            if (outfile.empty()) outfile = input.getValue() + ".svg";
            
            exportSVG(model, outfile, lhArg.getValue(), flhArg.getValue(), scaleArg.getValue(), rotArg.getValue());
        } else {
            std::cerr << "error: only --export-svg and --export-obj are currently supported"<< std::endl;
            return 1;
        }
        
    } catch (TCLAP::ArgException &e) {
        std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl;
        return 1;
    }
    
    return 0;
}

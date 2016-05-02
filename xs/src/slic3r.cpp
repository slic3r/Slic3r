

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

void exportSVG(const char* stlname, const char* svgname, float layerheight=0.2, float initialheight=0., float scale=1.0, float rotate=0.){
    TriangleMesh t;
    std::string outname;
    if(strlen(svgname)==0){
        outname=outname+stlname+".svg";
    }else{
        outname=outname+svgname;
    }
    t.ReadSTLFile(const_cast<char *>(stlname));
    t.repair();
    t.scale(scale);
    t.rotate_z(rotate);
    t.align_to_origin();
    SVGExport e(t,layerheight,initialheight);
    const char* svgfilename=outname.data();
    e.writeSVG(svgfilename);
    printf("writing: %s\n",svgfilename);
}

int main (int argc, char **argv){
    
    try{
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
        
        TCLAP::UnlabeledValueArg<std::string> input("inputfile","Input STL file name", true, "", "input file name");
        cmd.add(input);
        cmd.parse(argc,argv);
        if(expsvg.getValue()){
            exportSVG(input.getValue().data(),outputArg.getValue().data(),lhArg.getValue(),flhArg.getValue(),scaleArg.getValue(),rotArg.getValue());
        }else{
            std::cerr << "error: only svg export currently supported"<< std::endl;
            return 1;
        }
        
    }catch (TCLAP::ArgException &e) 
	{ std::cerr << "error: " << e.error() << " for arg " << e.argId() << std::endl; }
    
}

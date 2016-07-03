#ifndef slic3r_SVGExport_hpp_
#define slic3r_SVGExport_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"
#include "PrintConfig.hpp"
#include "SVG.hpp"
#include "TriangleMesh.hpp"

namespace Slic3r {

class SVGExport
{
    public:
    SVGExportConfig config;
    
    SVGExport(TriangleMesh &mesh) : mesh(&mesh) {};
    void writeSVG(const std::string &outputfile);
    
    private:
    TriangleMesh* mesh;
};

}

#endif

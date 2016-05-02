#ifndef slic3r_SVGExport_hpp_
#define slic3r_SVGExport_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"
#include "SVG.hpp"
#include "TriangleMesh.hpp"

namespace Slic3r {

class SVGExport
{
    public:
    SVGExport(TriangleMesh &t, float layerheight, float firstlayerheight=0.0);
    void writeSVG(const char* outputfile);
    private:
    TriangleMesh *t;
    std::vector<ExPolygons> layers;
    std::vector<float> heights;
    bool sliced;
};

}

#endif

#include "SVGExport.hpp"
#include "SVG.hpp"
#include <iostream>
#include <cstdio>
#define COORD(x) ((float)unscale(x)*10)

namespace Slic3r {

void
SVGExport::writeSVG(const std::string &outputfile)
{
    std::vector<float> heights;
    for (float z = this->config.first_layer_height.value; z <= this->mesh->stl.stats.max.z;
        z += this->config.layer_height.value)
        heights.push_back(z);

    std::vector<ExPolygons> layers;
    TriangleMeshSlicer(this->mesh).slice(heights, &layers);
    
    FILE* f = fopen(outputfile.c_str(), "w");
    fprintf(f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
        "<svg width=\"%f\" height=\"%f\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:slic3r=\"http://slic3r.org/namespaces/slic3r\">\n"
        "<!-- Generated using Slic3r %s http://slic3r.org/ -->\n"
        , this->mesh->stl.stats.max.x*10, this->mesh->stl.stats.max.y*10, SLIC3R_VERSION);
    
    //<g id="layer0" slic3r:z="0.1"> <path...> <path...> </g>
    
    for (size_t i = 0; i < heights.size(); ++i) {
        fprintf(f, "\t<g id=\"layer%zu\" slic3r:z=\"%0.4f\">\n", i, heights[i]);
        for (ExPolygons::const_iterator it = layers[i].begin(); it != layers[i].end(); ++it){
            std::string pd;
            Polygons pp = *it;
            for (Polygons::const_iterator mp = pp.begin(); mp != pp.end(); ++mp) {
                std::ostringstream d;
                d << "M ";
                for (Points::const_iterator p = mp->points.begin(); p != mp->points.end(); ++p) {
                    d << COORD(p->x) << " ";
                    d << COORD(p->y) << " ";
                }
                d << "z";
                pd += d.str() + " ";
            }
            fprintf(f,"\t\t<path d=\"%s\" style=\"fill: %s; stroke: %s; stroke-width: %s; fill-type: evenodd\" />\n",
                pd.c_str(),"white","black","0"
            );
        }
        fprintf(f,"\t</g>\n");
    }
    fprintf(f,"</svg>\n");
}

}

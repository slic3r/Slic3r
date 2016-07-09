#include "SVGExport.hpp"
#include "ClipperUtils.hpp"
#include <iostream>
#include <cstdio>
#define COORD(x) ((float)unscale(x)*10)

namespace Slic3r {

void
SVGExport::writeSVG(const std::string &outputfile)
{
    // if we are generating a raft, first_layer_height will not affect mesh slicing
    const float lh = this->config.layer_height.value;
    const float first_lh = this->config.first_layer_height.value;
    
    // generate the list of Z coordinates for mesh slicing
    // (we slice each layer at half of its thickness)
    std::vector<float> slice_z, layer_z;
    {
        const float first_slice_lh = (this->config.raft_layers > 0) ? lh : first_lh;
        slice_z.push_back(first_slice_lh/2);
        layer_z.push_back(first_slice_lh);
    }
    while (layer_z.back() + lh/2 <= this->mesh->stl.stats.max.z) {
        slice_z.push_back(layer_z.back() + lh/2);
        layer_z.push_back(layer_z.back() + lh);
    }
    
    // perform the slicing
    std::vector<ExPolygons> layers;
    TriangleMeshSlicer(this->mesh).slice(slice_z, &layers);
    
    // generate a solid raft if requested
    if (this->config.raft_layers > 0) {
        ExPolygons raft = offset_ex(layers.front(), scale_(this->config.raft_offset));
        for (int i = this->config.raft_layers; i >= 1; --i) {
            layer_z.insert(layer_z.begin(), first_lh + lh * (i-1));
            layers.insert(layers.begin(), raft);
        }
        
        // prepend total raft height to all sliced layers
        for (int i = this->config.raft_layers; i < layer_z.size(); ++i)
            layer_z[i] += first_lh + lh * (this->config.raft_layers-1);
    }
    
    FILE* f = fopen(outputfile.c_str(), "w");
    fprintf(f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
        "<svg width=\"%f\" height=\"%f\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:slic3r=\"http://slic3r.org/namespaces/slic3r\">\n"
        "<!-- Generated using Slic3r %s http://slic3r.org/ -->\n"
        , this->mesh->stl.stats.max.x*10, this->mesh->stl.stats.max.y*10, SLIC3R_VERSION);
    
    //<g id="layer0" slic3r:z="0.1"> <path...> <path...> </g>
    
    for (size_t i = 0; i < layer_z.size(); ++i) {
        fprintf(f, "\t<g id=\"layer%zu\" slic3r:z=\"%0.4f\">\n", i, layer_z[i]);
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
            fprintf(f,"\t\t<path d=\"%s\" style=\"fill: %s; stroke: %s; stroke-width: %s; fill-type: evenodd\" slic3r:area=\"%0.4f\" />\n",
                pd.c_str(), "white", "black", "0", unscale(unscale(it->area()))
            );
        }
        fprintf(f,"\t</g>\n");
    }
    fprintf(f,"</svg>\n");
}

}

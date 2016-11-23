#include "SLAPrint.hpp"
#include "ClipperUtils.hpp"
#include <iostream>
#include <cstdio>

namespace Slic3r {

void
SLAPrint::slice()
{
    TriangleMesh mesh = this->model->mesh();
    mesh.repair();
    
    // align to origin taking raft into account
    this->bb = mesh.bounding_box();
    if (this->config.raft_layers > 0) {
        this->bb.min.x -= this->config.raft_offset.value;
        this->bb.min.y -= this->config.raft_offset.value;
        this->bb.max.x += this->config.raft_offset.value;
        this->bb.max.y += this->config.raft_offset.value;
    }
    mesh.translate(0, 0, -bb.min.z);
    this->bb.translate(0, 0, -bb.min.z);
    
    // if we are generating a raft, first_layer_height will not affect mesh slicing
    const float lh       = this->config.layer_height.value;
    const float first_lh = this->config.first_layer_height.value;
    
    // generate the list of Z coordinates for mesh slicing
    // (we slice each layer at half of its thickness)
    this->layers.clear();
    {
        const float first_slice_lh = (this->config.raft_layers > 0) ? lh : first_lh;
        this->layers.push_back(Layer(first_slice_lh/2, first_slice_lh));
    }
    while (this->layers.back().print_z + lh/2 <= mesh.stl.stats.max.z) {
        this->layers.push_back(Layer(this->layers.back().print_z + lh/2, this->layers.back().print_z + lh));
    }
    
    // perform slicing and generate layers
    {
        std::vector<float> slice_z;
        for (size_t i = 0; i < this->layers.size(); ++i)
            slice_z.push_back(this->layers[i].slice_z);
        
        std::vector<ExPolygons> slices;
        TriangleMeshSlicer(&mesh).slice(slice_z, &slices);
        
        for (size_t i = 0; i < slices.size(); ++i)
            this->layers[i].slices = ExPolygonCollection(slices[i]);
    }
    
    // generate support material
    this->sm_pillars.clear();
    ExPolygons overhangs;
    if (this->config.support_material) {
        // flatten and merge all the overhangs
        {
            Polygons pp;
            for (std::vector<Layer>::const_iterator it = this->layers.begin()+1; it != this->layers.end(); ++it) {
                Polygons oh = diff(it->slices, (it - 1)->slices);
                pp.insert(pp.end(), oh.begin(), oh.end());
            }
            overhangs = union_ex(pp);
        }
        
        // generate points following the shape of each island
        Points pillars_pos;
        const coordf_t spacing = scale_(this->config.support_material_spacing);
        const coordf_t radius  = scale_(this->sm_pillars_radius());
        for (ExPolygons::const_iterator it = overhangs.begin(); it != overhangs.end(); ++it) {
            // leave a radius/2 gap between pillars and contour to prevent lateral adhesion
            for (float inset = radius * 1.5;; inset += spacing) {
                // inset according to the configured spacing
                Polygons curr = offset(*it, -inset);
                if (curr.empty()) break;
                
                // generate points along the contours
                for (Polygons::const_iterator pg = curr.begin(); pg != curr.end(); ++pg) {
                    Points pp = pg->equally_spaced_points(spacing);
                    for (Points::const_iterator p = pp.begin(); p != pp.end(); ++p)
                        pillars_pos.push_back(*p);
                }
            }
        }
        
        // for each pillar, check which layers it applies to
        for (Points::const_iterator p = pillars_pos.begin(); p != pillars_pos.end(); ++p) {
            SupportPillar pillar(*p);
            bool object_hit = false;
            
            // check layers top-down
            for (int i = this->layers.size()-1; i >= 0; --i) {
                // check whether point is void in this layer
                if (!this->layers[i].slices.contains(*p)) {
                    // no slice contains the point, so it's in the void
                    if (pillar.top_layer > 0) {
                        // we have a pillar, so extend it
                        pillar.bottom_layer = i + this->config.raft_layers;
                    } else if (object_hit) {
                        // we don't have a pillar and we're below the object, so create one
                        pillar.top_layer = i + this->config.raft_layers;
                    }
                } else {
                    if (pillar.top_layer > 0) {
                        // we have a pillar which is not needed anymore, so store it and initialize a new potential pillar
                        this->sm_pillars.push_back(pillar);
                        pillar = SupportPillar(*p);
                    }
                    object_hit = true;
                }
            }
            if (pillar.top_layer > 0) this->sm_pillars.push_back(pillar);
        }
    }
    
    // generate a solid raft if requested
    // (do this after support material because we take support material shape into account)
    if (this->config.raft_layers > 0) {
        ExPolygons raft = this->layers.front().slices;
        raft.insert(raft.end(), overhangs.begin(), overhangs.end()); // take support material into account
        raft = offset_ex(raft, scale_(this->config.raft_offset));
        for (int i = this->config.raft_layers; i >= 1; --i) {
            this->layers.insert(this->layers.begin(), Layer(0, first_lh + lh * (i-1)));
            this->layers.front().slices = raft;
        }
        
        // prepend total raft height to all sliced layers
        for (int i = this->config.raft_layers; i < this->layers.size(); ++i)
            this->layers[i].print_z += first_lh + lh * (this->config.raft_layers-1);
    }
}

void
SLAPrint::write_svg(const std::string &outputfile) const
{
    const Sizef3 size = this->bb.size();
    const double support_material_radius = sm_pillars_radius();
    
    FILE* f = fopen(outputfile.c_str(), "w");
    fprintf(f,
        "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>\n"
        "<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.0//EN\" \"http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd\">\n"
        "<svg width=\"%f\" height=\"%f\" xmlns=\"http://www.w3.org/2000/svg\" xmlns:svg=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\" xmlns:slic3r=\"http://slic3r.org/namespaces/slic3r\" viewport-fill=\"black\">\n"
        "<!-- Generated using Slic3r %s http://slic3r.org/ -->\n"
        , size.x, size.y, SLIC3R_VERSION);
    
    for (size_t i = 0; i < this->layers.size(); ++i) {
        const Layer &layer = this->layers[i];
        fprintf(f,
            "\t<g id=\"layer%zu\" slic3r:z=\"%0.4f\" slic3r:slice-z=\"%0.4f\" slic3r:layer-height=\"%0.4f\">\n", i,
            layer.print_z,
            layer.slice_z,
            layer.print_z - (i == 0 ? 0 : this->layers[i-1].print_z)
        );
        
        const ExPolygons &slices = layer.slices.expolygons;
        for (ExPolygons::const_iterator it = slices.begin(); it != slices.end(); ++it) {
            std::string pd;
            Polygons pp = *it;
            for (Polygons::const_iterator mp = pp.begin(); mp != pp.end(); ++mp) {
                std::ostringstream d;
                d << "M ";
                for (Points::const_iterator p = mp->points.begin(); p != mp->points.end(); ++p) {
                    d << unscale(p->x) - this->bb.min.x << " ";
                    d << size.y - (unscale(p->y) - this->bb.min.y) << " ";  // mirror Y coordinates as SVG uses downwards Y
                }
                d << "z";
                pd += d.str() + " ";
            }
            fprintf(f,"\t\t<path d=\"%s\" style=\"fill: %s; stroke: %s; stroke-width: %s; fill-type: evenodd\" slic3r:area=\"%0.4f\" />\n",
                pd.c_str(), "white", "black", "0", unscale(unscale(it->area()))
            );
        }
        
        // don't print support material in raft layers
        if (i >= this->config.raft_layers) {
            // look for support material pillars belonging to this layer
            for (std::vector<SupportPillar>::const_iterator it = this->sm_pillars.begin(); it != this->sm_pillars.end(); ++it) {
                if (!(it->top_layer >= i && it->bottom_layer <= i)) continue;
            
                // generate a conic tip
                float radius = fminf(
                    support_material_radius,
                    (it->top_layer - i + 1) * this->config.layer_height.value
                );
            
                fprintf(f,"\t\t<circle cx=\"%f\" cy=\"%f\" r=\"%f\" stroke-width=\"0\" fill=\"white\" slic3r:type=\"support\" />\n",
                    unscale(it->x) - this->bb.min.x,
                    size.y - (unscale(it->y) - this->bb.min.y),
                    radius
                );
            }
        }
        
        fprintf(f,"\t</g>\n");
    }
    fprintf(f,"</svg>\n");
}

coordf_t
SLAPrint::sm_pillars_radius() const
{
    coordf_t radius = this->config.support_material_extrusion_width.get_abs_value(this->config.support_material_spacing)/2;
    if (radius == 0) radius = this->config.support_material_spacing / 3; // auto
    return radius;
}

}

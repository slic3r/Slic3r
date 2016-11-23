#ifndef slic3r_SLAPrint_hpp_
#define slic3r_SLAPrint_hpp_

#include "libslic3r.h"
#include "ExPolygon.hpp"
#include "ExPolygonCollection.hpp"
#include "Model.hpp"
#include "Point.hpp"
#include "PrintConfig.hpp"
#include "SVG.hpp"

namespace Slic3r {

class SLAPrint
{
    public:
    SLAPrintConfig config;
    
    class Layer {
        public:
        ExPolygonCollection slices;
        float slice_z, print_z;
        
        Layer(float _slice_z, float _print_z) : slice_z(_slice_z), print_z(_print_z) {};
    };
    std::vector<Layer> layers;
    
    class SupportPillar : public Point {
        public:
        size_t top_layer, bottom_layer;
        SupportPillar(const Point &p) : Point(p), top_layer(0), bottom_layer(0) {};
    };
    std::vector<SupportPillar> sm_pillars;
    
    SLAPrint(Model* _model) : model(_model) {};
    void slice();
    void write_svg(const std::string &outputfile) const;
    
    private:
    Model* model;
    BoundingBoxf3 bb;
    
    coordf_t sm_pillars_radius() const;
};

}

#endif

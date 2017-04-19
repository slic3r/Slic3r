#ifndef slic3r_Flow_hpp_
#define slic3r_Flow_hpp_

#include "libslic3r.h"
#include "Config.hpp"
#include "ExtrusionEntity.hpp"

namespace Slic3r {

constexpr auto BRIDGE_EXTRA_SPACING = 0.05;
constexpr auto OVERLAP_FACTOR = 1.0;

enum FlowRole {
    frExternalPerimeter,
    frPerimeter,
    frInfill,
    frSolidInfill,
    frTopSolidInfill,
    frSupportMaterial,
    frSupportMaterialInterface,
};

class Flow
{
    public:
    float width, height, nozzle_diameter;
    bool bridge;
    
    Flow(float _w, float _h, float _nd, bool _bridge = false)
        : width(_w), height(_h), nozzle_diameter(_nd), bridge(_bridge) {};
    float spacing() const;
    float spacing(const Flow &other) const;
    void set_spacing(float spacing);
    void set_solid_spacing(const coord_t total_width) {
        this->set_spacing(Flow::solid_spacing(total_width, this->scaled_spacing()));
    };
    void set_solid_spacing(const coordf_t total_width) {
        this->set_spacing(Flow::solid_spacing(total_width, (coordf_t)this->spacing()));
    };
    double mm3_per_mm() const;
    coord_t scaled_width() const {
        return scale_(this->width);
    };
    coord_t scaled_spacing() const {
        return scale_(this->spacing());
    };
    coord_t scaled_spacing(const Flow &other) const {
        return scale_(this->spacing(other));
    };
    
    static Flow new_from_config_width(FlowRole role, const ConfigOptionFloatOrPercent &width, float nozzle_diameter, float height, float bridge_flow_ratio);
    static Flow new_from_spacing(float spacing, float nozzle_diameter, float height, bool bridge);
    template <class T> static T solid_spacing(const T total_width, const T spacing);
    
    private:
    static float _bridge_width(float nozzle_diameter, float bridge_flow_ratio);
    static float _auto_width(FlowRole role, float nozzle_diameter, float height);
    static float _width_from_spacing(float spacing, float nozzle_diameter, float height, bool bridge);
};

}

#endif

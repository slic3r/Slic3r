#ifndef slic3r_SimplePrint_hpp_
#define slic3r_SimplePrint_hpp_

#include "libslic3r.h"
#include "Point.hpp"
#include "Print.hpp"

namespace Slic3r {

class SimplePrint {
    public:
    bool arrange{true};
    bool center{true};
    std::function<void(int, const std::string&)> status_cb {nullptr};
    
    bool apply_config(DynamicPrintConfig config) { return this->_print.apply_config(config); }
    double total_used_filament() const { return this->_print.total_used_filament; }
    double total_extruded_volume() const { return this->_print.total_extruded_volume; }
    void set_model(const Model &model);
    void export_gcode(std::string outfile);
    const Model& model() const { return this->_model; };
    const Print& print() const { return this->_print; };
    
    private:
    Model _model;
    Print _print;
};

}

#endif

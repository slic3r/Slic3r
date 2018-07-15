#ifndef SLIC3RXS
#ifndef slic3r_PrintGCode_hpp
#define slic3r_PrintGCode_hpp

#include "GCode.hpp"
#include "GCode/CoolingBuffer.hpp"
#include "GCode/SpiralVase.hpp"
#include "Geometry.hpp"
#include "Flow.hpp"
#include "ExtrusionEntity.hpp"
#include "libslic3r.h"

#include <string>
#include <iostream>

namespace Slic3r {

class PrintGCode {
public:
    PrintGCode(Slic3r::Print& print, std::ostream& _fh) : 
        _print(print), 
        config(print.config), 
        _gcodegen(Slic3r::GCode()),
        objects(print.objects),
        fh(_fh),
        _cooling_buffer(Slic3r::CoolingBuffer(this->_gcodegen)),
        _spiral_vase(Slic3r::SpiralVase(this->config))
        { };

    /// Perform the export. export is a reserved name in C++, so changed to output
    void output();

    void flush_filters() { fh << this->filter(this->_cooling_buffer.flush(), 1); }

    /// Applies various filters, if enabled.
    std::string filter(const std::string& in, bool wait);

private:
    
    Slic3r::Print& _print;
    const Slic3r::PrintConfig& config;

    Slic3r::GCode _gcodegen;

    const PrintObjectPtrs& objects;

    std::ostream& fh;

    Slic3r::CoolingBuffer _cooling_buffer;
    Slic3r::SpiralVase _spiral_vase;
//    Slic3r::VibrationLimit _vibration_limit;
//    Slic3r::ArcFitting _arc_fitting;
//    Slic3r::PressureRegulator _pressure_regulator;

    size_t _skirt_done {0};
    bool _brim_done {false};
    bool _second_layer_things_done {false};
    std::string _last_obj_copy {""};
    bool _autospeed {false};

    void _print_first_layer_temperature(bool wait);
    void _print_off_temperature(bool wait);

};

} // namespace Slic3r

#endif // slic3r_PrintGCode_hpp
#endif // SLIC3RXS

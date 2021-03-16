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
#include <regex>

namespace Slic3r {

class PrintGCode {
public:
    /// Constructor.
    PrintGCode(Slic3r::Print& print, std::ostream& _fh);

    /// Perform the export. export is a reserved name in C++, so changed to output
    void output();

    /// Process an individual output for export. Writes to the ostream.
    void process_layer(size_t idx, const Layer* layer, const Points& copies);

    void flush_filters() { fh << this->filter(this->_cooling_buffer.flush(), true); }

    /// Applies various filters, if enabled.
    std::string filter(const std::string& in, bool wait = false);

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

    /// presence in the array indicates that the
    std::map<coord_t, bool> _skirt_done {};
    bool _brim_done {false};
    bool _second_layer_things_done {false};
    std::pair<Point, bool> _last_obj_copy {std::pair<Point, bool>(Point(), false)};
    bool _autospeed {false};

    void _print_first_layer_temperature(bool wait);
    void _print_off_temperature(bool wait);

    /// Utility function to print config options as gcode comments
    void _print_config(const ConfigBase& config);

    // Extrude perimeters: Decide where to put seams (hide or align seams).
    std::string _extrude_perimeters(std::map<size_t,ExtrusionEntityCollection> &by_region);

    // Chain the paths hierarchically by a greedy algorithm to minimize a travel distance.
    std::string _extrude_infill(std::map<size_t,ExtrusionEntityCollection> &by_region);

    /// regular expression to match heater gcodes
    std::regex bed_temp_regex { std::regex("M(?:190|140)", std::regex_constants::icase)};
    /// regular expression to match heater gcodes
    std::regex ex_temp_regex { std::regex("M(?:109|104)", std::regex_constants::icase)};
};

} // namespace Slic3r

#endif // slic3r_PrintGCode_hpp

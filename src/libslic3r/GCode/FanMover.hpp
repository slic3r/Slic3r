#ifndef slic3r_GCode_FanMover_hpp_
#define slic3r_GCode_FanMover_hpp_


#include "../libslic3r.h"
#include "../PrintConfig.hpp"
#include "../ExtrusionEntity.hpp"

#include "../Point.hpp"
#include "../GCodeReader.hpp"
#include <regex>

namespace Slic3r {

class BufferData {
public:
    std::string raw;
    float time;
    float fan_speed;
    float x = 0, y = 0, z = 0, e = 0;
    float dx = 0, dy = 0, dz = 0, de = 0;
    BufferData(std::string line, float time = 0, float fan_speed = 0) : raw(line), time(time), fan_speed(fan_speed) {}
};

class FanMover
{
private:
    const std::regex regex_fan_speed;
    const float nb_seconds_delay;
    const bool with_D_option;
    const bool relative_e;

    // in unit/second
    double current_speed = 1000 / 60.0;;
    float buffer_time_size = 0;
    GCodeReader m_parser{};
    int expected_fan_speed = 0;
    int current_fan_speed = 0;

    // The output of process_layer()
    std::list<BufferData> buffer;
    std::string m_process_output;

public:
    FanMover(const float nb_seconds_delay, const bool with_D_option, const bool relative_e)
        : regex_fan_speed("S[0-9]+"), nb_seconds_delay(nb_seconds_delay), with_D_option(with_D_option), relative_e(relative_e){}

    // Adds the gcode contained in the given string to the analysis and returns it after removing the workcodes
    const std::string& process_gcode(const std::string& gcode, bool flush);

private:
    // Processes the given gcode line
    void _process_gcode_line(GCodeReader& reader, const GCodeReader::GCodeLine& line);
};

} // namespace Slic3r


#endif /* slic3r_GCode_FanMover_hpp_ */

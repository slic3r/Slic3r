#include "FanMover.hpp"

#include "GCodeReader.hpp"

/*
#include <memory.h>
#include <string.h>
#include <float.h>

#include "../libslic3r.h"
#include "../PrintConfig.hpp"
#include "../Utils.hpp"
#include "Print.hpp"

#include <boost/log/trivial.hpp>
*/


namespace Slic3r {

const std::string& FanMover::process_gcode(const std::string& gcode, bool flush)
{
    m_process_output = "";

    m_parser.parse_buffer(gcode,
        [this](GCodeReader& reader, const GCodeReader::GCodeLine& line) { /*m_process_output += line.raw() + "\n";*/ this->_process_gcode_line(reader, line); });

    if (flush) {
        while (!buffer.empty()) {
            m_process_output += buffer.back().raw + "\n";
            buffer.pop_back();
        }
    }

    return m_process_output;
}

bool is_end_of_word(char c) {
   return c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == 0;
}

float get_axis_value(const std::string& line, char axis)
{
    char match[3] = " X";
    match[1] = axis;

    size_t pos = line.find(match) + 2;
    size_t end = line.find(' ', pos + 1);
    // Try to parse the numeric value.
    const char* c = line.c_str();
    char* pend = nullptr;
    double  v = strtod(c+ pos, &pend);
    if (pend != nullptr && is_end_of_word(*pend)) {
        // The axis value has been parsed correctly.
        return float(v);
    }
    return NAN;
}

void change_axis_value(std::string& line, char axis, const float new_value, const int decimal_digits)
{

    std::ostringstream ss;
    ss << std::fixed << std::setprecision(decimal_digits) << new_value;

    char match[3] = " X";
    match[1] = axis;

    size_t pos = line.find(match) + 2;
    size_t end = line.find(' ', pos + 1);
    line = line.replace(pos, end - pos, ss.str());
}

void FanMover::_process_gcode_line(GCodeReader& reader, const GCodeReader::GCodeLine& line)
{
    // processes 'normal' gcode lines
    std::string cmd = line.cmd();
    double time = 0;
    float fan_speed = -1;
    if (cmd.length() > 1) {
        if (line.has_f())
            current_speed = line.f() / 60.0f;
        switch (::toupper(cmd[0])) {
        case 'G':
        {
            if (::atoi(&cmd[1]) == 1 || ::atoi(&cmd[1]) == 0) {
                double distx = line.dist_X(reader);
                double disty = line.dist_Y(reader);
                double distz = line.dist_Z(reader);
                double dist = distx * distx + disty * disty + distz * distz;
                if (dist > 0) {
                    dist = std::sqrt(dist);
                    time = dist / current_speed;
                }
            }
            break;
        }
        case 'M':
        {
            if (::atoi(&cmd[1]) == 106) {
                if (line.has_value('S', fan_speed) ) {
                    int nb_M106_erased = 0;
                    if (fan_speed > expected_fan_speed) {
                        time = -1; // don't write!
                        buffer.emplace_front(BufferData("; erased: "+line.raw(), 0, -1));
                        //erase M106 in the buffer -> don't slowdown if you are in the process of step-up.
                        auto it = buffer.begin();
                        int i = 0;
                        while (it != buffer.end()) {
                            if (it->raw.compare(0, 4, "M106") == 0 && it->fan_speed < fan_speed) {
                                //found something that is lower than us -> change is speed by ours and delete us
                                it->fan_speed = fan_speed;
                                std::stringstream ss; ss << "S" << (int)fan_speed;
                                it->raw = std::regex_replace(it->raw, regex_fan_speed, ss.str());
                                nb_M106_erased++;
                            } else {
                                ++it;
                                i++;
                            }
                        }

                        if (nb_M106_erased == 0) {
                            //try to split the G1/G0 line to increae precision
                            if (!buffer.empty()) {
                                BufferData& backdata = buffer.back();
                                if (buffer_time_size > nb_seconds_delay * 1.1f && backdata.raw.size() > 2
                                    && backdata.raw[0] == 'G' && backdata.raw[1] == '1' && backdata.raw[2] == ' ') {
                                    float percent = (buffer_time_size - nb_seconds_delay) / backdata.time;
                                    std::string before = backdata.raw;
                                    std::string& after = backdata.raw;
                                    if (backdata.dx != 0) {
                                        change_axis_value(before, 'X', backdata.x + backdata.dx * percent, 3);
                                    }
                                    if (backdata.dy != 0) {
                                        change_axis_value(before, 'Y', backdata.y + backdata.dy * percent, 3);
                                    }
                                    if (backdata.dz != 0) {
                                        change_axis_value(before, 'Z', backdata.z + backdata.dz * percent, 3);
                                    }
                                    if (backdata.de != 0) {
                                        if (relative_e) {
                                            change_axis_value(before, 'E', backdata.de * percent, 5);
                                            change_axis_value(after, 'E', backdata.de * (1 - percent), 5);
                                        } else {
                                            change_axis_value(before, 'E', backdata.e + backdata.de * percent, 5);
                                            change_axis_value(after, 'E', backdata.e + backdata.de * (1 - percent), 5);
                                        }
                                    }
                                    m_process_output += before + "\n";
                                    buffer_time_size -= backdata.time * percent;
                                    backdata.time -= backdata.time * percent;

                                }
                            }

                            //print it
                            if (with_D_option) {
                                std::stringstream ss;
                                ss << " D" << (uint32_t)(buffer_time_size * 1000) << "\n";
                                m_process_output += line.raw() + ss.str();
                            } else {
                                m_process_output += line.raw() + "\n";
                            }
                            current_fan_speed = fan_speed;
                        }
                    }

                    //update
                    expected_fan_speed = fan_speed;
                }
            }
            break;
        }
        }
    }

    if (time >= 0) {
        buffer.emplace_front(BufferData(line.raw(), time, fan_speed));
        BufferData& front = buffer.front();
        if (line.has(Axis::X)) {
            front.x = reader.x();
            front.dx = line.dist_X(reader);
        }
        if (line.has(Axis::Y)) {
            front.y = reader.y();
            front.dy = line.dist_Y(reader);
        }
        if (line.has(Axis::Z)) {
            front.z = reader.z();
            front.dz = line.dist_Z(reader);
        }
        if (line.has(Axis::E)) {
            front.e = reader.e();
            if(relative_e)
                front.de = line.e();
            else
                front.de = line.dist_E(reader);
        }
        buffer_time_size += time;
    }
    // puts the line back into the gcode
    //if buffer too big, flush it.
    if (time > 0) {
        while (buffer_time_size - buffer.back().time > nb_seconds_delay) {
            BufferData &backdata = buffer.back();
            if (backdata.fan_speed < 0 || (int)backdata.fan_speed != (int)current_fan_speed) {
                buffer_time_size -= backdata.time;
                m_process_output += backdata.raw + "\n";
                if (backdata.fan_speed >= 0) {
                    current_fan_speed = backdata.fan_speed;
                }
            }
            buffer.pop_back();
        }
    }
}

} // namespace Slic3r


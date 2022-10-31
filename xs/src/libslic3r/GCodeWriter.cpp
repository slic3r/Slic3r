#include "GCodeWriter.hpp"
#include "utils.hpp"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>

#define FLAVOR_IS(val) this->config.gcode_flavor == val
#define FLAVOR_IS_NOT(val) this->config.gcode_flavor != val
#define COMMENT(comment) if (this->config.gcode_comments && !comment.empty()) gcode << " ; " << comment;
#define PRECISION(val, precision) std::fixed << std::setprecision(precision) << val
#define XYZF_NUM(val) PRECISION(val, 3)
#define E_NUM(val) PRECISION(val, 5)

namespace Slic3r {

void
GCodeWriter::apply_print_config(const PrintConfig &print_config)
{
    this->config.apply(print_config, true);
    this->_extrusion_axis = this->config.get_extrusion_axis();
}

std::string
GCodeWriter::notes() 
{
    std::ostringstream gcode;

    // Write the contents of the three notes sections
    // a semicolon at the beginning of each line.
    if (this->config.notes.getString().size() > 0) {
        gcode << "; Print Config Notes: \n";
        std::vector<std::string> temp_line = split_at_regex(this->config.notes.getString(),"\n");
        for (auto j = temp_line.cbegin(); j != temp_line.cend(); j++) {
            gcode << "; " << *j << "\n";
        }
        gcode << "; \n";
    }

    for (auto i = this->config.filament_notes.values.cbegin(); i != this->config.filament_notes.values.cend(); i++) {
        if (i->size() > 0) {
            gcode << "; Filament notes: \n";
            std::vector<std::string> temp_line = split_at_regex(*i,"\n");
            for (auto j = temp_line.cbegin(); j != temp_line.cend(); j++) {
                gcode << "; " << *j << "\n";
            }
            gcode << "; \n";
        }
    }

    if (this->config.printer_notes.getString().size() > 0) {
        gcode << "; Printer Config Notes: \n";
        std::vector<std::string> temp_line = split_at_regex(this->config.printer_notes.getString(),"\n");
        for (auto j = temp_line.cbegin(); j != temp_line.cend(); j++) {
            gcode << "; " << *j << "\n";
        }
        gcode << "; \n";
    }

    return gcode.str();
}


std::string
GCodeWriter::preamble()
{
    std::ostringstream gcode;

    if (FLAVOR_IS_NOT(gcfMakerWare)) {
        if (FLAVOR_IS_NOT(gcfM3dMicro)) {
            gcode << "G21 ; set units to millimeters\n";
        }
        gcode << "G90 ; use absolute coordinates\n";
    }
    if (FLAVOR_IS(gcfRepRap) || FLAVOR_IS(gcfTeacup) || FLAVOR_IS(gcfRepetier) || FLAVOR_IS(gcfSmoothie)) {
        if (this->config.use_relative_e_distances) {
            gcode << "M83 ; use relative distances for extrusion\n";
        } else {
            gcode << "M82 ; use absolute distances for extrusion\n";
        }
        gcode << this->reset_e(true);
    }


    return gcode.str();
}

std::string
GCodeWriter::postamble() const
{
    std::ostringstream gcode;
    if (FLAVOR_IS(gcfMachinekit))
          gcode << "M2 ; end of program\n";
    return gcode.str();
}

std::string
GCodeWriter::set_temperature(unsigned int temperature, bool wait, int tool) const
{
    wait = this->config.use_set_and_wait_extruder ? true : wait;
    std::string code, comment;
    if (wait && FLAVOR_IS_NOT(gcfTeacup) && FLAVOR_IS_NOT(gcfMakerWare) && FLAVOR_IS_NOT(gcfSailfish)) {
        code = "M109";
        comment = "set temperature and wait for it to be reached";
    } else {
        code = "M104";
        comment = "set temperature";
    }
    
    std::ostringstream gcode;
    gcode << code << " ";
    if (FLAVOR_IS(gcfMach3) || FLAVOR_IS(gcfMachinekit)) {
        gcode << "P";
    } else {
        gcode << "S";
    }
    gcode << temperature;
    if (tool != -1 && (this->multiple_extruders || FLAVOR_IS(gcfMakerWare) || FLAVOR_IS(gcfSailfish))) {
        gcode << " T" << tool;
    }
    gcode << " ; " << comment << "\n";
    
    if (FLAVOR_IS(gcfTeacup) && wait)
        gcode << "M116 ; wait for temperature to be reached\n";
    
    if (wait && tool !=-1 && (FLAVOR_IS(gcfMakerWare) || FLAVOR_IS(gcfSailfish)))
        gcode << "M6 T" << tool << " ; wait for temperature to be reached\n";
    
    return gcode.str();
}

std::string
GCodeWriter::set_bed_temperature(unsigned int temperature, bool wait) const
{
    std::string code, comment;
    wait = this->config.use_set_and_wait_bed ? true : wait;
    if (wait && FLAVOR_IS_NOT(gcfTeacup)) {
        if (FLAVOR_IS(gcfMakerWare) || FLAVOR_IS(gcfSailfish)) {
            code = "M109";
        } else {
            code = "M190";
        }
        comment = "set bed temperature and wait for it to be reached";
    } else {
        code = "M140";
        comment = "set bed temperature";
    }
    
    std::ostringstream gcode;
    gcode << code << " ";
    if (FLAVOR_IS(gcfMach3) || FLAVOR_IS(gcfMachinekit)) {
        gcode << "P";
    } else {
        gcode << "S";
    }
    gcode << temperature << " ; " << comment << "\n";
    
    if (FLAVOR_IS(gcfTeacup) && wait)
        gcode << "M116 ; wait for bed temperature to be reached\n";
    
    return gcode.str();
}

std::string
GCodeWriter::set_fan(unsigned int speed, bool dont_save)
{
    std::ostringstream gcode;
    const double baseline_factor = (this->config.fan_percentage ? 100.0 : 255.0);
    if (this->_last_fan_speed != speed || dont_save) {
        if (!dont_save) this->_last_fan_speed = speed;
        
        if (speed == 0) {
            if (FLAVOR_IS(gcfTeacup)) {
                gcode << "M106 S0";
            } else if (FLAVOR_IS(gcfMakerWare) || FLAVOR_IS(gcfSailfish)) {
                gcode << "M127";
            } else {
                gcode << "M107";
            }
            if (this->config.gcode_comments) gcode << " ; disable fan";
            gcode << "\n";
        } else {
            if (FLAVOR_IS(gcfMakerWare) || FLAVOR_IS(gcfSailfish)) {
                gcode << "M126";
            } else {
                gcode << "M106 ";
                if (FLAVOR_IS(gcfMach3) || FLAVOR_IS(gcfMachinekit)) {
                    gcode << "P";
                } else {
                    gcode << "S";
                }
                gcode << (baseline_factor * speed / 100.0);
            }
            if (this->config.gcode_comments) gcode << " ; enable fan";
            gcode << "\n";
        }
    }
    return gcode.str();
}

std::string
GCodeWriter::set_acceleration(unsigned int acceleration)
{
    if (acceleration == 0 || acceleration == this->_last_acceleration)
        return "";
    
    this->_last_acceleration = acceleration;
    
    std::ostringstream gcode;
    if (FLAVOR_IS(gcfRepetier) || (FLAVOR_IS(gcfRepRap))) {
        gcode << "M201 X" << acceleration << " Y" << acceleration;
        if (this->config.gcode_comments) gcode << " ; adjust acceleration";
        gcode << "\n";
    }
    if (FLAVOR_IS(gcfRepetier)) {
        gcode << "M202 X" << acceleration << " Y" << acceleration;
    } else if (FLAVOR_IS(gcfRepRap)) {
        gcode << "M204 P" << acceleration << " T" << acceleration;
    } else {
        gcode << "M204 S" << acceleration;
    }
    if (this->config.gcode_comments) gcode << " ; adjust acceleration";
    gcode << "\n";
    
    return gcode.str();
}

std::string
GCodeWriter::reset_e(bool force)
{
    if (FLAVOR_IS(gcfMach3)
        || FLAVOR_IS(gcfMakerWare)
        || FLAVOR_IS(gcfSailfish))
        return "";
    
    if (this->_extruder != NULL) {
        if (this->_extruder->E == 0 && !force) return "";
        this->_extruder->E = 0;
    }
    
    if (!this->_extrusion_axis.empty() && !this->config.use_relative_e_distances) {
        std::ostringstream gcode;
        gcode << "G92 " << this->_extrusion_axis << "0";
        if (this->config.gcode_comments) gcode << " ; reset extrusion distance";
        gcode << "\n";
        return gcode.str();
    } else {
        return "";
    }
}

std::string
GCodeWriter::update_progress(unsigned int num, unsigned int tot, bool allow_100) const
{
    if (FLAVOR_IS_NOT(gcfMakerWare) && FLAVOR_IS_NOT(gcfSailfish))
        return "";
    
    unsigned int percent = 100.0 * num / tot;
    if (!allow_100) percent = std::min(percent, (unsigned int)99);
    
    std::ostringstream gcode;
    gcode << "M73 P" << percent;
    if (this->config.gcode_comments) gcode << " ; update progress";
    gcode << "\n";
    return gcode.str();
}

bool
GCodeWriter::need_toolchange(unsigned int extruder_id) const
{
    // return false if this extruder was already selected
    return (this->_extruder == NULL) || (this->_extruder->id != extruder_id);
}

std::string
GCodeWriter::set_extruder(unsigned int extruder_id)
{
    if (!this->need_toolchange(extruder_id)) return "";
    return this->toolchange(extruder_id);
}

std::string
GCodeWriter::toolchange(unsigned int extruder_id)
{
    std::ostringstream gcode;
    
    // set the new extruder
    this->_extruder = &this->extruders.find(extruder_id)->second;
    
    //first thing to do : reset E (because a new item is now printed or with a new extruder)
    gcode << this->reset_e(true);
    
    // return the toolchange command
    // if we are running a single-extruder setup, just set the extruder and return nothing
    if (this->multiple_extruders) {
        if (FLAVOR_IS(gcfMakerWare)) {
            gcode << "M135 T";
        } else if (FLAVOR_IS(gcfSailfish)) {
            gcode << "M108 T";
        } else {
            gcode << "T";
        }
        gcode << extruder_id;
        if (this->config.gcode_comments) gcode << " ; change extruder";
        gcode << "\n";
    }
    return gcode.str();
}

std::string
GCodeWriter::set_speed(double F, const std::string &comment,
                       const std::string &cooling_marker) const
{
    std::ostringstream gcode;
    gcode.precision(3);
    gcode << "G1 F" << std::fixed << F;
    COMMENT(comment);
    gcode << cooling_marker;
    gcode << "\n";
    return gcode.str();
}

std::string
GCodeWriter::travel_to_xy(const Pointf &point, const std::string &comment)
{
    this->_pos.x = point.x;
    this->_pos.y = point.y;
    
    std::ostringstream gcode;
    gcode << "G1 X" << XYZF_NUM(point.x)
          <<   " Y" << XYZF_NUM(point.y)
          <<   " F" << XYZF_NUM(this->config.travel_speed.value * 60.0);
    COMMENT(comment);
    gcode << "\n";
    return gcode.str();
}

std::string
GCodeWriter::travel_to_xyz(const Pointf3 &point, const std::string &comment)
{
    /*  If target Z is lower than current Z but higher than nominal Z we
        don't perform the Z move but we only move in the XY plane and
        adjust the nominal Z by reducing the lift amount that will be 
        used for unlift. */
    if (!this->will_move_z(point.z)) {
        double nominal_z = this->_pos.z - this->_lifted;
        this->_lifted = this->_lifted - (point.z - nominal_z);
        return this->travel_to_xy(point);
    }
    
    /*  In all the other cases, we perform an actual XYZ move and cancel
        the lift. */
    this->_lifted = 0;
    this->_pos = point;
    
    std::ostringstream gcode;
    gcode << "G1 X" << XYZF_NUM(point.x)
          <<   " Y" << XYZF_NUM(point.y)
          <<   " Z" << XYZF_NUM(point.z)
          <<   " F" << XYZF_NUM(this->config.travel_speed.value * 60.0);
    COMMENT(comment);
    gcode << "\n";
    return gcode.str();
}

std::string
GCodeWriter::travel_to_z(double z, const std::string &comment)
{
    /*  If target Z is lower than current Z but higher than nominal Z
        we don't perform the move but we only adjust the nominal Z by
        reducing the lift amount that will be used for unlift. */
    if (!this->will_move_z(z)) {
        double nominal_z = this->_pos.z - this->_lifted;
        this->_lifted -= (z - nominal_z);
        return "";
    }
    
    /*  In all the other cases, we perform an actual Z move and cancel
        the lift. */
    this->_lifted = 0;
    return this->_travel_to_z(z, comment);
}

std::string
GCodeWriter::_travel_to_z(double z, const std::string &comment)
{
    this->_pos.z = z;
    
    std::ostringstream gcode;
    gcode << "G1 Z" << XYZF_NUM(z)
          <<   " F" << XYZF_NUM(this->config.travel_speed.value * 60.0);
    COMMENT(comment);
    gcode << "\n";
    return gcode.str();
}

bool
GCodeWriter::will_move_z(double z) const
{
    /* If target Z is lower than current Z but higher than nominal Z
        we don't perform an actual Z move. */
    if (this->_lifted > 0) {
        double nominal_z = this->_pos.z - this->_lifted;
        if (z >= nominal_z && z <= this->_pos.z)
            return false;
    }
    return true;
}

std::string
GCodeWriter::extrude_to_xy(const Pointf &point, double dE, const std::string &comment)
{
    this->_pos.x = point.x;
    this->_pos.y = point.y;
    this->_extruder->extrude(dE);
    
    std::ostringstream gcode;
    gcode << "G1 X" << XYZF_NUM(point.x)
          <<   " Y" << XYZF_NUM(point.y)
          <<    " " << this->_extrusion_axis << E_NUM(this->_extruder->E);
    COMMENT(comment);
    gcode << "\n";
    return gcode.str();
}

std::string
GCodeWriter::extrude_to_xyz(const Pointf3 &point, double dE, const std::string &comment)
{
    this->_pos = point;
    this->_lifted = 0;
    this->_extruder->extrude(dE);
    
    std::ostringstream gcode;
    gcode << "G1 X" << XYZF_NUM(point.x)
          <<   " Y" << XYZF_NUM(point.y)
          <<   " Z" << XYZF_NUM(point.z)
          <<    " " << this->_extrusion_axis << E_NUM(this->_extruder->E);
    COMMENT(comment);
    gcode << "\n";
    return gcode.str();
}

std::string
GCodeWriter::retract()
{
    return this->_retract(
        this->_extruder->retract_length(),
        this->_extruder->retract_restart_extra(),
        "retract"
    );
}

std::string
GCodeWriter::retract_for_toolchange()
{
    return this->_retract(
        this->_extruder->retract_length_toolchange(),
        this->_extruder->retract_restart_extra_toolchange(),
        "retract for toolchange",
        true
    );
}

std::string
GCodeWriter::_retract(double length, double restart_extra, const std::string &comment, bool long_retract)
{
    std::ostringstream gcode;
    std::ostringstream outcomment;

    outcomment << comment;
    
    /*  If firmware retraction is enabled, we use a fake value of 1
        since we ignore the actual configured retract_length which 
        might be 0, in which case the retraction logic gets skipped. */
    if (this->config.use_firmware_retraction) length = 1;
    
    // If we use volumetric E values we turn lengths into volumes */
    if (this->config.use_volumetric_e) {
        double d = this->_extruder->filament_diameter();
        double area = d * d * PI/4;
        length = length * area;
        restart_extra = restart_extra * area;
    }

    double dE = this->_extruder->retract(length, restart_extra);
    if (dE != 0) {
        outcomment << " extruder " << this->_extruder->id;
        if (this->config.use_firmware_retraction) {
            if (FLAVOR_IS(gcfMachinekit))
                gcode << "G22";
            else if ((FLAVOR_IS(gcfRepRap) || FLAVOR_IS(gcfRepetier)) && long_retract)
                gcode << "G10 S1";
            else
                gcode << "G10";
        } else {
            gcode << "G1 " << this->_extrusion_axis << E_NUM(this->_extruder->E)
                           << " F" << this->_extruder->retract_speed_mm_min;
        }
        COMMENT(outcomment.str());
        gcode << "\n";
    }
    
    if (FLAVOR_IS(gcfMakerWare))
        gcode << "M103 ; extruder off\n";
    
    return gcode.str();
}

std::string
GCodeWriter::unretract()
{
    std::ostringstream gcode;
    
    if (FLAVOR_IS(gcfMakerWare))
        gcode << "M101 ; extruder on\n";
    
    double dE = this->_extruder->unretract();
    if (dE != 0) {
        if (this->config.use_firmware_retraction) {
            if (FLAVOR_IS(gcfMachinekit))
                 gcode << "G23";
            else
                 gcode << "G11";
            if (this->config.gcode_comments) gcode << " ; unretract extruder " << this->_extruder->id;
            gcode << "\n";
            gcode << this->reset_e();
        } else {
            // use G1 instead of G0 because G0 will blend the restart with the previous travel move
            gcode << "G1 " << this->_extrusion_axis << E_NUM(this->_extruder->E)
                           << " F" << this->_extruder->retract_speed_mm_min;
            if (this->config.gcode_comments) gcode << " ; unretract extruder " << this->_extruder->id;
            gcode << "\n";
        }
    }
    
    return gcode.str();
}

/*  If this method is called more than once before calling unlift(),
    it will not perform subsequent lifts, even if Z was raised manually
    (i.e. with travel_to_z()) and thus _lifted was reduced. */
std::string
GCodeWriter::lift()
{
    // check whether the above/below conditions are met
    double target_lift = 0;
    {
        double above = this->config.retract_lift_above.get_at(this->_extruder->id);
        double below = this->config.retract_lift_below.get_at(this->_extruder->id);
        if (this->_pos.z >= above && (below == 0 || this->_pos.z <= below))
            target_lift = this->config.retract_lift.get_at(this->_extruder->id);
    }
    
    // compare against epsilon because travel_to_z() does math on it
    // and subtracting layer_height from retract_lift might not give
    // exactly zero
    if (std::abs(this->_lifted) < EPSILON && target_lift > 0) {
        this->_lifted = target_lift;
        return this->_travel_to_z(this->_pos.z + target_lift, "lift Z");
    }
    
    return "";
}

std::string
GCodeWriter::unlift()
{
    std::string gcode;
    if (this->_lifted > 0) {
        gcode += this->_travel_to_z(this->_pos.z - this->_lifted, "restore layer Z");
        this->_lifted = 0;
    }
    return gcode;
}

}

#include "WireframePrint.hpp"

namespace Slic3r {
	
WireframePrint::WireframePrint()
{
}

WireframePrint::~WireframePrint()
{
}

bool
WireframePrint::invalidate_state_by_config(const PrintConfigBase &config)
{
    const t_config_option_keys diff = this->config.diff(config);
    
    std::set<PrintStep> steps;
    std::set<PrintObjectStep> osteps;
    bool all = false;
    
    // this method only accepts PrintConfig option keys
    for (const t_config_option_key &opt_key : diff) {
        if (opt_key == "skirts"
            || opt_key == "skirt_height"
            || opt_key == "skirt_distance"
            || opt_key == "min_skirt_length"
            || opt_key == "ooze_prevention") {
            steps.insert(psSkirt);
        } else if (opt_key == "brim_width") {
            steps.insert(psBrim);
            steps.insert(psSkirt);
            osteps.insert(posSupportMaterial);
        } else if (opt_key == "brim_width"
            || opt_key == "interior_brim_width"
            || opt_key == "brim_connections_width") {
            steps.insert(psBrim);
            steps.insert(psSkirt);
        } else if (opt_key == "nozzle_diameter"
            || opt_key == "resolution"
            || opt_key == "z_steps_per_mm") {
            osteps.insert(posSlice);
        } else if (opt_key == "avoid_crossing_perimeters"
            || opt_key == "bed_shape"
            || opt_key == "bed_temperature"
            || opt_key == "between_objects_gcode"
            || opt_key == "bridge_acceleration"
            || opt_key == "bridge_fan_speed"
            || opt_key == "complete_objects"
            || opt_key == "cooling"
            || opt_key == "default_acceleration"
            || opt_key == "disable_fan_first_layers"
            || opt_key == "duplicate_distance"
            || opt_key == "end_gcode"
            || opt_key == "extruder_clearance_height"
            || opt_key == "extruder_clearance_radius"
            || opt_key == "extruder_offset"
            || opt_key == "extrusion_axis"
            || opt_key == "extrusion_multiplier"
            || opt_key == "fan_always_on"
            || opt_key == "fan_below_layer_time"
            || opt_key == "filament_colour"
            || opt_key == "filament_diameter"
            || opt_key == "first_layer_acceleration"
            || opt_key == "first_layer_bed_temperature"
            || opt_key == "first_layer_speed"
            || opt_key == "first_layer_temperature"
            || opt_key == "gcode_arcs"
            || opt_key == "gcode_comments"
            || opt_key == "gcode_flavor"
            || opt_key == "infill_acceleration"
            || opt_key == "infill_first"
            || opt_key == "layer_gcode"
            || opt_key == "min_fan_speed"
            || opt_key == "max_fan_speed"
            || opt_key == "min_print_speed"
            || opt_key == "notes"
            || opt_key == "only_retract_when_crossing_perimeters"
            || opt_key == "output_filename_format"
            || opt_key == "perimeter_acceleration"
            || opt_key == "post_process"
            || opt_key == "pressure_advance"
            || opt_key == "retract_before_travel"
            || opt_key == "retract_layer_change"
            || opt_key == "retract_length"
            || opt_key == "retract_length_toolchange"
            || opt_key == "retract_lift"
            || opt_key == "retract_lift_above"
            || opt_key == "retract_lift_below"
            || opt_key == "retract_restart_extra"
            || opt_key == "retract_restart_extra_toolchange"
            || opt_key == "retract_speed"
            || opt_key == "slowdown_below_layer_time"
            || opt_key == "spiral_vase"
            || opt_key == "standby_temperature_delta"
            || opt_key == "start_gcode"
            || opt_key == "temperature"
            || opt_key == "threads"
            || opt_key == "toolchange_gcode"
            || opt_key == "travel_speed"
            || opt_key == "use_firmware_retraction"
            || opt_key == "use_relative_e_distances"
            || opt_key == "vibration_limit"
            || opt_key == "wipe"
            || opt_key == "z_offset") {
            // these options only affect G-code export, so nothing to invalidate
        } else if (opt_key == "first_layer_extrusion_width") {
            osteps.insert(posPerimeters);
            osteps.insert(posInfill);
            osteps.insert(posSupportMaterial);
            steps.insert(psSkirt);
            steps.insert(psBrim);
        } else {
            // for legacy, if we can't handle this option let's invalidate all steps
            all = true;
            break;
        }
    }
    
    if (!diff.empty())
        this->config.apply(config, true);
    
    bool invalidated = false;
    if (all) {
        if (this->invalidate_all_steps())
            invalidated = true;
        
        for (PrintObject* object : this->objects)
            if (object->invalidate_all_steps())
                invalidated = true;
    } else {
        for (const PrintStep &step : steps)
            if (this->invalidate_step(step))
                invalidated = true;
    
        for (const PrintObjectStep &ostep : osteps)
            for (PrintObject* object : this->objects)
                if (object->invalidate_step(ostep))
                    invalidated = true;
    }
    
    return invalidated;
}

bool
WireframePrint::invalidate_step(PrintStep step)
{
    bool invalidated = this->state.invalidate(step);
    
    // propagate to dependent steps
    if (step == psSkirt) {
        this->invalidate_step(psBrim);
    }
    
    return invalidated;
}

bool
WireframePrint::invalidate_all_steps()
{
    // make a copy because when invalidating steps the iterators are not working anymore
    std::set<PrintStep> steps = this->state.started;
    
    bool invalidated = false;
    for (std::set<PrintStep>::const_iterator step = steps.begin(); step != steps.end(); ++step) {
        if (this->invalidate_step(*step)) invalidated = true;
    }
    return invalidated;
}

// returns true if an object step is done on all objects
// and there's at least one object
bool
WireframePrint::step_done(PrintObjectStep step) const
{
    if (this->objects.empty()) return false;
    FOREACH_OBJECT(this, object) {
        if (!(*object)->state.is_done(step))
            return false;
    }
    return true;
}

}
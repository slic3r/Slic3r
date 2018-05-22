#ifndef PRESETEDITOR_HPP
#define PRESETEDITOR_HPP

#include <string>


namespace Slic3r { namespace GUI {
using namespace std::string_literals;

class PresetEditor : public wxPanel {

    /// Member function to retrieve a list of options 
    /// that this preset governs. Subclasses should 
    /// call their own static method.
    virtual t_config_option_keys my_options() = 0;
    static t_config_option_keys options() { return t_config_option_keys {}; }
};

class PrintEditor : public PresetEditor {
public:

    /// Static method to retrieve list of options that this preset governs.
    static t_config_option_keys options() {
        return t_config_option_keys
        {
            "layer_height"s, "first_layer_height"s,
            "adaptive_slicing"s, "adaptive_slicing_quality"s, "match_horizontal_surfaces"s,
            "perimeters"s, "spiral_vase"s,
            "top_solid_layers"s, "bottom_solid_layers"s,
            "extra_perimeters"s, "avoid_crossing_perimeters"s, "thin_walls"s, "overhangs"s,
            "seam_position"s, "external_perimeters_first"s,
            "fill_density"s, "fill_pattern"s, "top_infill_pattern"s, "bottom_infill_pattern"s, "fill_gaps"s,
            "infill_every_layers"s, "infill_only_where_needed"s,
            "solid_infill_every_layers"s, "fill_angle"s, "solid_infill_below_area"s, ""s,
            "only_retract_when_crossing_perimeters"s, "infill_first"s,
            "max_print_speed"s, "max_volumetric_speed"s,
            "perimeter_speed"s, "small_perimeter_speed"s, "external_perimeter_speed"s, "infill_speed"s, ""s,
            "solid_infill_speed"s, "top_solid_infill_speed"s, "support_material_speed"s, ""s,
            "support_material_interface_speed"s, "bridge_speed"s, "gap_fill_speed"s,
            "travel_speed"s,
            "first_layer_speed"s,
            "perimeter_acceleration"s, "infill_acceleration"s, "bridge_acceleration"s, ""s,
            "first_layer_acceleration"s, "default_acceleration"s,
            "skirts"s, "skirt_distance"s, "skirt_height"s, "min_skirt_length"s,
            "brim_connections_width"s, "brim_width"s, "interior_brim_width"s,
            "support_material"s, "support_material_threshold"s, "support_material_max_layers"s, "support_material_enforce_layers"s,
            "raft_layers"s,
            "support_material_pattern"s, "support_material_spacing"s, "support_material_angle"s, ""s,
            "support_material_interface_layers"s, "support_material_interface_spacing"s,
            "support_material_contact_distance"s, "support_material_buildplate_only"s, "dont_support_bridges"s,
            "notes"s,
            "complete_objects"s, "extruder_clearance_radius"s, "extruder_clearance_height"s,
            "gcode_comments"s, "output_filename_format"s,
            "post_process"s,
            "perimeter_extruder"s, "infill_extruder"s, "solid_infill_extruder"s,
            "support_material_extruder"s, "support_material_interface_extruder"s,
            "ooze_prevention"s, "standby_temperature_delta"s,
            "interface_shells"s, "regions_overlap"s,
            "extrusion_width"s, "first_layer_extrusion_width"s, "perimeter_extrusion_width"s, ""s,
            "external_perimeter_extrusion_width"s, "infill_extrusion_width"s, "solid_infill_extrusion_width"s,
            "top_infill_extrusion_width"s, "support_material_extrusion_width"s,
            "support_material_interface_extrusion_width"s, "infill_overlap"s, "bridge_flow_ratio"s,
            "xy_size_compensation"s, "resolution"s, "shortcuts"s, "compatible_printers"s,
            "print_settings_id"s
        };
    }

    t_config_option_keys my_options() override { return PrintEditor::options(); }
};

class PrinterEditor : public PresetEditor {
    
    static t_config_option_keys options() {
        return t_config_option_keys
        {
            "bed_shape"s, "z_offset"s, "z_steps_per_mm"s, "has_heatbed"s,
            "gcode_flavor"s, "use_relative_e_distances"s,
            "serial_port"s, "serial_speed"s,
            "host_type"s, "print_host"s, "octoprint_apikey"s,
            "use_firmware_retraction"s, "pressure_advance"s, "vibration_limit"s,
            "use_volumetric_e"s,
            "start_gcode"s, "end_gcode"s, "before_layer_gcode"s, "layer_gcode"s, "toolchange_gcode"s, "between_objects_gcode"s,
            "nozzle_diameter"s, "extruder_offset"s, "min_layer_height"s, "max_layer_height"s,
            "retract_length"s, "retract_lift"s, "retract_speed"s, "retract_restart_extra"s, "retract_before_travel"s, "retract_layer_change"s, "wipe"s,
            "retract_length_toolchange"s, "retract_restart_extra_toolchange"s, "retract_lift_above"s, "retract_lift_below"s,
            "printer_settings_id"s,
            "printer_notes"s,
            "use_set_and_wait_bed"s, "use_set_and_wait_extruder"s
        };
    }
    
    t_config_option_keys my_options() override { return PrinterEditor::options(); }
};


}} // namespace Slic3r::GUI
#endif // PRESETEDITOR_HPP

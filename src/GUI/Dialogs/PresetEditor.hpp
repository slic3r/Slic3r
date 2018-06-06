#ifndef PRESETEDITOR_HPP
#define PRESETEDITOR_HPP

// stdlib
#include <string>
#include <functional>

// Libslic3r
#include "libslic3r.h"
#include "ConfigBase.hpp"
#include "Config.hpp"

// GUI
#include "misc_ui.hpp"

// Wx
#include <wx/treectrl.h>
#include <wx/imaglist.h>
#include <wx/panel.h>

#if SLIC3R_CPPVER==11
    #define st(A) (std::string(#A))
#elif SLIC3R_CPPVER==14
    #define st(A) ((#A)s)
#endif

// TODO for C++14 find/replace st([A-z_]*) with "\1"s

namespace Slic3r { namespace GUI {

class PresetEditor : public wxPanel {

public: 
    /// Member function to retrieve a list of options 
    /// that this preset governs. Subclasses should 
    /// call their own static method.
    virtual t_config_option_keys my_options() = 0;
    static t_config_option_keys options() { return t_config_option_keys {}; }

    wxSizer* sizer() { return _sizer; };
    PresetEditor(wxWindow* parent, t_config_option_keys options = {});
    PresetEditor() : PresetEditor(nullptr, {}) {};

    bool prompt_unsaved_changes();
    void select_preset_by_name(const wxString& name, bool force = false);
    
    /// suppress the callback when the tree selection is changed.
    bool disable_tree_sel_changed_event {false};

    void save_preset();
    std::function<void ()> on_save_preset {};
    std::function<void ()> on_value_change {};

    config_ptr config;

    virtual wxString title() = 0;
    virtual std::string name() = 0;
protected:
    // Main sizer
    wxSizer* _sizer {nullptr};
    wxString presets;
    wxImageList* _icons {nullptr};
    wxTreeCtrl* _treectrl {nullptr};
    wxBitmapButton* _btn_save_preset {nullptr}; 
    wxBitmapButton* _btn_delete_preset {nullptr};
    wxChoice* _presets_choice {nullptr};
    int _iconcount {-1};

    std::vector<wxString> _pages {};

    const unsigned int left_col_width {150};
    void _update_tree();
    void load_presets();

    virtual void _build() = 0;
    virtual void _update() = 0;
    virtual void _on_preset_loaded() = 0;
    void set_tooltips() { 
        this->_btn_save_preset->SetToolTip(wxString(_("Save current ")) + this->title());
        this->_btn_delete_preset->SetToolTip(_("Delete this preset."));
    }
};

class PrintEditor : public PresetEditor {
public:

    PrintEditor(wxWindow* parent, t_config_option_keys options = {});
    PrintEditor() : PrintEditor(nullptr, {}) {};

    wxString title() override { return _("Print Settings"); }
    std::string name() override { return st("print"); }

    /// Static method to retrieve list of options that this preset governs.
    static t_config_option_keys options() {
        return t_config_option_keys
        {
            st(layer_height), st(first_layer_height),
            st(adaptive_slicing), st(adaptive_slicing_quality), st(match_horizontal_surfaces),
            st(perimeters), st(spiral_vase),
            st(top_solid_layers), st(bottom_solid_layers),
            st(extra_perimeters), st(avoid_crossing_perimeters), st(thin_walls), st(overhangs),
            st(seam_position), st(external_perimeters_first),
            st(fill_density), st(fill_pattern), st(top_infill_pattern), st(bottom_infill_pattern), st(fill_gaps),
            st(infill_every_layers), st(infill_only_where_needed),
            st(solid_infill_every_layers), st(fill_angle), st(solid_infill_below_area), st(),
            st(only_retract_when_crossing_perimeters), st(infill_first),
            st(max_print_speed), st(max_volumetric_speed),
            st(perimeter_speed), st(small_perimeter_speed), st(external_perimeter_speed), st(infill_speed), st(),
            st(solid_infill_speed), st(top_solid_infill_speed), st(support_material_speed),
            st(support_material_interface_speed), st(bridge_speed), st(gap_fill_speed),
            st(travel_speed),
            st(first_layer_speed),
            st(perimeter_acceleration), st(infill_acceleration), st(bridge_acceleration),
            st(first_layer_acceleration), st(default_acceleration),
            st(skirts), st(skirt_distance), st(skirt_height), st(min_skirt_length),
            st(brim_connections_width), st(brim_width), st(interior_brim_width),
            st(support_material), st(support_material_threshold), st(support_material_max_layers), st(support_material_enforce_layers),
            st(raft_layers),
            st(support_material_pattern), st(support_material_spacing), st(support_material_angle), st(),
            st(support_material_interface_layers), st(support_material_interface_spacing),
            st(support_material_contact_distance), st(support_material_buildplate_only), st(dont_support_bridges),
            st(notes),
            st(complete_objects), st(extruder_clearance_radius), st(extruder_clearance_height),
            st(gcode_comments), st(output_filename_format),
            st(post_process),
            st(perimeter_extruder), st(infill_extruder), st(solid_infill_extruder),
            st(support_material_extruder), st(support_material_interface_extruder),
            st(ooze_prevention), st(standby_temperature_delta),
            st(interface_shells), st(regions_overlap),
            st(extrusion_width), st(first_layer_extrusion_width), st(perimeter_extrusion_width), st(),
            st(external_perimeter_extrusion_width), st(infill_extrusion_width), st(solid_infill_extrusion_width),
            st(top_infill_extrusion_width), st(support_material_extrusion_width),
            st(support_material_interface_extrusion_width), st(infill_overlap), st(bridge_flow_ratio),
            st(xy_size_compensation), st(resolution), st(shortcuts), st(compatible_printers),
            st(print_settings_id)
        };
    }

    t_config_option_keys my_options() override { return PrintEditor::options(); }

protected:
    void _update() override;
    void _build() override;
};

class MaterialEditor : public PresetEditor {
public:

    wxString title() override { return _("Material Settings"); }
    std::string name() override { return st("material"); }
    MaterialEditor(wxWindow* parent, t_config_option_keys options = {});
    MaterialEditor() : MaterialEditor(nullptr, {}) {};

    static t_config_option_keys options() {
        return t_config_option_keys
        {
            st(filament_colour), st(filament_diameter), st(filament_notes), st(filament_max_volumetric_speed), st(extrusion_multiplier), st(filament_density), st(filament_cost),
            st(temperature), st(first_layer_temperature), st(bed_temperature), st(first_layer_bed_temperature),
            st(fan_always_on), st(cooling), st(compatible_printers),
            st(min_fan_speed), st(max_fan_speed), st(bridge_fan_speed), st(disable_fan_first_layers),
            st(fan_below_layer_time), st(slowdown_below_layer_time), st(min_print_speed),
            st(start_filament_gcode), st(end_filament_gcode),
            st(filament_settings_id)
        };
    }
    
    t_config_option_keys my_options() override { return MaterialEditor::options(); }
protected:
    void _update() override;
    void _build() override;
};

class PrinterEditor : public PresetEditor {
public:
    PrinterEditor(wxWindow* parent, t_config_option_keys options = {});
    PrinterEditor() : PrinterEditor(nullptr, {}) {};

    wxString title() override { return _("Printer Settings"); }
    std::string name() override { return st("printer"); }
    
    static t_config_option_keys options() {
        return t_config_option_keys
        {
            st(bed_shape), st(z_offset), st(z_steps_per_mm), st(has_heatbed),
            st(gcode_flavor), st(use_relative_e_distances),
            st(serial_port), st(serial_speed),
            st(host_type), st(print_host), st(octoprint_apikey),
            st(use_firmware_retraction), st(pressure_advance), st(vibration_limit),
            st(use_volumetric_e),
            st(start_gcode), st(end_gcode), st(before_layer_gcode), st(layer_gcode), st(toolchange_gcode), st(between_objects_gcode),
            st(nozzle_diameter), st(extruder_offset), st(min_layer_height), st(max_layer_height),
            st(retract_length), st(retract_lift), st(retract_speed), st(retract_restart_extra), st(retract_before_travel), st(retract_layer_change), st(wipe),
            st(retract_length_toolchange), st(retract_restart_extra_toolchange), st(retract_lift_above), st(retract_lift_below),
            st(printer_settings_id),
            st(printer_notes),
            st(use_set_and_wait_bed), st(use_set_and_wait_extruder)
        };
    }
    
    t_config_option_keys my_options() override { return PrinterEditor::options(); }
protected:
    void _update() override;
    void _build() override;
};


}} // namespace Slic3r::GUI
#endif // PRESETEDITOR_HPP

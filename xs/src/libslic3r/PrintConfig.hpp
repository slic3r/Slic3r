// Configuration store of Slic3r.
//
// The configuration store is either static or dynamic.
// DynamicPrintConfig is used mainly at the user interface. while the StaticPrintConfig is used
// during the slicing and the g-code generation.
//
// The classes derived from StaticPrintConfig form a following hierarchy.
// Virtual inheritance is used for some of the parent objects.
//
// FullPrintConfig
//    PrintObjectConfig
//    PrintRegionConfig
//    PrintConfig
//        GCodeConfig
//    HostConfig
//

#ifndef slic3r_PrintConfig_hpp_
#define slic3r_PrintConfig_hpp_

#include "libslic3r.h"
#include "ConfigBase.hpp"

#define OPT_PTR(KEY) if (opt_key == #KEY) return &this->KEY

namespace Slic3r {

enum GCodeFlavor {
    gcfRepRap, gcfTeacup, gcfMakerWare, gcfSailfish, gcfMach3, gcfMachinekit, gcfNoExtrusion, gcfSmoothie, gcfRepetier,
};

enum HostType {
    htOctoprint, htDuet,
};

enum InfillPattern {
    ipRectilinear, ipGrid, ipAlignedRectilinear,
    ipTriangles, ipStars, ipCubic, 
    ipConcentric, ipHoneycomb, ip3DHoneycomb,
    ipGyroid, ipHilbertCurve, ipArchimedeanChords, ipOctagramSpiral,
};

enum SupportMaterialPattern {
    smpRectilinear, smpRectilinearGrid, smpHoneycomb, smpPillars,
};

enum SeamPosition {
    spRandom, spNearest, spAligned, spRear
};

template<> inline t_config_enum_values ConfigOptionEnum<GCodeFlavor>::get_enum_values() {
    t_config_enum_values keys_map;
    keys_map["reprap"]          = gcfRepRap;
    keys_map["repetier"]        = gcfRepetier;
    keys_map["teacup"]          = gcfTeacup;
    keys_map["makerware"]       = gcfMakerWare;
    keys_map["sailfish"]        = gcfSailfish;
    keys_map["mach3"]           = gcfMach3;
    keys_map["machinekit"]      = gcfMachinekit;
    keys_map["no-extrusion"]    = gcfNoExtrusion;
    keys_map["smoothie"]    = gcfSmoothie;
    return keys_map;
}

template<> inline t_config_enum_values ConfigOptionEnum<HostType>::get_enum_values() {
    t_config_enum_values keys_map;
    keys_map["octoprint"]           = htOctoprint;
    keys_map["duet"]                = htDuet;
    return keys_map;
}

template<> inline t_config_enum_values ConfigOptionEnum<InfillPattern>::get_enum_values() {
    t_config_enum_values keys_map;
    keys_map["rectilinear"]         = ipRectilinear;
    keys_map["alignedrectilinear"]  = ipAlignedRectilinear;
    keys_map["grid"]                = ipGrid;
    keys_map["triangles"]           = ipTriangles;
    keys_map["stars"]               = ipStars;
    keys_map["cubic"]               = ipCubic;
    keys_map["concentric"]          = ipConcentric;
    keys_map["honeycomb"]           = ipHoneycomb;
    keys_map["3dhoneycomb"]         = ip3DHoneycomb;
    keys_map["gyroid"]              = ipGyroid;
    keys_map["hilbertcurve"]        = ipHilbertCurve;
    keys_map["archimedeanchords"]   = ipArchimedeanChords;
    keys_map["octagramspiral"]      = ipOctagramSpiral;
    return keys_map;
}

template<> inline t_config_enum_values ConfigOptionEnum<SupportMaterialPattern>::get_enum_values() {
    t_config_enum_values keys_map;
    keys_map["rectilinear"]         = smpRectilinear;
    keys_map["rectilinear-grid"]    = smpRectilinearGrid;
    keys_map["honeycomb"]           = smpHoneycomb;
    keys_map["pillars"]             = smpPillars;
    return keys_map;
}

template<> inline t_config_enum_values ConfigOptionEnum<SeamPosition>::get_enum_values() {
    t_config_enum_values keys_map;
    keys_map["random"]              = spRandom;
    keys_map["nearest"]             = spNearest;
    keys_map["aligned"]             = spAligned;
    keys_map["rear"]                = spRear;
    return keys_map;
}

// Defines each and every configuration option of Slic3r, including the properties of the GUI dialogs.
// Does not store the actual values, but defines default values.
class PrintConfigDef : public ConfigDef
{
    public:
    PrintConfigDef();
};

// The one and only global definition of SLic3r configuration options.
// This definition is constant.
extern const PrintConfigDef print_config_def;

// Slic3r configuration storage with print_config_def assigned.
class PrintConfigBase : public virtual ConfigBase
{
    public:
    PrintConfigBase() {
        this->def = &print_config_def;
    };
    bool set_deserialize(t_config_option_key opt_key, std::string str, bool append = false);
    double min_object_distance() const;
    
    protected:
    void _handle_legacy(t_config_option_key &opt_key, std::string &value) const;
};

// Slic3r dynamic configuration, used to override the configuration 
// per object, per modification volume or per printing material.
// The dynamic configuration is also used to store user modifications of the print global parameters,
// so the modified configuration values may be diffed against the active configuration
// to invalidate the proper slicing resp. g-code generation processing steps.
// This object is mapped to Perl as Slic3r::Config.
class DynamicPrintConfig : public PrintConfigBase, public DynamicConfig
{
    public:
    DynamicPrintConfig() : PrintConfigBase(), DynamicConfig() {};
    void normalize();
};


class StaticPrintConfig : public PrintConfigBase, public StaticConfig
{
    public:
    StaticPrintConfig() : PrintConfigBase(), StaticConfig() {};
};

// This object is mapped to Perl as Slic3r::Config::PrintObject.
class PrintObjectConfig : public virtual StaticPrintConfig
{
    public:
    ConfigOptionBool                adaptive_slicing;
    ConfigOptionPercent             adaptive_slicing_quality;
    ConfigOptionBool                dont_support_bridges;
    ConfigOptionFloatOrPercent      extrusion_width;
    ConfigOptionFloatOrPercent      first_layer_height;
    ConfigOptionBool                infill_only_where_needed;
    ConfigOptionBool                interface_shells;
    ConfigOptionFloat               layer_height;
    ConfigOptionBool                match_horizontal_surfaces;
    ConfigOptionInt                 raft_layers;
    ConfigOptionFloat               regions_overlap;
    ConfigOptionEnum<SeamPosition>  seam_position;
    ConfigOptionBool                support_material;
    ConfigOptionInt                 support_material_angle;
    ConfigOptionBool                support_material_buildplate_only;
    ConfigOptionFloat               support_material_contact_distance;
    ConfigOptionInt                 support_material_max_layers;
    ConfigOptionInt                 support_material_enforce_layers;
    ConfigOptionInt                 support_material_extruder;
    ConfigOptionFloatOrPercent      support_material_extrusion_width;
    ConfigOptionInt                 support_material_interface_extruder;
    ConfigOptionFloatOrPercent      support_material_interface_extrusion_width;
    ConfigOptionInt                 support_material_interface_layers;
    ConfigOptionFloat               support_material_interface_spacing;
    ConfigOptionFloatOrPercent      support_material_interface_speed;
    ConfigOptionEnum<SupportMaterialPattern> support_material_pattern;
    ConfigOptionFloat               support_material_pillar_size;
    ConfigOptionFloat               support_material_pillar_spacing;
    ConfigOptionFloat               support_material_spacing;
    ConfigOptionFloat               support_material_speed;
    ConfigOptionFloatOrPercent      support_material_threshold;
    ConfigOptionFloat               xy_size_compensation;
    ConfigOptionInt                 sequential_print_priority;
    
    PrintObjectConfig(bool initialize = true) : StaticPrintConfig() {
        if (initialize)
            this->set_defaults();
    }
    
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) {
        OPT_PTR(adaptive_slicing);
        OPT_PTR(adaptive_slicing_quality);
        OPT_PTR(dont_support_bridges);
        OPT_PTR(extrusion_width);
        OPT_PTR(first_layer_height);
        OPT_PTR(infill_only_where_needed);
        OPT_PTR(interface_shells);
        OPT_PTR(layer_height);
        OPT_PTR(match_horizontal_surfaces);
        OPT_PTR(raft_layers);
        OPT_PTR(regions_overlap);
        OPT_PTR(seam_position);
        OPT_PTR(support_material);
        OPT_PTR(support_material_angle);
        OPT_PTR(support_material_buildplate_only);
        OPT_PTR(support_material_contact_distance);
        OPT_PTR(support_material_max_layers);
        OPT_PTR(support_material_enforce_layers);
        OPT_PTR(support_material_extruder);
        OPT_PTR(support_material_extrusion_width);
        OPT_PTR(support_material_interface_extruder);
        OPT_PTR(support_material_interface_extrusion_width);
        OPT_PTR(support_material_interface_layers);
        OPT_PTR(support_material_interface_spacing);
        OPT_PTR(support_material_interface_speed);
        OPT_PTR(support_material_pattern);
        OPT_PTR(support_material_pillar_size);
        OPT_PTR(support_material_pillar_spacing);
        OPT_PTR(support_material_spacing);
        OPT_PTR(support_material_speed);
        OPT_PTR(support_material_threshold);
        OPT_PTR(xy_size_compensation);
        OPT_PTR(sequential_print_priority);
        
        return NULL;
    };
};

// This object is mapped to Perl as Slic3r::Config::PrintRegion.
class PrintRegionConfig : public virtual StaticPrintConfig
{
    public:
    ConfigOptionEnum<InfillPattern> bottom_infill_pattern;
    ConfigOptionInt                 bottom_solid_layers;
    ConfigOptionFloat               bridge_flow_ratio;
    ConfigOptionFloat               bridge_speed;
    ConfigOptionFloatOrPercent      external_perimeter_extrusion_width;
    ConfigOptionFloatOrPercent      external_perimeter_speed;
    ConfigOptionBool                external_perimeters_first;
    ConfigOptionBool                extra_perimeters;
    ConfigOptionFloat               fill_angle;
    ConfigOptionPercent             fill_density;
    ConfigOptionBool                fill_gaps;
    ConfigOptionEnum<InfillPattern> fill_pattern;
    ConfigOptionFloatOrPercent      gap_fill_speed;
    ConfigOptionInt                 infill_extruder;
    ConfigOptionFloatOrPercent      infill_extrusion_width;
    ConfigOptionInt                 infill_every_layers;
    ConfigOptionFloatOrPercent      infill_overlap;
    ConfigOptionFloat               infill_speed;
    ConfigOptionFloat               min_shell_thickness;
    ConfigOptionFloat               min_top_bottom_shell_thickness;
    ConfigOptionBool                overhangs;
    ConfigOptionInt                 perimeter_extruder;
    ConfigOptionFloatOrPercent      perimeter_extrusion_width;
    ConfigOptionFloat               perimeter_speed;
    ConfigOptionInt                 perimeters;
    ConfigOptionFloatOrPercent      small_perimeter_speed;
    ConfigOptionFloat               solid_infill_below_area;
    ConfigOptionInt                 solid_infill_extruder;
    ConfigOptionFloatOrPercent      solid_infill_extrusion_width;
    ConfigOptionInt                 solid_infill_every_layers;
    ConfigOptionFloatOrPercent      solid_infill_speed;
    ConfigOptionBool                thin_walls;
    ConfigOptionFloatOrPercent      top_infill_extrusion_width;
    ConfigOptionEnum<InfillPattern> top_infill_pattern;
    ConfigOptionInt                 top_solid_layers;
    ConfigOptionFloatOrPercent      top_solid_infill_speed;
    
    PrintRegionConfig(bool initialize = true) : StaticPrintConfig() {
        if (initialize)
            this->set_defaults();
    }
    
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) {
        OPT_PTR(bottom_infill_pattern);
        OPT_PTR(bottom_solid_layers);
        OPT_PTR(bridge_flow_ratio);
        OPT_PTR(bridge_speed);
        OPT_PTR(external_perimeter_extrusion_width);
        OPT_PTR(external_perimeter_speed);
        OPT_PTR(external_perimeters_first);
        OPT_PTR(extra_perimeters);
        OPT_PTR(fill_angle);
        OPT_PTR(fill_density);
        OPT_PTR(fill_gaps);
        OPT_PTR(fill_pattern);
        OPT_PTR(gap_fill_speed);
        OPT_PTR(infill_extruder);
        OPT_PTR(infill_extrusion_width);
        OPT_PTR(infill_every_layers);
        OPT_PTR(infill_overlap);
        OPT_PTR(infill_speed);
        OPT_PTR(min_shell_thickness);
        OPT_PTR(overhangs);
        OPT_PTR(perimeter_extruder);
        OPT_PTR(perimeter_extrusion_width);
        OPT_PTR(perimeter_speed);
        OPT_PTR(perimeters);
        OPT_PTR(small_perimeter_speed);
        OPT_PTR(solid_infill_below_area);
        OPT_PTR(solid_infill_extruder);
        OPT_PTR(solid_infill_extrusion_width);
        OPT_PTR(solid_infill_every_layers);
        OPT_PTR(solid_infill_speed);
        OPT_PTR(thin_walls);
        OPT_PTR(top_infill_extrusion_width);
        OPT_PTR(top_infill_pattern);
        OPT_PTR(top_solid_infill_speed);
        OPT_PTR(top_solid_layers);
        OPT_PTR(min_top_bottom_shell_thickness);

        return NULL;
    };
};

// This object is mapped to Perl as Slic3r::Config::GCode.
class GCodeConfig : public virtual StaticPrintConfig
{
    public:
    ConfigOptionString              pause_gcode;
    ConfigOptionString              pause_heights;
    ConfigOptionString              before_layer_gcode;
    ConfigOptionString              between_objects_gcode;
    ConfigOptionString              end_gcode;
    ConfigOptionStrings             end_filament_gcode;
    ConfigOptionString              extrusion_axis;
    ConfigOptionFloats              extrusion_multiplier;
    ConfigOptionBool                fan_percentage;
    ConfigOptionFloats              filament_diameter;
    ConfigOptionFloats              filament_density;
    ConfigOptionFloats              filament_cost;
    ConfigOptionFloats              filament_max_volumetric_speed;
    ConfigOptionStrings             filament_notes;
    ConfigOptionBool                gcode_comments;
    ConfigOptionEnum<GCodeFlavor>   gcode_flavor;
    ConfigOptionBool                label_printed_objects;
    ConfigOptionString              layer_gcode;
    ConfigOptionFloat               max_print_speed;
    ConfigOptionFloat               max_volumetric_speed;
    ConfigOptionString              notes;
    ConfigOptionFloat               pressure_advance;
    ConfigOptionString              printer_notes;
    ConfigOptionFloats              retract_length;
    ConfigOptionFloats              retract_length_toolchange;
    ConfigOptionFloats              retract_lift;
    ConfigOptionFloats              retract_lift_above;
    ConfigOptionFloats              retract_lift_below;
    ConfigOptionFloats              retract_restart_extra;
    ConfigOptionFloats              retract_restart_extra_toolchange;
    ConfigOptionFloats              retract_speed;
    ConfigOptionString              start_gcode;
    ConfigOptionStrings             start_filament_gcode;
    ConfigOptionString              toolchange_gcode;
    ConfigOptionFloat               travel_speed;
    ConfigOptionBool                use_firmware_retraction;
    ConfigOptionBool                use_relative_e_distances;
    ConfigOptionBool                use_volumetric_e;
    ConfigOptionBool                use_set_and_wait_extruder;
    ConfigOptionBool                use_set_and_wait_bed;
    
    GCodeConfig(bool initialize = true) : StaticPrintConfig() {
        if (initialize)
            this->set_defaults();
    }
    
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) {
        OPT_PTR(pause_gcode);
        OPT_PTR(pause_heights);
        OPT_PTR(before_layer_gcode);
        OPT_PTR(between_objects_gcode);
        OPT_PTR(end_gcode);
        OPT_PTR(end_filament_gcode);
        OPT_PTR(extrusion_axis);
        OPT_PTR(extrusion_multiplier);
        OPT_PTR(fan_percentage);
        OPT_PTR(filament_diameter);
        OPT_PTR(filament_density);
        OPT_PTR(filament_cost);
        OPT_PTR(filament_max_volumetric_speed);
        OPT_PTR(filament_notes);
        OPT_PTR(gcode_comments);
        OPT_PTR(gcode_flavor);
        OPT_PTR(label_printed_objects);
        OPT_PTR(layer_gcode);
        OPT_PTR(max_print_speed);
        OPT_PTR(max_volumetric_speed);
        OPT_PTR(notes);
        OPT_PTR(pressure_advance);
        OPT_PTR(printer_notes);
        OPT_PTR(retract_length);
        OPT_PTR(retract_length_toolchange);
        OPT_PTR(retract_lift);
        OPT_PTR(retract_lift_above);
        OPT_PTR(retract_lift_below);
        OPT_PTR(retract_restart_extra);
        OPT_PTR(retract_restart_extra_toolchange);
        OPT_PTR(retract_speed);
        OPT_PTR(start_gcode);
        OPT_PTR(start_filament_gcode);
        OPT_PTR(toolchange_gcode);
        OPT_PTR(travel_speed);
        OPT_PTR(use_firmware_retraction);
        OPT_PTR(use_relative_e_distances);
        OPT_PTR(use_volumetric_e);
        OPT_PTR(use_set_and_wait_extruder);
        OPT_PTR(use_set_and_wait_bed);
        
        return NULL;
    };
    
    std::string get_extrusion_axis() const
    {
        if ((this->gcode_flavor.value == gcfMach3) || (this->gcode_flavor.value == gcfMachinekit)) {
            return "A";
        } else if (this->gcode_flavor.value == gcfNoExtrusion) {
            return "";
        } else {
            return this->extrusion_axis.value;
        }
    };
};

// This object is mapped to Perl as Slic3r::Config::Print.
class PrintConfig : public GCodeConfig
{
    public:
    ConfigOptionBool                avoid_crossing_perimeters;
    ConfigOptionPoints              bed_shape;
    ConfigOptionBool                has_heatbed;
    ConfigOptionInt                 bed_temperature;
    ConfigOptionFloat               bridge_acceleration;
    ConfigOptionInt                 bridge_fan_speed;
    ConfigOptionFloat               brim_connections_width;
    ConfigOptionBool                brim_ears;
    ConfigOptionFloat               brim_ears_max_angle;
    ConfigOptionFloat               brim_width;
    ConfigOptionBool                complete_objects;
    ConfigOptionBool                cooling;
    ConfigOptionFloat               default_acceleration;
    ConfigOptionInt                 disable_fan_first_layers;
    ConfigOptionFloat               duplicate_distance;
    ConfigOptionFloat               extruder_clearance_height;
    ConfigOptionFloat               extruder_clearance_radius;
    ConfigOptionPoints              extruder_offset;
    ConfigOptionBool                fan_always_on;
    ConfigOptionInt                 fan_below_layer_time;
    ConfigOptionStrings             filament_colour;
    ConfigOptionFloat               first_layer_acceleration;
    ConfigOptionInt                 first_layer_bed_temperature;
    ConfigOptionFloatOrPercent      first_layer_extrusion_width;
    ConfigOptionFloatOrPercent      first_layer_speed;
    ConfigOptionInts                first_layer_temperature;
    ConfigOptionBool                gcode_arcs;
    ConfigOptionFloat               infill_acceleration;
    ConfigOptionBool                infill_first;
    ConfigOptionFloat               interior_brim_width;
    ConfigOptionInt                 max_fan_speed;
    ConfigOptionFloats              max_layer_height;
    ConfigOptionInt                 min_fan_speed;
    ConfigOptionFloats              min_layer_height;
    ConfigOptionFloat               min_print_speed;
    ConfigOptionFloat               min_skirt_length;
    ConfigOptionFloats              nozzle_diameter;
    ConfigOptionBool                only_retract_when_crossing_perimeters;
    ConfigOptionBool                ooze_prevention;
    ConfigOptionString              output_filename_format;
    ConfigOptionFloat               perimeter_acceleration;
    ConfigOptionStrings             post_process;
    ConfigOptionFloat               resolution;
    ConfigOptionFloats              retract_before_travel;
    ConfigOptionBools               retract_layer_change;
    ConfigOptionFloat               skirt_distance;
    ConfigOptionInt                 skirt_height;
    ConfigOptionInt                 skirts;
    ConfigOptionInt                 slowdown_below_layer_time;
    ConfigOptionBool                spiral_vase;
    ConfigOptionInt                 standby_temperature_delta;
    ConfigOptionInts                temperature;
    ConfigOptionInt                 threads;
    ConfigOptionFloat               vibration_limit;
    ConfigOptionBools               wipe;
    ConfigOptionFloat               z_offset;
    ConfigOptionFloat               z_steps_per_mm;
    
    PrintConfig(bool initialize = true) : GCodeConfig(false) {
        if (initialize)
            this->set_defaults();
    }
    
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) {
        OPT_PTR(avoid_crossing_perimeters);
        OPT_PTR(bed_shape);
        OPT_PTR(has_heatbed);
        OPT_PTR(bed_temperature);
        OPT_PTR(bridge_acceleration);
        OPT_PTR(bridge_fan_speed);
        OPT_PTR(brim_connections_width);
        OPT_PTR(brim_ears);
        OPT_PTR(brim_ears_max_angle);
        OPT_PTR(brim_width);
        OPT_PTR(complete_objects);
        OPT_PTR(cooling);
        OPT_PTR(default_acceleration);
        OPT_PTR(disable_fan_first_layers);
        OPT_PTR(duplicate_distance);
        OPT_PTR(extruder_clearance_height);
        OPT_PTR(extruder_clearance_radius);
        OPT_PTR(extruder_offset);
        OPT_PTR(fan_always_on);
        OPT_PTR(fan_below_layer_time);
        OPT_PTR(filament_colour);
        OPT_PTR(first_layer_acceleration);
        OPT_PTR(first_layer_bed_temperature);
        OPT_PTR(first_layer_extrusion_width);
        OPT_PTR(first_layer_speed);
        OPT_PTR(first_layer_temperature);
        OPT_PTR(gcode_arcs);
        OPT_PTR(infill_acceleration);
        OPT_PTR(infill_first);
        OPT_PTR(interior_brim_width);
        OPT_PTR(max_fan_speed);
        OPT_PTR(max_layer_height);
        OPT_PTR(min_fan_speed);
        OPT_PTR(min_layer_height);
        OPT_PTR(min_print_speed);
        OPT_PTR(min_skirt_length);
        OPT_PTR(nozzle_diameter);
        OPT_PTR(only_retract_when_crossing_perimeters);
        OPT_PTR(ooze_prevention);
        OPT_PTR(output_filename_format);
        OPT_PTR(perimeter_acceleration);
        OPT_PTR(post_process);
        OPT_PTR(resolution);
        OPT_PTR(retract_before_travel);
        OPT_PTR(retract_layer_change);
        OPT_PTR(skirt_distance);
        OPT_PTR(skirt_height);
        OPT_PTR(skirts);
        OPT_PTR(slowdown_below_layer_time);
        OPT_PTR(spiral_vase);
        OPT_PTR(standby_temperature_delta);
        OPT_PTR(temperature);
        OPT_PTR(threads);
        OPT_PTR(vibration_limit);
        OPT_PTR(wipe);
        OPT_PTR(z_offset);
        OPT_PTR(z_steps_per_mm);
        
        // look in parent class
        ConfigOption* opt;
        if ((opt = GCodeConfig::optptr(opt_key, create)) != NULL) return opt;
        
        return NULL;
    };
};

class HostConfig : public virtual StaticPrintConfig
{
    public:
    ConfigOptionEnum<HostType>      host_type;
    ConfigOptionString              print_host;
    ConfigOptionString              octoprint_apikey;
    ConfigOptionString              serial_port;
    ConfigOptionInt                 serial_speed;
    
    HostConfig(bool initialize = true) : StaticPrintConfig() {
        if (initialize)
            this->set_defaults();
    }
    
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) {
        OPT_PTR(host_type);
        OPT_PTR(print_host);
        OPT_PTR(octoprint_apikey);
        OPT_PTR(serial_port);
        OPT_PTR(serial_speed);
        
        return NULL;
    };
};

// This object is mapped to Perl as Slic3r::Config::Full.
class FullPrintConfig
    : public PrintObjectConfig, public PrintRegionConfig, public PrintConfig, public HostConfig
{
    public:
    FullPrintConfig(bool initialize = true) :
        PrintObjectConfig(false),
        PrintRegionConfig(false), 
        PrintConfig(false), 
        HostConfig(false)
    {
        if (initialize)
            this->set_defaults();
    }

    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) {
        ConfigOption* opt;
        if ((opt = PrintObjectConfig::optptr(opt_key, create)) != NULL) return opt;
        if ((opt = PrintRegionConfig::optptr(opt_key, create)) != NULL) return opt;
        if ((opt = PrintConfig::optptr(opt_key, create)) != NULL) return opt;
        if ((opt = HostConfig::optptr(opt_key, create)) != NULL) return opt;
        return NULL;
    };
};

class SLAPrintConfig
    : public virtual StaticPrintConfig
{
    public:
    ConfigOptionFloat               fill_angle;
    ConfigOptionPercent             fill_density;
    ConfigOptionEnum<InfillPattern> fill_pattern;
    ConfigOptionFloatOrPercent      first_layer_height;
    ConfigOptionFloatOrPercent      infill_extrusion_width;
    ConfigOptionFloat               layer_height;
    ConfigOptionFloatOrPercent      perimeter_extrusion_width;
    ConfigOptionInt                 raft_layers;
    ConfigOptionFloat               raft_offset;
    ConfigOptionBool                support_material;
    ConfigOptionFloatOrPercent      support_material_extrusion_width;
    ConfigOptionFloat               support_material_spacing;
    ConfigOptionInt                 threads;
    
    virtual ConfigOption* optptr(const t_config_option_key &opt_key, bool create = false) {
        OPT_PTR(fill_angle);
        OPT_PTR(fill_density);
        OPT_PTR(fill_pattern);
        OPT_PTR(first_layer_height);
        OPT_PTR(infill_extrusion_width);
        OPT_PTR(layer_height);
        OPT_PTR(perimeter_extrusion_width);
        OPT_PTR(raft_layers);
        OPT_PTR(raft_offset);
        OPT_PTR(support_material);
        OPT_PTR(support_material_extrusion_width);
        OPT_PTR(support_material_spacing);
        OPT_PTR(threads);
        
        return NULL;
    };
};

class CLIActionsConfigDef : public ConfigDef
{
    public:
    CLIActionsConfigDef();
};

class CLITransformConfigDef : public ConfigDef
{
    public:
    CLITransformConfigDef();
};

class CLIMiscConfigDef : public ConfigDef
{
    public:
    CLIMiscConfigDef();
};

// This class defines the command line options representing actions.
extern const CLIActionsConfigDef    cli_actions_config_def;

// This class defines the command line options representing transforms.
extern const CLITransformConfigDef  cli_transform_config_def;

// This class defines all command line options that are not actions or transforms.
extern const CLIMiscConfigDef       cli_misc_config_def;

}

#endif

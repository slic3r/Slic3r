package Slic3r::Config;
use strict;
use warnings;
use utf8;

#==============================================
# For setlocale.
use POSIX qw (setlocale);
use Locale::Messages qw (LC_MESSAGES);

use Locale::TextDomain ('Slic3r');

# Set the locale according to the environment.
setlocale (LC_MESSAGES, "");
#==============================================

use List::Util qw(first);

# cemetery of old config settings
our @Ignore = qw(duplicate_x duplicate_y multiply_x multiply_y support_material_tool acceleration);

my $serialize_comma     = sub { join ',', @{$_[0]} };
my $serialize_comma_bool = sub { join ',', map $_ // 0, @{$_[0]} };
my $deserialize_comma   = sub { [ split /,/, $_[0] ] };

our $Options = {

    # miscellaneous options
    'notes' => {
        label   => __"Configuration notes",
        tooltip => __"You can put here your personal notes. This text will be added to the G-code header comments.",
        cli     => 'notes=s',
        type    => 's',
        multiline => 1,
        full_width => 1,
        height  => 130,
        serialize   => sub { join '\n', split /\R/, $_[0] },
        deserialize => sub { join "\n", split /\\n/, $_[0] },
        default => '',
    },
    'threads' => {
        label   => __"Threads",
        tooltip => __"Threads are used to parallelize long-running tasks. Optimal threads number is slightly above the number of available cores/processors. Beware that more threads consume more memory.",
        sidetext => __"(more speed but more memory usage)",
        cli     => 'threads|j=i',
        type    => 'i',
        min     => 1,
        max     => 16,
        default => $Slic3r::have_threads ? 2 : 1,
        readonly => !$Slic3r::have_threads,
    },
    'resolution' => {
        label   => __"Resolution",
        tooltip => __"Minimum detail resolution, used to simplify the input file for speeding up the slicing job and reducing memory usage. High-resolution models often carry more detail than printers can render. Set to zero to disable any simplification and use full resolution from input.",
        sidetext => __"mm",
        cli     => 'resolution=f',
        type    => 'f',
        min     => 0,
        default => 0,
    },

    # output options
    'output_filename_format' => {
        label   => __"Output filename format",
        tooltip => __"You can use all configuration options as variables inside this template. For example: [layer_height], [fill_density] etc. You can also use [timestamp], [year], [month], [day], [hour], [minute], [second], [version], [input_filename], [input_filename_base].",
        cli     => 'output-filename-format=s',
        type    => 's',
        full_width => 1,
        default => '[input_filename_base].gcode',
    },

    # printer options
    'print_center' => {
        label   => __"Print center",
        tooltip => __"Enter the G-code coordinates of the point you want to center your print around.",
        sidetext => __"mm",
        cli     => 'print-center=s',
        type    => 'point',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [100,100],
    },
    'gcode_flavor' => {
        label   => __"G-code flavor",
        tooltip => __"Some G/M-code commands, including temperature control and others, are not universal. Set this option to your printer\'s firmware to get a compatible output. The \"No extrusion\" flavor prevents Slic3r from exporting any extrusion value at all.",
        cli     => 'gcode-flavor=s',
        type    => 'select',
        values  => [qw(reprap teacup makerbot sailfish mach3 no-extrusion)],
        labels  => ['RepRap (Marlin/Sprinter)', 'Teacup', 'MakerBot', 'Sailfish', 'Mach3/EMC', 'No extrusion'],
        default => 'reprap',
    },
    'use_relative_e_distances' => {
        label   => __"Use relative E distances",
        tooltip => __"If your firmware requires relative E values, check this, otherwise leave it unchecked. Most firmwares use absolute values.",
        cli     => 'use-relative-e-distances!',
        type    => 'bool',
        default => 0,
    },
    'extrusion_axis' => {
        label   => __"Extrusion axis",
        tooltip => __"Use this option to set the axis letter associated to your printer\'s extruder (usually E but some printers use A).",
        cli     => 'extrusion-axis=s',
        type    => 's',
        default => 'E',
    },
    'z_offset' => {
        label   => __"Z offset",
        tooltip => __"This value will be added (or subtracted) from all the Z coordinates in the output G-code. It is used to compensate for bad Z endstop position: for example, if your endstop zero actually leaves the nozzle 0.3mm far from the print bed, set this to -0.3 (or fix your endstop).",
        sidetext => __"mm",
        cli     => 'z-offset=f',
        type    => 'f',
        default => 0,
    },
    'gcode_arcs' => {
        label   => __"Use native G-code arcs",
        tooltip => __"This experimental feature tries to detect arcs from segments and generates G2/G3 arc commands instead of multiple straight G1 commands.",
        cli     => 'gcode-arcs!',
        type    => 'bool',
        default => 0,
    },
    'g0' => {
        label   => __"Use G0 for travel moves",
        tooltip => __"Only enable this if your firmware supports G0 properly (thus decouples all axes using their maximum speeds instead of synchronizing them). Travel moves and retractions will be combined in single commands, speeding them print up.",
        cli     => 'g0!',
        type    => 'bool',
        default => 0,
    },
    'gcode_comments' => {
        label   => __"Verbose G-code",
        tooltip => __"Enable this to get a commented G-code file, with each line explained by a descriptive text. If you print from SD card, the additional weight of the file could make your firmware slow down.",
        cli     => 'gcode-comments!',
        type    => 'bool',
        default => 0,
    },
    
    # extruders options
    'extruder_offset' => {
        label   => __"Extruder offset",
        tooltip => __"If your firmware doesn\'t handle the extruder displacement you need the G-code to take it into account. This option lets you specify the displacement of each extruder with respect to the first one. It expects positive coordinates (they will be subtracted from the XY coordinate).",
        sidetext => __"mm",
        cli     => 'extruder-offset=s@',
        type    => 'point',
        serialize   => sub { join ',', map { join 'x', @$_ } @{$_[0]} },
        deserialize => sub { [ map [ split /x/, $_ ], (ref $_[0] eq 'ARRAY') ? @{$_[0]} : (split /,/, $_[0] || '0x0') ] },
        default => [[0,0]],
    },
    'nozzle_diameter' => {
        label   => __"Nozzle diameter",
        tooltip => __"This is the diameter of your extruder nozzle (for example: 0.5, 0.35 etc.)",
        cli     => 'nozzle-diameter=f@',
        type    => 'f',
        sidetext => __"mm",
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [0.5],
    },
    'filament_diameter' => {
        label   => __"Diameter",
        tooltip => __"Enter your filament diameter here. Good precision is required, so use a caliper and do multiple measurements along the filament, then compute the average.",
        sidetext => __"mm",
        cli     => 'filament-diameter=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default     => [3],
    },
    'extrusion_multiplier' => {
        label   => __"Extrusion multiplier",
        tooltip => __"This factor changes the amount of flow proportionally. You may need to tweak this setting to get nice surface finish and correct single wall widths. Usual values are between 0.9 and 1.1. If you think you need to change this more, check filament diameter and your firmware E steps.",
        cli     => 'extrusion-multiplier=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [1],
    },
    'temperature' => {
        label   => __"Other layers",
        tooltip => __"Extruder temperature for layers after the first one. Set this to zero to disable temperature control commands in the output.",
        sidetext => __"°C",
        cli     => 'temperature=i@',
        type    => 'i',
        max     => 400,
        serialize   => $serialize_comma,
        deserialize => sub { $_[0] ? [ split /,/, $_[0] ] : [0] },
        default => [200],
    },
    'first_layer_temperature' => {
        label   => __"First layer",
        tooltip => __"Extruder temperature for first layer. If you want to control temperature manually during print, set this to zero to disable temperature control commands in the output file.",
        sidetext => __"°C",
        cli     => 'first-layer-temperature=i@',
        type    => 'i',
        serialize   => $serialize_comma,
        deserialize => sub { $_[0] ? [ split /,/, $_[0] ] : [0] },
        max     => 400,
        default => [200],
    },
    
    # extruder mapping
    'perimeter_extruder' => {
        label   => __"Perimeter extruder",
        tooltip => __"The extruder to use when printing perimeters.",
        cli     => 'perimeter-extruder=i',
        type    => 'i',
        aliases => [qw(perimeters_extruder)],
        default => 1,
    },
    'infill_extruder' => {
        label   => __"Infill extruder",
        tooltip => __"The extruder to use when printing infill.",
        cli     => 'infill-extruder=i',
        type    => 'i',
        default => 1,
    },
    'support_material_extruder' => {
        label   => __"Support material extruder",
        tooltip => __"The extruder to use when printing support material. This affects brim too.",
        cli     => 'support-material-extruder=i',
        type    => 'i',
        default => 1,
    },
    
    # filament options
    'first_layer_bed_temperature' => {
        label   => __"First layer",
        tooltip => __"Heated build plate temperature for the first layer. Set this to zero to disable bed temperature control commands in the output.",
        sidetext => __"°C",
        cli     => 'first-layer-bed-temperature=i',
        type    => 'i',
        max     => 300,
        default => 0,
    },
    'bed_temperature' => {
        label   => __"Other layers",
        tooltip => __"Bed temperature for layers after the first one. Set this to zero to disable bed temperature control commands in the output.",
        sidetext => __"°C",
        cli     => 'bed-temperature=i',
        type    => 'i',
        max     => 300,
        default => 0,
    },
    
    # speed options
    'travel_speed' => {
        label   => __"Travel",
        tooltip => __"Speed for travel moves (jumps between distant extrusion points).",
        sidetext => __"mm/s",
        cli     => 'travel-speed=f',
        type    => 'f',
        aliases => [qw(travel_feed_rate)],
        default => 130,
    },
    'perimeter_speed' => {
        label   => __"Perimeters",
        tooltip => __"Speed for perimeters (contours, aka vertical shells).",
        sidetext => __"mm/s",
        cli     => 'perimeter-speed=f',
        type    => 'f',
        aliases => [qw(perimeter_feed_rate)],
        default => 30,
    },
    'small_perimeter_speed' => {
        label   => __"Small perimeters",
        tooltip => __"This separate setting will affect the speed of perimeters having radius <= 6.5mm (usually holes). If expressed as percentage (for example: 80%) it will be calculated on the perimeters speed setting above.",
        sidetext => __"mm/s or %",
        cli     => 'small-perimeter-speed=s',
        type    => 'f',
        ratio_over => 'perimeter_speed',
        default => 30,
    },
    'external_perimeter_speed' => {
        label   => __"External perimeters",
        tooltip => __"This separate setting will affect the speed of external perimeters (the visible ones). If expressed as percentage (for example: 80%) it will be calculated on the perimeters speed setting above.",
        sidetext => __"mm/s or %",
        cli     => 'external-perimeter-speed=s',
        type    => 'f',
        ratio_over => 'perimeter_speed',
        default => '70%',
    },
    'infill_speed' => {
        label   => __"Infill",
        tooltip => __"Speed for printing the internal fill.",
        sidetext => __"mm/s",
        cli     => 'infill-speed=f',
        type    => 'f',
        aliases => [qw(print_feed_rate infill_feed_rate)],
        default => 60,
    },
    'solid_infill_speed' => {
        label   => __"Solid infill",
        tooltip => __"Speed for printing solid regions (top/bottom/internal horizontal shells). This can be expressed as a percentage (for example: 80%) over the default infill speed above.",
        sidetext => __"mm/s or %",
        cli     => 'solid-infill-speed=s',
        type    => 'f',
        ratio_over => 'infill_speed',
        aliases => [qw(solid_infill_feed_rate)],
        default => 60,
    },
    'top_solid_infill_speed' => {
        label   => __"Top solid infill",
        tooltip => __"Speed for printing top solid regions. You may want to slow down this to get a nicer surface finish. This can be expressed as a percentage (for example: 80%) over the solid infill speed above.",
        sidetext => __"mm/s or %",
        cli     => 'top-solid-infill-speed=s',
        type    => 'f',
        ratio_over => 'solid_infill_speed',
        default => 50,
    },
    'support_material_speed' => {
        label   => __"Support material",
        tooltip => __"Speed for printing support material.",
        sidetext => __"mm/s",
        cli     => 'support-material-speed=f',
        type    => 'f',
        default => 60,
    },
    'bridge_speed' => {
        label   => __"Bridges",
        tooltip => __"Speed for printing bridges.",
        sidetext => __"mm/s",
        cli     => 'bridge-speed=f',
        type    => 'f',
        aliases => [qw(bridge_feed_rate)],
        default => 60,
    },
    'gap_fill_speed' => {
        label   => __"Gap fill",
        tooltip => __"Speed for filling small gaps using short zigzag moves. Keep this reasonably low to avoid too much shaking and resonance issues. Set zero to disable gaps filling.",
        sidetext => __"mm/s",
        cli     => 'gap-fill-speed=f',
        type    => 'f',
        default => 20,
    },
    'first_layer_speed' => {
        label   => __"First layer speed",
        tooltip => __"If expressed as absolute value in mm/s, this speed will be applied to all the print moves of the first layer, regardless of their type. If expressed as a percentage (for example: 40%) it will scale the default speeds.",
        sidetext => __"mm/s or %",
        cli     => 'first-layer-speed=s',
        type    => 'f',
        default => '30%',
    },
    
    # acceleration options
    'default_acceleration' => {
        label   => __"Default",
        tooltip => __"This is the acceleration your printer will be reset to after the role-specific acceleration values are used (perimeter/infill). Set zero to prevent resetting acceleration at all.",
        sidetext => __"mm/s²",
        cli     => 'default-acceleration=f',
        type    => 'f',
        default => 0,
    },
    'perimeter_acceleration' => {
        label   => __"Perimeters",
        tooltip => __"This is the acceleration your printer will use for perimeters. A high value like 9000 usually gives good results if your hardware is up to the job. Set zero to disable acceleration control for perimeters.",
        sidetext => __"mm/s²",
        cli     => 'perimeter-acceleration=f',
        type    => 'f',
        default => 0,
    },
    'infill_acceleration' => {
        label   => __"Infill",
        tooltip => __"This is the acceleration your printer will use for infill. Set zero to disable acceleration control for infill.",
        sidetext => __"mm/s²",
        cli     => 'infill-acceleration=f',
        type    => 'f',
        default => 0,
    },
    'bridge_acceleration' => {
        label   => __"Bridge",
        tooltip => __"This is the acceleration your printer will use for bridges. Set zero to disable acceleration control for bridges.",
        sidetext => __"mm/s²",
        cli     => 'bridge-acceleration=f',
        type    => 'f',
        default => 0,
    },
    
    # accuracy options
    'layer_height' => {
        label   => __"Layer height",
        tooltip => __"This setting controls the height (and thus the total number) of the slices/layers. Thinner layers give better accuracy but take more time to print.",
        sidetext => 'mm',
        cli     => 'layer-height=f',
        type    => 'f',
        default => 0.4,
    },
    'first_layer_height' => {
        label   => __"First layer height",
        tooltip => __"When printing with very low layer heights, you might still want to print a thicker bottom layer to improve adhesion and tolerance for non perfect build plates. This can be expressed as an absolute value or as a percentage (for example: 150%) over the default layer height.",
        sidetext => __"mm or %",
        cli     => 'first-layer-height=s',
        type    => 'f',
        ratio_over => 'layer_height',
        default => 0.35,
    },
    'infill_every_layers' => {
        label   => __"Infill every",
        tooltip => __"This feature allows to combine infill and speed up your print by extruding thicker infill layers while preserving thin perimeters, thus accuracy.",
        sidetext => __"layers",
        cli     => 'infill-every-layers=i',
        type    => 'i',
        min     => 1,
        default => 1,
    },
    'solid_infill_every_layers' => {
        label   => __"Solid infill every",
        tooltip => __"This feature allows to force a solid layer every given number of layers. Zero to disable.",
        sidetext => __"layers",
        cli     => 'solid-infill-every-layers=i',
        type    => 'i',
        min     => 0,
        default => 0,
    },
    'infill_only_where_needed' => {
        label   => __"Only infill where needed",
        tooltip => __"This option will limit infill to the areas actually needed for supporting ceilings (it will act as internal support material).",
        cli     => 'infill-only-where-needed!',
        type    => 'bool',
        default => 0,
    },
    'infill_first' => {
        label   => __"Infill before perimeters",
        tooltip => __"This option will switch the print order of perimeters and infill, making the latter first.",
        cli     => 'infill-first!',
        type    => 'bool',
        default => 0,
    },
    
    # flow options
    'extrusion_width' => {
        label   => __"Default extrusion width",
        tooltip => __"Set this to a non-zero value to set a manual extrusion width. If left to zero, Slic3r calculates a width automatically. If expressed as percentage (for example: 230%) it will be computed over layer height.",
        sidetext => __"mm or % (leave 0 for auto)",
        cli     => 'extrusion-width=s',
        type    => 'f',
        default => 0,
    },
    'first_layer_extrusion_width' => {
        label   => __"First layer",
        tooltip => __"Set this to a non-zero value to set a manual extrusion width for first layer. You can use this to force fatter extrudates for better adhesion. If expressed as percentage (for example 120%) if will be computed over layer height.",
        sidetext => __"mm or % (leave 0 for default)",
        cli     => 'first-layer-extrusion-width=s',
        type    => 'f',
        default => '200%',
    },
    'perimeter_extrusion_width' => {
        label   => __"Perimeters",
        tooltip => __"Set this to a non-zero value to set a manual extrusion width for perimeters. You may want to use thinner extrudates to get more accurate surfaces. If expressed as percentage (for example 90%) if will be computed over layer height.",
        sidetext => __"mm or % (leave 0 for default)",
        cli     => 'perimeter-extrusion-width=s',
        type    => 'f',
        aliases => [qw(perimeters_extrusion_width)],
        default => 0,
    },
    'infill_extrusion_width' => {
        label   => __"Infill",
        tooltip => __"Set this to a non-zero value to set a manual extrusion width for infill. You may want to use fatter extrudates to speed up the infill and make your parts stronger. If expressed as percentage (for example 90%) if will be computed over layer height.",
        sidetext => __"mm or % (leave 0 for default)",
        cli     => 'infill-extrusion-width=s',
        type    => 'f',
        default => 0,
    },
    'solid_infill_extrusion_width' => {
        label   => __"Solid infill",
        tooltip => __"Set this to a non-zero value to set a manual extrusion width for infill for solid surfaces. If expressed as percentage (for example 90%) if will be computed over layer height.",
        sidetext => __"mm or % (leave 0 for default)",
        cli     => 'solid-infill-extrusion-width=s',
        type    => 'f',
        default => 0,
    },
    'top_infill_extrusion_width' => {
        label   => __"Top solid infill",
        tooltip => __"Set this to a non-zero value to set a manual extrusion width for infill for top surfaces. You may want to use thinner extrudates to fill all narrow regions and get a smoother finish. If expressed as percentage (for example 90%) if will be computed over layer height.",
        sidetext => __"mm or % (leave 0 for default)",
        cli     => 'top-infill-extrusion-width=s',
        type    => 'f',
        default => 0,
    },
    'support_material_extrusion_width' => {
        label   => 'Support material',
        tooltip => 'Set this to a non-zero value to set a manual extrusion width for support material. If expressed as percentage (for example 90%) if will be computed over layer height.',
        sidetext => 'mm or % (leave 0 for default)',
        cli     => 'support-material-extrusion-width=s',
        type    => 'f',
        default => 0,
    },
    'bridge_flow_ratio' => {
        label   => 'Bridge flow ratio',
        tooltip => 'This factor affects the amount of plastic for bridging. You can decrease it slightly to pull the extrudates and prevent sagging, although default settings are usually good and you should experiment with cooling (use a fan) before tweaking this.',
        cli     => 'bridge-flow-ratio=f',
        type    => 'f',
        default => 1,
    },
    'vibration_limit' => {
        label   => 'Vibration limit',
        tooltip => 'This experimental option will slow down those moves hitting the configured frequency limit. The purpose of limiting vibrations is to avoid mechanical resonance. Set zero to disable.',
        sidetext => 'Hz',
        cli     => 'vibration-limit=f',
        type    => 'f',
        default => 0,
    },
    
    # print options
    'perimeters' => {
        label   => __"Perimeters (minimum)",
        tooltip => __"This option sets the number of perimeters to generate for each layer. Note that Slic3r will increase this number automatically when it detects sloping surfaces which benefit from a higher number of perimeters.",
        cli     => 'perimeters=i',
        type    => 'i',
        aliases => [qw(perimeter_offsets)],
        default => 3,
    },
    'solid_layers' => {
        label   => __"Solid layers",
        tooltip => __"Number of solid layers to generate on top and bottom surfaces.",
        cli     => 'solid-layers=i',
        type    => 'i',
        shortcut => [qw(top_solid_layers bottom_solid_layers)],
    },
    'top_solid_layers' => {
        label   => __"Top",
        tooltip => __"Number of solid layers to generate on top surfaces.",
        cli     => 'top-solid-layers=i',
        type    => 'i',
        default => 3,
    },
    'bottom_solid_layers' => {
        label   => __"Bottom",
        tooltip => __"Number of solid layers to generate on bottom surfaces.",
        cli     => 'bottom-solid-layers=i',
        type    => 'i',
        default => 3,
    },
    'fill_pattern' => {
        label   => __"Fill pattern",
        tooltip => __"Fill pattern for general low-density infill.",
        cli     => 'fill-pattern=s',
        type    => 'select',
        values  => [qw(rectilinear line concentric honeycomb hilbertcurve archimedeanchords octagramspiral)],
        labels  => [qw(rectilinear line concentric honeycomb), 'hilbertcurve (slow)', 'archimedeanchords (slow)', 'octagramspiral (slow)'],
        default => 'honeycomb',
    },
    'solid_fill_pattern' => {
        label   => __"Top/bottom fill pattern",
        tooltip => __"Fill pattern for top/bottom infill.",
        cli     => 'solid-fill-pattern=s',
        type    => 'select',
        values  => [qw(rectilinear concentric hilbertcurve archimedeanchords octagramspiral)],
        labels  => [qw(rectilinear concentric), 'hilbertcurve (slow)', 'archimedeanchords (slow)', 'octagramspiral (slow)'],
        default => 'rectilinear',
    },
    'fill_density' => {
        label   => __"Fill density",
        tooltip => __"Density of internal infill, expressed in the range 0 - 1.",
        cli     => 'fill-density=f',
        type    => 'f',
        default => 0.4,
    },
    'fill_angle' => {
        label   => __"Fill angle",
        tooltip => __"Default base angle for infill orientation. Cross-hatching will be applied to this. Bridges will be infilled using the best direction Slic3r can detect, so this setting does not affect them.",
        sidetext => __"°",
        cli     => 'fill-angle=i',
        type    => 'i',
        max     => 359,
        default => 45,
    },
    'solid_infill_below_area' => {
        label   => __"Solid infill threshold area",
        tooltip => __"Force solid infill for regions having a smaller area than the specified threshold.",
        sidetext => __"mm²",
        cli     => 'solid-infill-below-area=f',
        type    => 'f',
        default => 70,
    },
    'extra_perimeters' => {
        label   => __"Generate extra perimeters when needed",
        tooltip => __"Add more perimeters when needed for avoiding gaps in sloping walls.",
        cli     => 'extra-perimeters!',
        type    => 'bool',
        default => 1,
    },
    'randomize_start' => {
        label   => __"Randomize starting points",
        tooltip => __"Start each layer from a different vertex to prevent plastic build-up on the same corner.",
        cli     => 'randomize-start!',
        type    => 'bool',
        default => 0,
    },
    'avoid_crossing_perimeters' => {
        label   => __"Avoid crossing perimeters",
        tooltip => __"Optimize travel moves in order to minimize the crossing of perimeters. This is mostly useful with Bowden extruders which suffer from oozing. This feature slows down both the print and the G-code generation.",
        cli     => 'avoid-crossing-perimeters!',
        type    => 'bool',
        default => 0,
    },
    'external_perimeters_first' => {
        label   => __"External perimeters first",
        tooltip => __"Print contour perimeters from the outermost one to the innermost one instead of the default inverse order.",
        cli     => 'external-perimeters-first!',
        type    => 'bool',
        default => 0,
    },
    'spiral_vase' => {
        label   => 'Spiral vase',
        tooltip => 'This experimental feature will raise Z gradually while printing a single-walled object in order to remove any visible seam. By enabling this option other settings will be overridden to enforce a single perimeter, no infill, no top solid layers, no support material. You can still set any number of bottom solid layers as well as skirt/brim loops. It won\'t work when printing more than an object.',
        cli     => 'spiral-vase!',
        type    => 'bool',
        default => 0,
    },
    'only_retract_when_crossing_perimeters' => {
        label   => __"Only retract when crossing perimeters",
        tooltip => __"Disables retraction when travelling between infill paths inside the same island.",
        cli     => 'only-retract-when-crossing-perimeters!',
        type    => 'bool',
        default => 1,
    },
    'support_material' => {
        label   => __"Generate support material",
        tooltip => __"Enable support material generation.",
        cli     => 'support-material!',
        type    => 'bool',
        default => 0,
    },
    'support_material_threshold' => {
        label   => __"Overhang threshold",
        tooltip => __"Support material will not generated for overhangs whose slope angle is above the given threshold. Set to zero for automatic detection.",
        sidetext => __"°",
        cli     => 'support-material-threshold=i',
        type    => 'i',
        default => 0,
    },
    'support_material_pattern' => {
        label   => __"Pattern",
        tooltip => __"Pattern used to generate support material.",
        cli     => 'support-material-pattern=s',
        type    => 'select',
        values  => [qw(rectilinear rectilinear-grid honeycomb)],
        labels  => ['rectilinear', 'rectilinear grid', 'honeycomb'],
        default => 'rectilinear',
    },
    'support_material_spacing' => {
        label   => __"Pattern spacing",
        tooltip => __"Spacing between support material lines.",
        sidetext => __"mm",
        cli     => 'support-material-spacing=f',
        type    => 'f',
        default => 2.5,
    },
    'support_material_angle' => {
        label   => __"Pattern angle",
        tooltip => __"Use this setting to rotate the support material pattern on the horizontal plane.",
        sidetext => __"°",
        cli     => 'support-material-angle=i',
        type    => 'i',
        default => 0,
    },
    'support_material_interface_layers' => {
        label   => __"Interface layers",
        tooltip => __"Number of interface layers to insert between the object(s) and support material.",
        sidetext => 'layers',
        cli     => 'support-material-interface-layers=i',
        type    => 'i',
        default => 0,
    },
    'support_material_interface_spacing' => {
        label   => __"Interface pattern spacing",
        tooltip => __"Spacing between interface lines. Set zero to get a solid interface.",
        sidetext => __"mm",
        cli     => 'support-material-interface-spacing=f',
        type    => 'f',
        default => 0,
    },
    'support_material_enforce_layers' => {
        label   => __"Enforce support for the first",
        tooltip => __"Generate support material for the specified number of layers counting from bottom, regardless of whether normal support material is enabled or not and regardless of any angle threshold. This is useful for getting more adhesion of objects having a very thin or poor footprint on the build plate.",
        sidetext => 'layers',
        cli     => 'support-material-enforce-layers=f',
        type    => 'i',
        default => 0,
    },
    'raft_layers' => {
        label   => __"Raft layers",
        tooltip => __"Number of total raft layers to insert below the object(s).",
        sidetext => 'layers',
        cli     => 'raft-layers=i',
        type    => 'i',
        default => 0,
    },
    'start_gcode' => {
        label   => __"Start G-code",
        tooltip => __"This start procedure is inserted at the beginning of the output file, right after the temperature control commands for extruder and bed. If Slic3r detects M104 or M190 in your custom codes, such commands will not be prepended automatically. Note that you can use placeholder variables for all Slic3r settings, so you can put a \"M104 S[first_layer_temperature]\" command wherever you want.",
        cli     => 'start-gcode=s',
        type    => 's',
        multiline => 1,
        full_width => 1,
        height  => 120,
        serialize   => sub { join '\n', split /\R+/, $_[0] },
        deserialize => sub { join "\n", split /\\n/, $_[0] },
        default => <<'END',
G28 ; home all axes
G1 Z5 F5000 ; lift nozzle
END
    },
    'end_gcode' => {
        label   => __"End G-code",
        tooltip => __"This end procedure is inserted at the end of the output file. Note that you can use placeholder variables for all Slic3r settings.",
        cli     => 'end-gcode=s',
        type    => 's',
        multiline => 1,
        full_width => 1,
        height  => 120,
        serialize   => sub { join '\n', split /\R+/, $_[0] },
        deserialize => sub { join "\n", split /\\n/, $_[0] },
        default => <<'END',
M104 S0 ; turn off temperature
G28 X0  ; home X axis
M84     ; disable motors
END
    },
    'layer_gcode' => {
        label   => __"Layer change G-code",
        tooltip => __"This custom code is inserted at every layer change, right after the Z move and before the extruder moves to the first layer point. Note that you can use placeholder variables for all Slic3r settings.",
        cli     => 'layer-gcode=s',
        type    => 's',
        multiline => 1,
        full_width => 1,
        height  => 50,
        serialize   => sub { join '\n', split /\R+/, $_[0] },
        deserialize => sub { join "\n", split /\\n/, $_[0] },
        default => '',
    },
    'toolchange_gcode' => {
        label   => __"Tool change G-code",
        tooltip => __"This custom code is inserted at every extruder change. Note that you can use placeholder variables for all Slic3r settings as well as [previous_extruder] and [next_extruder].",
        cli     => 'toolchange-gcode=s',
        type    => 's',
        multiline => 1,
        full_width => 1,
        height  => 50,
        serialize   => sub { join '\n', split /\R+/, $_[0] },
        deserialize => sub { join "\n", split /\\n/, $_[0] },
        default => '',
    },
    'post_process' => {
        label   => __"Post-processing scripts",
        tooltip => __"If you want to process the output G-code through custom scripts, just list their absolute paths here. Separate multiple scripts with a semicolon. Scripts will be passed the absolute path to the G-code file as the first argument, and they can access the Slic3r config settings by reading environment variables.",
        cli     => 'post-process=s@',
        type    => 's@',
        multiline => 1,
        full_width => 1,
        height  => 60,
        serialize   => sub { join '; ', @{$_[0]} },
        deserialize => sub { [ split /\s*(?:;|\R)\s*/s, $_[0] ] },
        default => [],
    },
    
    # retraction options
    'retract_length' => {
        label   => __"Length",
        tooltip => __"When retraction is triggered, filament is pulled back by the specified amount (the length is measured on raw filament, before it enters the extruder).",
        sidetext => __"mm (zero to disable)",
        cli     => 'retract-length=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [1],
    },
    'retract_speed' => {
        label   => __"Speed",
        tooltip => __"The speed for retractions (it only applies to the extruder motor).",
        sidetext => __"mm/s",
        cli     => 'retract-speed=f@',
        type    => 'i',
        max     => 1000,
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [30],
    },
    'retract_restart_extra' => {
        label   => __"Extra length on restart",
        tooltip => __"When the retraction is compensated after the travel move, the extruder will push this additional amount of filament. This setting is rarely needed.",
        sidetext => __"mm",
        cli     => 'retract-restart-extra=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [0],
    },
    'retract_before_travel' => {
        label   => __"Minimum travel after retraction",
        tooltip => __"Retraction is not triggered when travel moves are shorter than this length.",
        sidetext => __"mm",
        cli     => 'retract-before-travel=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [2],
    },
    'retract_lift' => {
        label   => __"Lift Z",
        tooltip => __"If you set this to a positive value, Z is quickly raised every time a retraction is triggered.",
        sidetext => __"mm",
        cli     => 'retract-lift=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [0],
    },
    'retract_layer_change' => {
        label   => __"Retract on layer change",
        tooltip => __"This flag enforces a retraction whenever a Z move is done.",
        cli     => 'retract-layer-change!',
        type    => 'bool',
        serialize   => $serialize_comma_bool,
        deserialize => $deserialize_comma,
        default => [1],
    },
    'wipe' => {
        label   => __"Wipe before retract",
        tooltip => __"This flag will move the nozzle while retracting to minimize the possible blob on leaky extruders.",
        cli     => 'wipe!',
        type    => 'bool',
        serialize   => $serialize_comma_bool,
        deserialize => $deserialize_comma,
        default => [0],
    },
    'retract_length_toolchange' => {
        label   => __"Length",
        tooltip => __"When retraction is triggered before changing tool, filament is pulled back by the specified amount (the length is measured on raw filament, before it enters the extruder).",
        sidetext => __"mm (zero to disable)",
        cli     => 'retract-length-toolchange=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [10],
    },
    'retract_restart_extra_toolchange' => {
        label   => __"Extra length on restart",
        tooltip => __"When the retraction is compensated after changing tool, the extruder will push this additional amount of filament.",
        sidetext => __"mm",
        cli     => 'retract-restart-extra-toolchange=f@',
        type    => 'f',
        serialize   => $serialize_comma,
        deserialize => $deserialize_comma,
        default => [0],
    },
    
    # cooling options
    'cooling' => {
        label   => __"Enable auto cooling",
        tooltip => __"This flag enables the automatic cooling logic that adjusts print speed and fan speed according to layer printing time.",
        cli     => 'cooling!',
        type    => 'bool',
        default => 1,
    },
    'min_fan_speed' => {
        label   =>  __"Min",
        tooltip => __"This setting represents the minimum PWM your fan needs to work.",
        sidetext => __"%",
        cli     => 'min-fan-speed=i',
        type    => 'i',
        max     => 1000,
        default => 35,
    },
    'max_fan_speed' => {
        label   => __"Max",
        tooltip => __"This setting represents the maximum speed of your fan.",
        sidetext => __"%",
        cli     => 'max-fan-speed=i',
        type    => 'i',
        max     => 1000,
        default => 100,
    },
    'bridge_fan_speed' => {
        label   => __"Bridges fan speed",
        tooltip => __"This fan speed is enforced during all bridges.",
        sidetext => __"%",
        cli     => 'bridge-fan-speed=i',
        type    => 'i',
        max     => 1000,
        default => 100,
    },
    'fan_below_layer_time' => {
        label   => __"Enable fan if layer print time is below",
        tooltip => __"If layer print time is estimated below this number of seconds, fan will be enabled and its speed will be calculated by interpolating the minimum and maximum speeds.",
        sidetext => __"approximate seconds",
        cli     => 'fan-below-layer-time=i',
        type    => 'i',
        max     => 1000,
        width   => 60,
        default => 60,
    },
    'slowdown_below_layer_time' => {
        label   => __"Slow down if layer print time is below",
        tooltip => __"If layer print time is estimated below this number of seconds, print moves speed will be scaled down to extend duration to this value.",
        sidetext => __"approximate seconds",
        cli     => 'slowdown-below-layer-time=i',
        type    => 'i',
        max     => 1000,
        width   => 60,
        default => 30,
    },
    'min_print_speed' => {
        label   => __"Min print speed",
        tooltip => __"Slic3r will not scale speed down below this speed.",
        sidetext => __"mm/s",
        cli     => 'min-print-speed=f',
        type    => 'i',
        max     => 1000,
        default => 10,
    },
    'disable_fan_first_layers' => {
        label   => __"Disable fan for the first",
        tooltip => __"You can set this to a positive value to disable fan at all during the first layers, so that it does not make adhesion worse.",
        sidetext => __"layers",
        cli     => 'disable-fan-first-layers=i',
        type    => 'i',
        max     => 1000,
        default => 1,
    },
    'fan_always_on' => {
        label   => __"Keep fan always on",
        tooltip => __"If this is enabled, fan will never be disabled and will be kept running at least at its minimum speed. Useful for PLA, harmful for ABS.",
        cli     => 'fan-always-on!',
        type    => 'bool',
        default => 0,
    },
    
    # skirt/brim options
    'skirts' => {
        label   => __"Loops",
        tooltip => __"Number of loops for this skirt, in other words its thickness. Set this to zero to disable skirt.",
        cli     => 'skirts=i',
        type    => 'i',
        default => 1,
    },
    'min_skirt_length' => {
        label   => __"Minimum extrusion length",
        tooltip => __"Generate no less than the number of skirt loops required to consume the specified amount of filament on the bottom layer. For multi-extruder machines, this minimum applies to each extruder.",
        sidetext => __"mm",
        cli     => 'min-skirt-length=f',
        type    => 'f',
        default => 0,
        min     => 0,
    },
    'skirt_distance' => {
        label   => __"Distance from object",
        tooltip => __"Distance between skirt and object(s). Set this to zero to attach the skirt to the object(s) and get a brim for better adhesion.",
        sidetext => __"mm",
        cli     => 'skirt-distance=f',
        type    => 'f',
        default => 6,
    },
    'skirt_height' => {
        label   => __"Skirt height",
        tooltip => __"Height of skirt expressed in layers. Set this to a tall value to use skirt as a shield against drafts.",
        sidetext => __"layers",
        cli     => 'skirt-height=i',
        type    => 'i',
        default => 1,
    },
    'brim_width' => {
        label   => __"Brim width",
        tooltip => __"Horizontal width of the brim that will be printed around each object on the first layer.",
        sidetext => __"mm",
        cli     => 'brim-width=f',
        type    => 'f',
        default => 0,
    },
    
    # transform options
    'scale' => {
        label   => __"Scale",
        cli     => 'scale=f',
        type    => 'f',
        default => 1,
    },
    'rotate' => {
        label   => __"Rotate",
        sidetext => __"°",
        cli     => 'rotate=i',
        type    => 'i',
        max     => 359,
        default => 0,
    },
    'duplicate' => {
        label   => __"Copies (autoarrange)",
        cli     => 'duplicate=i',
        type    => 'i',
        min     => 1,
        default => 1,
    },
    'bed_size' => {
        label   => __"Bed size",
        tooltip => __"Size of your bed. This is used to adjust the preview in the plater and for auto-arranging parts in it.",
        sidetext => __"mm",
        cli     => 'bed-size=s',
        type    => 'point',
        serialize   => $serialize_comma,
        deserialize => sub { [ split /[,x]/, $_[0] ] },
        default => [200,200],
    },
    'duplicate_grid' => {
        label   => __"Copies (grid)",
        cli     => 'duplicate-grid=s',
        type    => 'point',
        serialize   => $serialize_comma,
        deserialize => sub { [ split /[,x]/, $_[0] ] },
        default => [1,1],
    },
    'duplicate_distance' => {
        label   => __"Distance between copies",
        tooltip => __"Distance used for the auto-arrange feature of the plater.",
        sidetext => __"mm",
        cli     => 'duplicate-distance=f',
        type    => 'f',
        aliases => [qw(multiply_distance)],
        default => 6,
    },
    
    # sequential printing options
    'complete_objects' => {
        label   => __"Complete individual objects",
        tooltip => __"When printing multiple objects or copies, this feature will complete each object before moving onto next one (and starting it from its bottom layer). This feature is useful to avoid the risk of ruined prints. Slic3r should warn and prevent you from extruder collisions, but beware.",
        cli     => 'complete-objects!',
        type    => 'bool',
        default => 0,
    },
    'extruder_clearance_radius' => {
        label   => __"Radius",
        tooltip => __"Set this to the clearance radius around your extruder. If the extruder is not centered, choose the largest value for safety. This setting is used to check for collisions and to display the graphical preview in the plater.",
        sidetext => __"mm",
        cli     => 'extruder-clearance-radius=f',
        type    => 'f',
        default => 20,
    },
    'extruder_clearance_height' => {
        label   => __"Height",
        tooltip => __"Set this to the vertical distance between your nozzle tip and (usually) the X carriage rods. In other words, this is the height of the clearance cylinder around your extruder, and it represents the maximum depth the extruder can peek before colliding with other printed objects.",
        sidetext => __"mm",
        cli     => 'extruder-clearance-height=f',
        type    => 'f',
        default => 20,
    },
};

# generate accessors
if (eval "use Class::XSAccessor; 1") {
    Class::XSAccessor->import(
        getters => { map { $_ => $_ } keys %$Options },
    );
} else {
    no strict 'refs';
    for my $opt_key (keys %$Options) {
        *{$opt_key} = sub { $_[0]{$opt_key} };
    }
}

sub new {
    my $class = shift;
    my %args = @_;
    
    my $self = bless {}, $class;
    $self->apply(%args);
    return $self;
}

sub new_from_defaults {
    my $class = shift;
    my @opt_keys = 
    return $class->new(
        map { $_ => $Options->{$_}{default} }
            grep !$Options->{$_}{shortcut},
            (@_ ? @_ : keys %$Options)
    );
}

sub new_from_cli {
    my $class = shift;
    my %args = @_;
    
    delete $args{$_} for grep !defined $args{$_}, keys %args;
    
    for (qw(start end layer toolchange)) {
        my $opt_key = "${_}_gcode";
        if ($args{$opt_key}) {
            die "Invalid value for --${_}-gcode: file does not exist\n"
                if !-e $args{$opt_key};
            Slic3r::open(\my $fh, "<", $args{$opt_key})
                or die "Failed to open $args{$opt_key}\n";
            binmode $fh, ':utf8';
            $args{$opt_key} = do { local $/; <$fh> };
            close $fh;
        }
    }
    
    $args{$_} = $Options->{$_}{deserialize}->($args{$_})
        for grep exists $args{$_}, qw(print_center bed_size duplicate_grid extruder_offset retract_layer_change wipe);
    
    return $class->new(%args);
}

sub merge {
    my $class = shift;
    my $config = $class->new;
    $config->apply($_) for @_;
    return $config;
}

sub load {
    my $class = shift;
    my ($file) = @_;
    
    my $ini = __PACKAGE__->read_ini($file);
    my $config = __PACKAGE__->new;
    $config->set($_, $ini->{_}{$_}, 1) for keys %{$ini->{_}};
    return $config;
}

sub apply {
    my $self = shift;
    my %args = @_ == 1 ? %{$_[0]} : @_; # accept a single Config object too
    
    $self->set($_, $args{$_}) for keys %args;
}

sub clone {
    my $self = shift;
    my $new = __PACKAGE__->new(%$self);
    $new->{$_} = [@{$new->{$_}}] for grep { ref $new->{$_} eq 'ARRAY' } keys %$new;
    return $new;
}

sub get_value {
    my $self = shift;
    my ($opt_key) = @_;
    
    no strict 'refs';
    my $value = $self->get($opt_key);
    $value = $self->get_value($Options->{$opt_key}{ratio_over}) * $1/100
        if $Options->{$opt_key}{ratio_over} && $value =~ /^(\d+(?:\.\d+)?)%$/;
    return $value;
}

sub get {
    my $self = shift;
    my ($opt_key) = @_;
    
    return $self->{$opt_key};
}

sub set {
    my $self = shift;
    my ($opt_key, $value, $deserialize) = @_;
    
    # handle legacy options
    return if $opt_key ~~ @Ignore;
    if ($opt_key =~ /^(extrusion_width|bottom_layer_speed|first_layer_height)_ratio$/) {
        $opt_key = $1;
        $opt_key =~ s/^bottom_layer_speed$/first_layer_speed/;
        $value = $value =~ /^\d+(?:\.\d+)?$/ && $value != 0 ? ($value*100) . "%" : 0;
    }
    if ($opt_key eq 'threads' && !$Slic3r::have_threads) {
        $value = 1;
    }
    
    # For historical reasons, the world's full of configs having these very low values;
    # to avoid unexpected behavior we need to ignore them.  Banning these two hard-coded
    # values is a dirty hack and will need to be removed sometime in the future, but it
    # will avoid lots of complaints for now.
    if ($opt_key eq 'perimeter_acceleration' && $value == '25') {
        $value = 0;
    }
    if ($opt_key eq 'infill_acceleration' && $value == '50') {
        $value = 0;
    }
    
    if (!exists $Options->{$opt_key}) {
        my @keys = grep { $Options->{$_}{aliases} && grep $_ eq $opt_key, @{$Options->{$_}{aliases}} } keys %$Options;
        if (!@keys) {
            warn __("Unknown option")." $opt_key\n";
            return;
        }
        $opt_key = $keys[0];
    }
    
    # clone arrayrefs
    $value = [@$value] if ref $value eq 'ARRAY';
    
    # deserialize if requested
    $value = $Options->{$opt_key}{deserialize}->($value)
        if $deserialize && $Options->{$opt_key}{deserialize};
    
    $self->{$opt_key} = $value;
    
    if ($Options->{$opt_key}{shortcut}) {
        $self->set($_, $value, $deserialize) for @{$Options->{$opt_key}{shortcut}};
    }
}

sub set_ifndef {
    my $self = shift;
    my ($opt_key, $value, $deserialize) = @_;
    
    $self->set($opt_key, $value, $deserialize)
        if !defined $self->get($opt_key);
}

sub has {
    my $self = shift;
    my ($opt_key) = @_;
    return exists $self->{$opt_key};
}

sub serialize {
    my $self = shift;
    my ($opt_key) = @_;
    
    my $value = $self->get($opt_key);
    $value = $Options->{$opt_key}{serialize}->($value) if $Options->{$opt_key}{serialize};
    return $value;
}

sub save {
    my $self = shift;
    my ($file) = @_;
    
    my $ini = { _ => {} };
    foreach my $opt_key (sort keys %$self) {
        next if $Options->{$opt_key}{shortcut};
        next if $Options->{$opt_key}{gui_only};
        $ini->{_}{$opt_key} = $self->serialize($opt_key);
    }
    __PACKAGE__->write_ini($file, $ini);
}

sub setenv {
    my $self = shift;
    
    foreach my $opt_key (sort keys %$Options) {
        next if $Options->{$opt_key}{gui_only};
        $ENV{"SLIC3R_" . uc $opt_key} = $self->serialize($opt_key);
    }
}

# this method is idempotent by design
sub validate {
    my $self = shift;
    
    # -j, --threads
    die __"Invalid value for --threads\n"
        if $self->threads < 1;
    die __"Your perl wasn't built with multithread support\n"
        if $self->threads > 1 && !$Slic3r::have_threads;

    # --layer-height
    die __"Invalid value for --layer-height\n"
        if $self->layer_height <= 0;
    die __"--layer-height must be a multiple of print resolution\n"
        if $self->layer_height / &Slic3r::SCALING_FACTOR % 1 != 0;
    
    # --first-layer-height
    die __"Invalid value for --first-layer-height\n"
        if $self->first_layer_height !~ /^(?:\d*(?:\.\d+)?)%?$/;
    
    # --filament-diameter
    die __"Invalid value for --filament-diameter\n"
        if grep $_ < 1, @{$self->filament_diameter};
    
    # --nozzle-diameter
    die __"Invalid value for --nozzle-diameter\n"
        if grep $_ < 0, @{$self->nozzle_diameter};
    die __"--layer-height can't be greater than --nozzle-diameter\n"
        if grep $self->layer_height > $_, @{$self->nozzle_diameter};
    die "First layer height can't be greater than --nozzle-diameter\n"
        if grep $self->get_value('first_layer_height') > $_, @{$self->nozzle_diameter};
    
    # --perimeters
    die __"Invalid value for --perimeters\n"
        if $self->perimeters < 0;
    
    # --solid-layers
    die __"Invalid value for --solid-layers\n" if defined $self->solid_layers && $self->solid_layers < 0;
    die __"Invalid value for --top-solid-layers\n"    if $self->top_solid_layers      < 0;
    die __"Invalid value for --bottom-solid-layers\n" if $self->bottom_solid_layers   < 0;
    
    # --print-center
    die __"Invalid value for --print-center\n"
        if !ref $self->print_center 
            && (!$self->print_center || $self->print_center !~ /^\d+,\d+$/);
    
    # --fill-pattern
    die __"Invalid value for --fill-pattern\n"
        if !first { $_ eq $self->fill_pattern } @{$Options->{fill_pattern}{values}};
    
    # --solid-fill-pattern
    die __"Invalid value for --solid-fill-pattern\n"
        if !first { $_ eq $self->solid_fill_pattern } @{$Options->{solid_fill_pattern}{values}};
    
    # --fill-density
    die __"Invalid value for --fill-density\n"
        if $self->fill_density < 0 || $self->fill_density > 1;
    die __"The selected fill pattern is not supposed to work at 100% density\n"
        if $self->fill_density == 1
            && !first { $_ eq $self->fill_pattern } @{$Options->{solid_fill_pattern}{values}};
    
    # --infill-every-layers
    die __"Invalid value for --infill-every-layers\n"
        if $self->infill_every_layers !~ /^\d+$/ || $self->infill_every_layers < 1;
    
    # --scale
    die __"Invalid value for --scale\n"
        if $self->scale <= 0;
    
    # --bed-size
    die __"Invalid value for --bed-size\n"
        if !ref $self->bed_size 
            && (!$self->bed_size || $self->bed_size !~ /^\d+,\d+$/);
    
    # --duplicate-grid
    die __"Invalid value for --duplicate-grid\n"
        if !ref $self->duplicate_grid 
            && (!$self->duplicate_grid || $self->duplicate_grid !~ /^\d+,\d+$/);
    
    # --duplicate
    die __"Invalid value for --duplicate or --duplicate-grid\n"
        if !$self->duplicate || $self->duplicate < 1 || !$self->duplicate_grid
            || (grep !$_, @{$self->duplicate_grid});
    die __"Use either --duplicate or --duplicate-grid (using both doesn't make sense)\n"
        if $self->duplicate > 1 && $self->duplicate_grid && (grep $_ && $_ > 1, @{$self->duplicate_grid});
    
    # --skirt-height
    die __"Invalid value for --skirt-height\n"
        if $self->skirt_height < 0;
    
    # --bridge-flow-ratio
    die __"Invalid value for --bridge-flow-ratio\n"
        if $self->bridge_flow_ratio <= 0;
    
    # extruder clearance
    die __"Invalid value for --extruder-clearance-radius\n"
        if $self->extruder_clearance_radius <= 0;
    die __"Invalid value for --extruder-clearance-height\n"
        if $self->extruder_clearance_height <= 0;
    
    # --extrusion-multiplier
    die "Invalid value for --extrusion-multiplier\n"
        if defined first { $_ <= 0 } @{$self->extrusion_multiplier};
}

sub replace_options {
    my $self = shift;
    my ($string, $more_variables) = @_;
    
    $more_variables ||= {};
    $more_variables->{$_} = $ENV{$_} for grep /^SLIC3R_/, keys %ENV;
    {
        my $variables_regex = join '|', keys %$more_variables;
        $string =~ s/\[($variables_regex)\]/$more_variables->{$1}/eg;
    }
    
    my @lt = localtime; $lt[5] += 1900; $lt[4] += 1;
    $string =~ s/\[timestamp\]/sprintf '%04d%02d%02d-%02d%02d%02d', @lt[5,4,3,2,1,0]/egx;
    $string =~ s/\[year\]/$lt[5]/eg;
    $string =~ s/\[month\]/$lt[4]/eg;
    $string =~ s/\[day\]/$lt[3]/eg;
    $string =~ s/\[hour\]/$lt[2]/eg;
    $string =~ s/\[minute\]/$lt[1]/eg;
    $string =~ s/\[second\]/$lt[0]/eg;
    $string =~ s/\[version\]/$Slic3r::VERSION/eg;
    
    # build a regexp to match the available options
    my @options = grep !$Slic3r::Config::Options->{$_}{multiline},
        grep $self->has($_),
        keys %{$Slic3r::Config::Options};
    my $options_regex = join '|', @options;
    
    # use that regexp to search and replace option names with option values
    $string =~ s/\[($options_regex)\]/$self->serialize($1)/eg;
    foreach my $opt_key (grep ref $self->$_ eq 'ARRAY', @options) {
        my $value = $self->$opt_key;
        $string =~ s/\[${opt_key}_${_}\]/$value->[$_]/eg for 0 .. $#$value;
        if ($Options->{$opt_key}{type} eq 'point') {
            $string =~ s/\[${opt_key}_X\]/$value->[0]/eg;
            $string =~ s/\[${opt_key}_Y\]/$value->[1]/eg;
        }
    }
    return $string;
}

# min object distance is max(duplicate_distance, clearance_radius)
sub min_object_distance {
    my $self = shift;
    
    return ($self->complete_objects && $self->extruder_clearance_radius > $self->duplicate_distance)
        ? $self->extruder_clearance_radius
        : $self->duplicate_distance;
}

# CLASS METHODS:

sub write_ini {
    my $class = shift;
    my ($file, $ini) = @_;
    
    Slic3r::open(\my $fh, '>', $file);
    binmode $fh, ':utf8';
    my $localtime = localtime;
    printf $fh __("# generated by Slic3r")." $Slic3r::VERSION ".__("on")." %s\n", "$localtime";
    foreach my $category (sort keys %$ini) {
        printf $fh "\n[%s]\n", $category if $category ne '_';
        foreach my $key (sort keys %{$ini->{$category}}) {
            printf $fh "%s = %s\n", $key, $ini->{$category}{$key};
        }
    }
    close $fh;
}

sub read_ini {
    my $class = shift;
    my ($file) = @_;
    
    local $/ = "\n";
    Slic3r::open(\my $fh, '<', $file);
    binmode $fh, ':utf8';
    
    my $ini = { _ => {} };
    my $category = '_';
    while (my $_ = <$fh>) {
        s/\R+$//;
        next if /^\s+/;
        next if /^$/;
        next if /^\s*#/;
        if (/^\[(\w+)\]$/) {
            $category = $1;
            next;
        }
        /^(\w+) = (.*)/ or die __("Unreadable configuration file (invalid data at line ")."$.)\n";
        $ini->{$category}{$1} = $2;
    }
    close $fh;
    
    return $ini;
}

1;

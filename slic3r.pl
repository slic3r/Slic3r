#!/usr/bin/env perl

use strict;
use warnings;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/lib";
    use local::lib '--no-create', "$FindBin::Bin/local-lib";
}

use File::Basename qw(basename);
use Getopt::Long qw(:config no_auto_abbrev);
use List::Util qw(first);
use POSIX qw(setlocale LC_NUMERIC ceil);
use Slic3r;
use Slic3r::Geometry qw(epsilon X Y Z deg2rad);
use Time::HiRes qw(gettimeofday tv_interval);
$|++;
binmode STDOUT, ':utf8';
binmode STDERR, ':utf8';
$ENV{GDK_BACKEND} = 'x11';

our %opt = ();
my %cli_options = ();
{
    my %options = (
        'help'                  => sub { usage() },
        'version'               => sub { print "$Slic3r::VERSION\n"; exit 0 },
        
        'debug'                 => \$Slic3r::debug,
        'gui'                   => \$opt{gui},
        'no-gui'                => \$opt{no_gui},
        'o|output=s'            => \$opt{output},
        'j|threads=i'           => \$opt{threads},
        
        'save=s'                => \$opt{save},
        'load=s@'               => \$opt{load},
        'autosave=s'            => \$opt{autosave},
        'ignore-nonexistent-config' => \$opt{ignore_nonexistent_config},
        'datadir=s'             => \$opt{datadir},
        'export-svg'            => \$opt{export_svg},
        'merge|m'               => \$opt{merge},
        'repair'                => \$opt{repair},
        'cut=f'                 => \$opt{cut},
        'cut-grid=s'            => \$opt{cut_grid},
        'split'                 => \$opt{split},
        'info'                  => \$opt{info},
        
        'scale=f'               => \$opt{scale},
        'rotate=f'              => \$opt{rotate},
        'rotate-x=f'            => \$opt{rotate_x},
        'rotate-y=f'            => \$opt{rotate_y},
        'duplicate=i'           => \$opt{duplicate},
        'duplicate-grid=s'      => \$opt{duplicate_grid},
        'print-center=s'        => \$opt{print_center},
        'dont-arrange'          => \$opt{dont_arrange},
        
        # legacy options, ignored
        'no-plater'             => \$opt{no_plater},
        'gui-mode=s'            => \$opt{gui_mode},
    );
    foreach my $opt_key (keys %{$Slic3r::Config::Options}) {
        my $cli = $Slic3r::Config::Options->{$opt_key}->{cli} or next;
        # allow both the dash-separated option name and the full opt_key
        $options{ "$opt_key|$cli" } = \$cli_options{$opt_key};
    }
    
    @ARGV = grep !/^-psn_\d/, @ARGV if $^O eq 'darwin';
    GetOptions(%options) or usage(1);
    
    warn "--no-plater option is deprecated; ignoring\n" if $opt{no_plater};
    warn "--gui-mode option is deprecated (Slic3r now has only Expert Mode); ignoring\n"  if $opt{gui_mode};
}

# load configuration files
my @external_configs = ();
if ($opt{load}) {
    foreach my $configfile (@{$opt{load}}) {
        $configfile = Slic3r::decode_path($configfile);
        if (-e Slic3r::encode_path($configfile)) {
            push @external_configs, Slic3r::Config->load($configfile);
        } elsif (-e Slic3r::encode_path("$FindBin::Bin/$configfile")) {
            printf STDERR "Loading $FindBin::Bin/$configfile\n";
            push @external_configs, Slic3r::Config->load("$FindBin::Bin/$configfile");
        } else {
            $opt{ignore_nonexistent_config} or die "Cannot find specified configuration file ($configfile).\n";
        }
    }
    
    # expand shortcuts before applying, otherwise destination values would be already filled with defaults
    $_->normalize for @external_configs;
}

# process command line options
my $cli_config = Slic3r::Config->new_from_cli(%cli_options);
$cli_config->normalize;  # expand shortcuts

# save configuration
if ($opt{save}) {
    my $config = $cli_config->clone;
    $config->apply($_) for @external_configs;
    if (@{$config->get_keys} > 0) {
        $config->save($opt{save});
    } else {
        Slic3r::Config->new_from_defaults->save(Slic3r::decode_path($opt{save}));
    }
}

# launch GUI
my $gui;
if ((!@ARGV || $opt{gui}) && !$opt{no_gui} && !$opt{save} && eval "require Slic3r::GUI; 1") {
    {
        no warnings 'once';
        $Slic3r::GUI::datadir       = Slic3r::decode_path($opt{datadir} // '');
        $Slic3r::GUI::autosave      = Slic3r::decode_path($opt{autosave} // '');
        $Slic3r::GUI::threads       = $opt{threads};
    }
    $gui = Slic3r::GUI->new;
    setlocale(LC_NUMERIC, 'C');
    $gui->CallAfter(sub {
        $gui->{mainframe}->load_config_file($_) for @{$opt{load}};
        $gui->{mainframe}->load_config($cli_config);
        foreach my $input_file (@ARGV) {
            $input_file = Slic3r::decode_path($input_file);
            $gui->{mainframe}{plater}->load_file($input_file);
        }
    });
    $gui->MainLoop;
    exit;
}
die $@ if $@ && $opt{gui} && !$opt{no_gui};

if (@ARGV) {  # slicing from command line
    # apply command line config on top of default config
    my $config = Slic3r::Config->new_from_defaults;
    $config->apply($_) for @external_configs;
    $config->apply($cli_config);
    $config->validate;
    
    if ($opt{repair}) {
        foreach my $file (@ARGV) {
            $file = Slic3r::decode_path($file);
            die "Repair is currently supported only on STL files\n"
                if $file !~ /\.stl$/i;
            
            my $output_file = $file;
            $output_file =~ s/\.(stl)$/_fixed.obj/i;
            my $tmesh = Slic3r::TriangleMesh->new;
            $tmesh->ReadSTLFile($file);
            $tmesh->repair;
            $tmesh->WriteOBJFile($output_file);
        }
        exit;
    }
    
    if ($opt{cut}) {
        foreach my $file (@ARGV) {
            $file = Slic3r::decode_path($file);
            my $model = Slic3r::Model->read_from_file($file);
            $model->add_default_instances;
            my $mesh = $model->mesh;
            $mesh->align_to_bed();
            my $upper = Slic3r::TriangleMesh->new;
            my $lower = Slic3r::TriangleMesh->new;
            $mesh->cut(Z, $opt{cut}, $upper, $lower);
            $upper->repair;
            $lower->repair;
            $upper->write_ascii("${file}_upper.stl")
                if $upper->facets_count > 0;
            $lower->write_ascii("${file}_lower.stl")
                if $lower->facets_count > 0;
        }
        exit;
    }
    
    if ($opt{cut_grid}) {
        my ($grid_x, $grid_y) = split /[,x]/, $opt{cut_grid}, 2;
        foreach my $file (@ARGV) {
            $file = Slic3r::decode_path($file);
            my $model = Slic3r::Model->read_from_file($file);
            $model->add_default_instances;
            my $mesh = $model->mesh;
            $mesh->align_to_bed();
            my $bb = $mesh->bounding_box;
            
            my $x_parts = ceil(($bb->size->x - epsilon)/$grid_x);
            my $y_parts = ceil(($bb->size->y - epsilon)/$grid_y); #--
            
            for my $i (1..$x_parts) {
                my $this = Slic3r::TriangleMesh->new;
                if ($i == $x_parts) {
                    $this = $mesh;
                } else {
                    my $next = Slic3r::TriangleMesh->new;
                    $mesh->cut(X, $bb->x_min + ($grid_x * $i), $next, $this);
                    $this->repair;
                    $next->repair;
                    $mesh = $next;
                }
                
                for my $j (1..$y_parts) {
                    my $tile = Slic3r::TriangleMesh->new;
                    if ($j == $y_parts) {
                        $tile = $this;
                    } else {
                        my $next = Slic3r::TriangleMesh->new;
                        $this->cut(Y, $bb->y_min + ($grid_y * $j), $next, $tile);
                        $tile->repair;
                        $next->repair;
                        $this = $next;
                    }
                    $tile->write_ascii("${file}_${i}_${j}.stl");
                }
            }
        }
        exit;
    }
    
    if ($opt{split}) {
        foreach my $file (@ARGV) {
            $file = Slic3r::decode_path($file);
            my $model = Slic3r::Model->read_from_file($file);
            $model->add_default_instances;
            my $mesh = $model->mesh;
            $mesh->repair;
            
            my $part_count = 0;
            foreach my $new_mesh (@{$mesh->split}) {
                my $output_file = sprintf '%s_%02d.stl', $file, ++$part_count;
                printf "Writing to %s\n", basename($output_file);
                $new_mesh->write_binary($output_file);
            }
        }
        exit;
    }
    
    while (my $input_file = shift @ARGV) {
        $input_file = Slic3r::decode_path($input_file);
        my $model;
        if ($opt{merge}) {
            my @models = map Slic3r::Model->read_from_file($_), $input_file, (splice @ARGV, 0);
            $model = Slic3r::Model->merge(@models);
        } else {
            $model = Slic3r::Model->read_from_file($input_file);
        }
        $model->repair;
        
        if ($opt{info}) {
            $model->print_info;
            next;
        }
        
        if (defined $opt{duplicate_grid}) {
            $opt{duplicate_grid} = [ split /[,x]/, $opt{duplicate_grid}, 2 ];
        }
        if (defined $opt{print_center}) {
            $opt{print_center} = Slic3r::Pointf->new(split /[,x]/, $opt{print_center}, 2);
        }
        
        my $sprint = Slic3r::Print::Simple->new(
            scale           => $opt{scale}          // 1,
            rotate          => deg2rad($opt{rotate} // 0),
            rotate_x        => deg2rad($opt{rotate_x} // 0),
            rotate_y        => deg2rad($opt{rotate_y} // 0),
            duplicate       => $opt{duplicate}      // 1,
            duplicate_grid  => $opt{duplicate_grid} // [1,1],
            print_center    => $opt{print_center},
            dont_arrange    => $opt{dont_arrange}   // 0,
            status_cb       => sub {
                my ($percent, $message) = @_;
                printf "=> %s\n", $message;
            },
            output_file     => Slic3r::decode_path($opt{output}),
        );
        
        $sprint->apply_config($config);
        $sprint->config->set('threads', $opt{threads}) if $opt{threads};
        $sprint->set_model($model);
        
        if ($opt{export_svg}) {
            $sprint->export_svg;
        } else {
            my $t0 = [gettimeofday];
            $sprint->export_gcode;
            
            # output some statistics
            {
                my $duration = tv_interval($t0);
                printf "Done. Process took %d minutes and %.3f seconds\n", 
                    int($duration/60), ($duration - int($duration/60)*60);  # % truncates to integer
            }
            printf "Filament required: %.1fmm (%.1fcm3)\n",
                $sprint->total_used_filament, $sprint->total_extruded_volume/1000;
        }
    }
} else {
    usage(1) unless $opt{save};
}

sub usage {
    my ($exit_code) = @_;
    
    my $config = Slic3r::Config->new_from_defaults->as_hash;
    
    my $j = '';
    if ($Slic3r::have_threads) {
        $j = <<"EOF";
    -j, --threads <num> Number of threads to use
EOF
    }
    
    print <<"EOF";
Slic3r $Slic3r::VERSION is a STL-to-GCODE translator for RepRap 3D printers
written by Alessandro Ranellucci <aar\@cpan.org> - https://slic3r.org/

Usage: slic3r.pl [ OPTIONS ] [ file.stl ] [ file2.stl ] ...

    --help              Output this usage screen and exit
    --version           Output the version of Slic3r and exit
    --save <file>       Save configuration to the specified file
    --load <file>       Load configuration from the specified file. It can be used 
                        more than once to load options from multiple files.
    --datadir <path>    Load and store settings at the given directory.
                        This is useful for maintaining different profiles or including
                        configurations from a network storage.
    -o, --output <file> File to output gcode to (by default, the file will be saved
                        into the same directory as the input file using the
                        --output-filename-format to generate the filename.) If a
                        directory is specified for this option, the output will
                        be saved under that directory, and the filename will be
                        generated by --output-filename-format.
  
  Non-slicing actions (no G-code will be generated):
    --repair            Repair given STL files and save them as <name>_fixed.obj
    --cut <z>           Cut given input files at given Z (relative) and export
                        them as <name>_upper.stl and <name>_lower.stl
    --split             Split the shells contained in given STL file into several STL files
    --info              Output information about the supplied file(s) and exit
    
$j
  GUI options:
    --gui               Forces the GUI launch instead of command line slicing (if you
                        supply a model file, it will be loaded into the plater)
    --no-gui            Forces the command line slicing instead of gui. 
                        This takes precedence over --gui if both are present.
    --autosave <file>   Automatically export current configuration to the specified file

  Output options:
    --output-filename-format
                        Output file name format; all config options enclosed in brackets
                        will be replaced by their values, as well as [input_filename_base]
                        and [input_filename] (default: $config->{output_filename_format})
    --post-process      Generated G-code will be processed with the supplied script;
                        call this more than once to process through multiple scripts.
    --export-svg        Export a SVG file containing slices instead of G-code.
    -m, --merge         If multiple files are supplied, they will be composed into a single 
                        print rather than processed individually.
  
  Printer options:
    --nozzle-diameter   Diameter of nozzle in mm (default: $config->{nozzle_diameter}->[0])
    --print-center      Coordinates in mm of the point to center the print around
                        (default: 100,100)
    --z-offset          Additional height in mm to add to vertical coordinates
                        (+/-, default: $config->{z_offset})
    --z-steps-per-mm    Number of full steps per mm of the Z axis. Experimental feature for
                        preventing rounding issues.
    --gcode-flavor      The type of G-code to generate (reprap/teacup/repetier/makerware/sailfish/mach3/machinekit/smoothie/no-extrusion,
                        default: $config->{gcode_flavor})
    --use-relative-e-distances Enable this to get relative E values (default: no)
    --use-firmware-retraction  Enable firmware-controlled retraction using G10/G11 (default: no)
    --use-volumetric-e  Express E in cubic millimeters and prepend M200 (default: no)
    --gcode-arcs        Use G2/G3 commands for native arcs (experimental, not supported
                        by all firmwares)
    --gcode-comments    Make G-code verbose by adding comments (default: no)
    --vibration-limit   Limit the frequency of moves on X and Y axes (Hz, set zero to disable;
                        default: $config->{vibration_limit})
    --pressure-advance  Adjust pressure using the experimental advance algorithm (K constant,
                        set zero to disable; default: $config->{pressure_advance})
    
  Filament options:
    --filament-diameter Diameter in mm of your raw filament (default: $config->{filament_diameter}->[0])
    --extrusion-multiplier
                        Change this to alter the amount of plastic extruded. There should be
                        very little need to change this value, which is only useful to 
                        compensate for filament packing (default: $config->{extrusion_multiplier}->[0])
    --temperature       Extrusion temperature in degree Celsius, set 0 to disable (default: $config->{temperature}->[0])
    --first-layer-temperature Extrusion temperature for the first layer, in degree Celsius,
                        set 0 to disable (default: same as --temperature)
    --bed-temperature   Heated bed temperature in degree Celsius, set 0 to disable (default: $config->{bed_temperature})
    --first-layer-bed-temperature Heated bed temperature for the first layer, in degree Celsius,
                        set 0 to disable (default: same as --bed-temperature)
    
  Speed options:
    --travel-speed      Speed of non-print moves in mm/s (default: $config->{travel_speed})
    --perimeter-speed   Speed of print moves for perimeters in mm/s (default: $config->{perimeter_speed})
    --small-perimeter-speed
                        Speed of print moves for small perimeters in mm/s or % over perimeter speed
                        (default: $config->{small_perimeter_speed})
    --external-perimeter-speed
                        Speed of print moves for the external perimeter in mm/s or % over perimeter speed
                        (default: $config->{external_perimeter_speed})
    --infill-speed      Speed of print moves in mm/s (default: $config->{infill_speed})
    --solid-infill-speed Speed of print moves for solid surfaces in mm/s or % over infill speed
                        (default: $config->{solid_infill_speed})
    --top-solid-infill-speed Speed of print moves for top surfaces in mm/s or % over solid infill speed
                        (default: $config->{top_solid_infill_speed})
    --support-material-speed
                        Speed of support material print moves in mm/s (default: $config->{support_material_speed})
    --support-material-interface-speed
                        Speed of support material interface print moves in mm/s or % over support material
                        speed (default: $config->{support_material_interface_speed})
    --bridge-speed      Speed of bridge print moves in mm/s (default: $config->{bridge_speed})
    --gap-fill-speed    Speed of gap fill print moves in mm/s (default: $config->{gap_fill_speed})
    --first-layer-speed Speed of print moves for bottom layer, expressed either as an absolute
                        value or as a percentage over normal speeds (default: $config->{first_layer_speed})
    
  Acceleration options:
    --perimeter-acceleration
                        Overrides firmware's default acceleration for perimeters. (mm/s^2, set zero
                        to disable; default: $config->{perimeter_acceleration})
    --infill-acceleration
                        Overrides firmware's default acceleration for infill. (mm/s^2, set zero
                        to disable; default: $config->{infill_acceleration})
    --bridge-acceleration
                        Overrides firmware's default acceleration for bridges. (mm/s^2, set zero
                        to disable; default: $config->{bridge_acceleration})
    --first-layer-acceleration
                        Overrides firmware's default acceleration for first layer. (mm/s^2, set zero
                        to disable; default: $config->{first_layer_acceleration})
    --default-acceleration
                        Acceleration will be reset to this value after the specific settings above
                        have been applied. (mm/s^2, set zero to disable; default: $config->{default_acceleration})
    
  Accuracy options:
    --layer-height      Layer height in mm (default: $config->{layer_height})
    --first-layer-height Layer height for first layer (mm or %, default: $config->{first_layer_height})
    --infill-every-layers
                        Infill every N layers (default: $config->{infill_every_layers})
    --solid-infill-every-layers
                        Force a solid layer every N layers (default: $config->{solid_infill_every_layers})
  
  Print options:
    --perimeters        Number of perimeters/horizontal skins (range: 0+, default: $config->{perimeters})
    --top-solid-layers  Number of solid layers to do for top surfaces (range: 0+, default: $config->{top_solid_layers})
    --bottom-solid-layers  Number of solid layers to do for bottom surfaces (range: 0+, default: $config->{bottom_solid_layers})
    --min-shell-thickness  Minimum thickness of all solid shells (range: 0+, default: 0)
    --solid-layers      Shortcut for setting the two options above at once
    --fill-density      Infill density (range: 0%-100%, default: $config->{fill_density}%)
    --fill-angle        Infill angle in degrees (range: 0-90, default: $config->{fill_angle})
    --fill-pattern      Pattern to use to fill non-solid layers (default: $config->{fill_pattern})
    --fill-gaps         Fill gaps with single passes (default: yes)
    --external-infill-pattern Pattern to use to fill solid layers.
                        (Shortcut for --top-infill-pattern and --bottom-infill-pattern)
    --top-infill-pattern Pattern to use to fill top solid layers (default: $config->{top_infill_pattern})
    --bottom-infill-pattern Pattern to use to fill bottom solid layers (default: $config->{bottom_infill_pattern})
    --start-gcode       Load initial G-code from the supplied file. This will overwrite
                        the default command (home all axes [G28]).
    --end-gcode         Load final G-code from the supplied file. This will overwrite 
                        the default commands (turn off temperature [M104 S0],
                        home X axis [G28 X], disable motors [M84]).
    --before-layer-gcode  Load before-layer-change G-code from the supplied file (default: nothing).
    --layer-gcode       Load layer-change G-code from the supplied file (default: nothing).
    --toolchange-gcode  Load tool-change G-code from the supplied file (default: nothing).
    --seam-position     Position of loop starting points (random/nearest/aligned, default: $config->{seam_position}).
    --external-perimeters-first Reverse perimeter order. (default: no)
    --spiral-vase       Experimental option to raise Z gradually when printing single-walled vases
                        (default: no)
    --only-retract-when-crossing-perimeters
                        Disable retraction when travelling between infill paths inside the same island.
                        (default: no)
    --solid-infill-below-area
                        Force solid infill when a region has a smaller area than this threshold
                        (mm^2, default: $config->{solid_infill_below_area})
    --infill-only-where-needed
                        Only infill under ceilings (default: no)
    --infill-first      Make infill before perimeters (default: no)
  
   Quality options (slower slicing):
    --extra-perimeters  Add more perimeters when needed (default: yes)
    --avoid-crossing-perimeters Optimize travel moves so that no perimeters are crossed (default: no)
    --thin-walls        Detect single-width walls (default: yes)
    --detect-bridging-perimeters  Detect bridging perimeters and apply bridge flow, speed and fan
                        (default: yes)
  
   Support material options:
    --support-material  Generate support material for overhangs
    --support-material-threshold
                        Overhang threshold angle (range: 0-90, set 0 for automatic detection,
                        default: $config->{support_material_threshold})
    --support-material-pattern
                        Pattern to use for support material (default: $config->{support_material_pattern})
    --support-material-spacing
                        Spacing between pattern lines (mm, default: $config->{support_material_spacing})
    --support-material-pillar-size
                        Size of the pillars in the pillar support pattern (default: $config->{support_material_pillar_size})
    --support-material-pillar-spacing
                        Spacing between the pillars in the pillar support pattern (default: $config->{support_material_pillar_spacing})
    --support-material-angle
                        Support material angle in degrees (range: 0-90, default: $config->{support_material_angle})
    --support-material-contact-distance
                        Vertical distance between object and support material (0+, default: $config->{support_material_contact_distance})
    --support-material-interface-layers
                        Number of perpendicular layers between support material and object (0+, default: $config->{support_material_interface_layers})
    --support-material-interface-spacing
                        Spacing between interface pattern lines (mm, set 0 to get a solid layer, default: $config->{support_material_interface_spacing})
    --raft-layers       Number of layers to raise the printed objects by (range: 0+, default: $config->{raft_layers})
    --support-material-enforce-layers
                        Enforce support material on the specified number of layers from bottom,
                        regardless of --support-material and threshold (0+, default: $config->{support_material_enforce_layers})
    --support-material-buildplate-only
                        Only create support if it lies on a build plate. Don't create support on a print. (default: no)
    --dont-support-bridges
                        Experimental option for preventing support material from being generated under bridged areas (default: yes)
  
   Retraction options:
    --retract-length    Length of retraction in mm when pausing extrusion (default: $config->{retract_length}[0])
    --retract-speed     Speed for retraction in mm/s (default: $config->{retract_speed}[0])
    --retract-restart-extra
                        Additional amount of filament in mm to push after
                        compensating retraction (default: $config->{retract_restart_extra}[0])
    --retract-before-travel
                        Only retract before travel moves of this length in mm (default: $config->{retract_before_travel}[0])
    --retract-lift      Lift Z by the given distance in mm when retracting (default: $config->{retract_lift}[0])
    --retract-lift-above Only lift Z when above the specified height (default: $config->{retract_lift_above}[0])
    --retract-lift-below Only lift Z when below the specified height (default: $config->{retract_lift_below}[0])
    --retract-layer-change
                        Enforce a retraction before each Z move (default: no)
    --wipe              Wipe the nozzle while doing a retraction (default: no)
    
   Retraction options for multi-extruder setups:
    --retract-length-toolchange
                        Length of retraction in mm when disabling tool (default: $config->{retract_length_toolchange}[0])
    --retract-restart-extra-toolchange
                        Additional amount of filament in mm to push after
                        switching tool (default: $config->{retract_restart_extra_toolchange}[0])
   
   Cooling options:
    --cooling           Enable fan and cooling control
    --min-fan-speed     Minimum fan speed (default: $config->{min_fan_speed}%)
    --max-fan-speed     Maximum fan speed (default: $config->{max_fan_speed}%)
    --bridge-fan-speed  Fan speed to use when bridging (default: $config->{bridge_fan_speed}%)
    --fan-below-layer-time Enable fan if layer print time is below this approximate number 
                        of seconds (default: $config->{fan_below_layer_time})
    --slowdown-below-layer-time Slow down if layer print time is below this approximate number
                        of seconds (default: $config->{slowdown_below_layer_time})
    --min-print-speed   Minimum print speed (mm/s, default: $config->{min_print_speed})
    --disable-fan-first-layers Disable fan for the first N layers (default: $config->{disable_fan_first_layers})
    --fan-always-on     Keep fan always on at min fan speed, even for layers that don't need
                        cooling
   
   Skirt options:
    --skirts            Number of skirts to draw (0+, default: $config->{skirts})
    --skirt-distance    Distance in mm between innermost skirt and object 
                        (default: $config->{skirt_distance})
    --skirt-height      Height of skirts to draw (expressed in layers, 0+, default: $config->{skirt_height})
    --min-skirt-length  Generate no less than the number of loops required to consume this length
                        of filament on the first layer, for each extruder (mm, 0+, default: $config->{min_skirt_length})
    --brim-width        Width of the brim that will get added to each object to help adhesion
                        (mm, default: $config->{brim_width})
    --brim-overlap      Overlap between perimeter and brim to control bonding strength (%, default: $config->{brim_overlap})
    --brim-ears         Print brim only on sharp corners.
    --brim-ears-max-angle Maximum angle considered for adding brim ears. (degrees, default: $config->{brim_ears_max_angle})
    --interior-brim-width  Width of the brim that will get printed inside object holes to help adhesion
                        (mm, default: $config->{interior_brim_width})
   
   Transform options:
    --scale             Factor for scaling input object (default: 1)
    --rotate            Rotation angle in degrees around Z (default: 0)
    --rotate-x          Rotation angle in degrees around X (default: 0)
    --rotate-y          Rotation angle in degrees around Y (default: 0)
    --duplicate         Number of items with auto-arrange (1+, default: 1)
    --duplicate-grid    Number of items with grid arrangement (default: 1,1)
    --duplicate-distance Distance in mm between copies (default: $config->{duplicate_distance})
    --dont-arrange      Don't arrange the objects on the build plate. The model coordinates
                        define the absolute positions on the build plate. 
                        The option --print-center will be ignored.
    --xy-size-compensation
                        Grow/shrink objects by the configured absolute distance (mm, default: $config->{xy_size_compensation})
   
   Sequential printing options:
    --complete-objects  When printing multiple objects and/or copies, complete each one before
                        starting the next one; watch out for extruder collisions (default: no)
    --extruder-clearance-radius Radius in mm above which extruder won't collide with anything
                        (default: $config->{extruder_clearance_radius})
    --extruder-clearance-height Maximum vertical extruder depth; i.e. vertical distance from 
                        extruder tip and carriage bottom (default: $config->{extruder_clearance_height})
   
   Miscellaneous options:
    --notes             Notes to be added as comments to the output file
    --resolution        Minimum detail resolution (mm, set zero for full resolution, default: $config->{resolution})
  
   Flow options (advanced):
    --extrusion-width   Set extrusion width manually; it accepts either an absolute value in mm
                        (like 0.65) or a percentage over layer height (like 200%)
    --first-layer-extrusion-width
                        Set a different extrusion width for first layer
    --perimeter-extrusion-width
                        Set a different extrusion width for perimeters
    --external-perimeter-extrusion-width
                        Set a different extrusion width for external perimeters
    --infill-extrusion-width
                        Set a different extrusion width for infill
    --solid-infill-extrusion-width
                        Set a different extrusion width for solid infill
    --top-infill-extrusion-width
                        Set a different extrusion width for top infill
    --support-material-extrusion-width
                        Set a different extrusion width for support material
    --infill-overlap    Overlap between infill and perimeters (default: $config->{infill_overlap})
    --bridge-flow-ratio Multiplier for extrusion when bridging (> 0, default: $config->{bridge_flow_ratio})
  
   Multiple extruder options:
    --extruder-offset   Offset of each extruder, if firmware doesn't handle the displacement
                        (can be specified multiple times, default: 0x0)
    --perimeter-extruder
                        Extruder to use for perimeters and brim (1+, default: $config->{perimeter_extruder})
    --infill-extruder   Extruder to use for infill (1+, default: $config->{infill_extruder})
    --solid-infill-extruder   Extruder to use for solid infill (1+, default: $config->{solid_infill_extruder})
    --support-material-extruder
                        Extruder to use for support material, raft and skirt (1+, default: $config->{support_material_extruder})
    --support-material-interface-extruder
                        Extruder to use for support material interface (1+, default: $config->{support_material_interface_extruder})
    --ooze-prevention   Drop temperature and park extruders outside a full skirt for automatic wiping
                        (default: no)
    --standby-temperature-delta
                        Temperature difference to be applied when an extruder is not active and
                        --ooze-prevention is enabled (default: $config->{standby_temperature_delta})
    
EOF
    exit ($exit_code || 0);
}

__END__

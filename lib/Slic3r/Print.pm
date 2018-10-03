# The slicing work horse.
# Extends C++ class Slic3r::Print
package Slic3r::Print;
use strict;
use warnings;

use File::Basename qw(basename fileparse);
use File::Spec;
use List::Util qw(min max first sum);
use Slic3r::ExtrusionLoop ':roles';
use Slic3r::ExtrusionPath ':roles';
use Slic3r::Flow ':roles';
use Slic3r::Geometry qw(X Y Z X1 Y1 X2 Y2 MIN MAX PI scale unscale convex_hull);
use Slic3r::Geometry::Clipper qw(diff_ex union_ex intersection_ex intersection offset
    offset2 union union_pt_chained JT_ROUND JT_SQUARE diff_pl);
use Slic3r::Print::State ':steps';
use Slic3r::Surface qw(S_TYPE_BOTTOM);

our $status_cb;

sub set_status_cb {
    my ($class, $cb) = @_;
    $status_cb = $cb;
}

sub status_cb {
    return $status_cb // sub {};
}

# this value is not supposed to be compared with $layer->id
# since they have different semantics
sub total_layer_count {
    my $self = shift;
    return max(map $_->total_layer_count, @{$self->objects});
}

sub size {
    my $self = shift;
    return $self->bounding_box->size;
}

sub process {
    my ($self) = @_;
    
    ### No need to call this as we call it as part of prepare_infill()
    ### until we fix the idempotency issue.
    ###$self->status_cb->(20, "Generating perimeters");
    ###$_->make_perimeters for @{$self->objects};
    
    $self->status_cb->(70, "Infilling layers");
    $_->infill for @{$self->objects};
    
    $_->generate_support_material for @{$self->objects};
    $self->make_skirt;
    $self->make_brim;  # must come after make_skirt
    
    # time to make some statistics
    if (0) {
        eval "use Devel::Size";
        print  "MEMORY USAGE:\n";
        printf "  meshes        = %.1fMb\n", List::Util::sum(map Devel::Size::total_size($_->meshes), @{$self->objects})/1024/1024;
        printf "  layer slices  = %.1fMb\n", List::Util::sum(map Devel::Size::total_size($_->slices), map @{$_->layers}, @{$self->objects})/1024/1024;
        printf "  region slices = %.1fMb\n", List::Util::sum(map Devel::Size::total_size($_->slices), map @{$_->regions}, map @{$_->layers}, @{$self->objects})/1024/1024;
        printf "  perimeters    = %.1fMb\n", List::Util::sum(map Devel::Size::total_size($_->perimeters), map @{$_->regions}, map @{$_->layers}, @{$self->objects})/1024/1024;
        printf "  fills         = %.1fMb\n", List::Util::sum(map Devel::Size::total_size($_->fills), map @{$_->regions}, map @{$_->layers}, @{$self->objects})/1024/1024;
        printf "  print object  = %.1fMb\n", Devel::Size::total_size($self)/1024/1024;
    }
    if (0) {
        eval "use Slic3r::Test::SectionCut";
        Slic3r::Test::SectionCut->new(print => $self)->export_svg("section_cut.svg");
    }
}

sub escaped_split {
    my ($line) = @_;

    # Free up three characters for temporary replacement
    $line =~ s/%/%%/g;
    $line =~ s/#/##/g;
    $line =~ s/\?/\?\?/g;

    # replace escaped !'s
    $line =~ s/\!\!/%#\?/g;
    
    # split on non-escaped whitespace
    my @split = split /(?<=[^\!])\s+/, $line, -1;

    for my $part (@split) {
      # replace escaped whitespace with the whitespace
      $part =~ s/\!(\s+)/$1/g;

      # resub temp symbols
      $part =~ s/%#\?/\!/g;
      $part =~ s/%%/%/g;
      $part =~ s/##/#/g;
      $part =~ s/\?\?/\?/g;
    }

    return @split;
}

sub export_gcode {
    my $self = shift;
    my %params = @_;
    
    # prerequisites
    $self->process;
    
    # output everything to a G-code file
    my $output_file = $self->output_filepath($params{output_file} // '');
    $self->status_cb->(90, "Exporting G-code" . ($output_file ? " to $output_file" : ""));
    
    {
        # open output gcode file if we weren't supplied a file-handle
        my ($fh, $tempfile);
        if ($params{output_fh}) {
            $fh = $params{output_fh};
        } else {
            $tempfile = "$output_file.tmp";
            Slic3r::open(\$fh, ">", $tempfile)
                or die "Failed to open $tempfile for writing\n";
    
            # enable UTF-8 output since user might have entered Unicode characters in fields like notes
            binmode $fh, ':utf8';
        }

        Slic3r::Print::GCode->new(
            print   => $self,
            fh      => $fh,
        )->export;

        # close our gcode file
        close $fh;
        if ($tempfile) {
            my $renamed = 0;
            for my $i (1..5) {
                last if $renamed = rename Slic3r::encode_path($tempfile), Slic3r::encode_path($output_file);
                # Wait for 1/4 seconds and try to rename once again.
                select(undef, undef, undef, 0.25);
            }
            Slic3r::debugf "Failed to remove the output G-code file from $tempfile to $output_file. Is $tempfile locked?\n"
                if !$renamed;
        }
    }
    
    # run post-processing scripts
    if (@{$self->config->post_process}) {
        $self->status_cb->(95, "Running post-processing scripts");
        $self->config->setenv;
        for my $script (@{$self->config->post_process}) {
            Slic3r::debugf "  '%s' '%s'\n", $script, $output_file;
            my @parsed_script = escaped_split $script;
            my $executable = shift @parsed_script ;
            push @parsed_script, $output_file;
            # -x doesn't return true on Windows except for .exe files
            if (($^O eq 'MSWin32') ? !(-e $executable) : !(-x $executable)) {
                die "The configured post-processing script is not executable: check permissions or escape whitespace/exclamation points. ($executable) \n";
            }
            system($executable, @parsed_script);
        }
    }
}

# Export SVG slices for the offline SLA printing.
sub export_svg {
    my $self = shift;
    my %params = @_;
    
    $_->slice for @{$self->objects};
    
    my $fh = $params{output_fh};
    if (!$fh) {
        my $output_file = $self->output_filepath($params{output_file});
        $output_file =~ s/\.gcode$/.svg/i;
        Slic3r::open(\$fh, ">", $output_file) or die "Failed to open $output_file for writing\n";
        print "Exporting to $output_file..." unless $params{quiet};
    }
    
    my $print_bb = $self->bounding_box;
    my $print_size = $print_bb->size;
    print $fh sprintf <<"EOF", unscale($print_size->[X]), unscale($print_size->[Y]);
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.0//EN" "http://www.w3.org/TR/2001/REC-SVG-20010904/DTD/svg10.dtd">
<svg width="%s" height="%s" xmlns="http://www.w3.org/2000/svg" xmlns:svg="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" xmlns:slic3r="http://slic3r.org/namespaces/slic3r">
  <!-- 
  Generated using Slic3r $Slic3r::VERSION
  http://slic3r.org/
   -->
EOF
    
    my $print_polygon = sub {
        my ($polygon, $type) = @_;
        printf $fh qq{    <polygon slic3r:type="%s" points="%s" style="fill: %s" />\n},
            $type, (join ' ', map { join ',', map unscale $_, @$_ } @$polygon),
            ($type eq 'contour' ? 'white' : 'black');
    };
    
    my @layers = sort { $a->print_z <=> $b->print_z }
        map { @{$_->layers}, @{$_->support_layers} }
        @{$self->objects};
    
    my $layer_id = -1;
    my @previous_layer_slices = ();
    for my $layer (@layers) {
        $layer_id++;
        if ($layer->slice_z == -1) {
            printf $fh qq{  <g id="layer%d">\n}, $layer_id;
        } else {
            printf $fh qq{  <g id="layer%d" slic3r:z="%s">\n}, $layer_id, unscale($layer->slice_z);
        }
        
        my @current_layer_slices = ();
        # sort slices so that the outermost ones come first
        my @slices = sort { $a->contour->contains_point($b->contour->first_point) ? 0 : 1 } @{$layer->slices};
        foreach my $copy (@{$layer->object->_shifted_copies}) {
            foreach my $slice (@slices) {
                my $expolygon = $slice->clone;
                $expolygon->translate(@$copy);
                $expolygon->translate(-$print_bb->x_min, -$print_bb->y_min);
                $print_polygon->($expolygon->contour, 'contour');
                $print_polygon->($_, 'hole') for @{$expolygon->holes};
                push @current_layer_slices, $expolygon;
            }
        }
        # generate support material
        if ($self->has_support_material && $layer->id > 0) {
            my (@supported_slices, @unsupported_slices) = ();
            foreach my $expolygon (@current_layer_slices) {
                my $intersection = intersection_ex(
                    [ map @$_, @previous_layer_slices ],
                    [ @$expolygon ],
                );
                @$intersection
                    ? push @supported_slices, $expolygon
                    : push @unsupported_slices, $expolygon;
            }
            my @supported_points = map @$_, @$_, @supported_slices;
            foreach my $expolygon (@unsupported_slices) {
                # look for the nearest point to this island among all
                # supported points
                my $contour = $expolygon->contour;
                my $support_point = $contour->first_point->nearest_point(\@supported_points)
                    or next;
                my $anchor_point = $support_point->nearest_point([ @$contour ]);
                printf $fh qq{    <line x1="%s" y1="%s" x2="%s" y2="%s" style="stroke-width: 2; stroke: white" />\n},
                    map @$_, $support_point, $anchor_point;
            }
        }
        print $fh qq{  </g>\n};
        @previous_layer_slices = @current_layer_slices;
    }
    
    print $fh "</svg>\n";
    close $fh;
    print "Done.\n" unless $params{quiet};
}

sub make_skirt {
    my $self = shift;
    
    # prerequisites
    $_->make_perimeters for @{$self->objects};
    $_->infill for @{$self->objects};
    $_->generate_support_material for @{$self->objects};
    
    return if $self->step_done(STEP_SKIRT);
    $self->set_step_started(STEP_SKIRT);
    
    # since this method must be idempotent, we clear skirt paths *before*
    # checking whether we need to generate them
    $self->skirt->clear;
    
    if (!$self->has_skirt) {
        $self->set_step_done(STEP_SKIRT);
        return;
    }
    $self->status_cb->(88, "Generating skirt");
    
    # First off we need to decide how tall the skirt must be.
    # The skirt_height option from config is expressed in layers, but our
    # object might have different layer heights, so we need to find the print_z
    # of the highest layer involved.
    # Note that unless has_infinite_skirt() == true
    # the actual skirt might not reach this $skirt_height_z value since the print
    # order of objects on each layer is not guaranteed and will not generally
    # include the thickest object first. It is just guaranteed that a skirt is
    # prepended to the first 'n' layers (with 'n' = skirt_height).
    # $skirt_height_z in this case is the highest possible skirt height for safety.
    my $skirt_height_z = -1;
    foreach my $object (@{$self->objects}) {
        my $skirt_height = $self->has_infinite_skirt
            ? $object->layer_count
            : min($self->config->skirt_height, $object->layer_count);
        my $highest_layer = $object->get_layer($skirt_height - 1);
        $skirt_height_z = max($skirt_height_z, $highest_layer->print_z);
    }
    
    # collect points from all layers contained in skirt height
    my @points = ();
    foreach my $object (@{$self->objects}) {
        my @object_points = ();
        
        # get object layers up to $skirt_height_z
        foreach my $layer (@{$object->layers}) {
            last if $layer->print_z > $skirt_height_z;
            push @object_points, map @$_, map @$_, @{$layer->slices};
        }
        
        # get support layers up to $skirt_height_z
        foreach my $layer (@{$object->support_layers}) {
            last if $layer->print_z > $skirt_height_z;
            push @object_points, map @{$_->polyline}, @{$layer->support_fills} if $layer->support_fills;
            push @object_points, map @{$_->polyline}, @{$layer->support_interface_fills} if $layer->support_interface_fills;
        }
        
        # repeat points for each object copy
        foreach my $copy (@{$object->_shifted_copies}) {
            my @copy_points = map $_->clone, @object_points;
            $_->translate(@$copy) for @copy_points;
            push @points, @copy_points;
        }
    }
    return if @points < 3;  # at least three points required for a convex hull
    
    # find out convex hull
    my $convex_hull = convex_hull(\@points);
    
    my @extruded_length = ();  # for each extruder
    
    # skirt may be printed on several layers, having distinct layer heights,
    # but loops must be aligned so can't vary width/spacing
    # TODO: use each extruder's own flow
    my $first_layer_height = $self->skirt_first_layer_height;
    my $flow = $self->skirt_flow;
    my $spacing = $flow->spacing;
    my $mm3_per_mm = $flow->mm3_per_mm;
    
    my @extruders_e_per_mm = ();
    my $extruder_idx = 0;
    
    my $skirts = $self->config->skirts;
    $skirts ||= 1 if $self->has_infinite_skirt;
    
    # draw outlines from outside to inside
    # loop while we have less skirts than required or any extruder hasn't reached the min length if any
    my $distance = scale max($self->config->skirt_distance, $self->config->brim_width);
    for (my $i = $skirts; $i > 0; $i--) {
        $distance += scale $spacing;
        my $loop = offset([$convex_hull], $distance, 1, JT_ROUND, scale(0.1))->[0];
        my $eloop = Slic3r::ExtrusionLoop->new_from_paths(
            Slic3r::ExtrusionPath->new(
                polyline        => Slic3r::Polygon->new(@$loop)->split_at_first_point,
                role            => EXTR_ROLE_SKIRT,
                mm3_per_mm      => $mm3_per_mm,         # this will be overridden at G-code export time
                width           => $flow->width,
                height          => $first_layer_height, # this will be overridden at G-code export time
            ),
        );
        $eloop->role(EXTRL_ROLE_SKIRT);
        $self->skirt->append($eloop);
        
        if ($self->config->min_skirt_length > 0) {
            $extruded_length[$extruder_idx] ||= 0;
            if (!$extruders_e_per_mm[$extruder_idx]) {
                my $config = Slic3r::Config::GCode->new;
                $config->apply_static($self->config);
                my $extruder = Slic3r::Extruder->new($extruder_idx, $config);
                $extruders_e_per_mm[$extruder_idx] = $extruder->e_per_mm($mm3_per_mm);
            }
            $extruded_length[$extruder_idx] += unscale $loop->length * $extruders_e_per_mm[$extruder_idx];
            $i++ if defined first { ($extruded_length[$_] // 0) < $self->config->min_skirt_length } 0 .. $#{$self->extruders};
            if ($extruded_length[$extruder_idx] >= $self->config->min_skirt_length) {
                if ($extruder_idx < $#{$self->extruders}) {
                    $extruder_idx++;
                    next;
                }
            }
        }
    }
    
    $self->skirt->reverse;
    
    $self->set_step_done(STEP_SKIRT);
}

sub make_brim {
    my $self = shift;
    
    # prerequisites
    $_->make_perimeters for @{$self->objects};
    $_->infill for @{$self->objects};
    $_->generate_support_material for @{$self->objects};
    $self->make_skirt;
    
    $self->status_cb->(88, "Generating brim");
    $self->_make_brim;
}

# Wrapper around the C++ Slic3r::Print::validate()
# to produce a Perl exception without a hang-up on some Strawberry perls.
sub validate
{
    my $self = shift;
    my $err = $self->_validate;
    die $err . "\n" if (defined($err) && $err ne '');
}

1;

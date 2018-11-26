package Slic3r::Print::Object;
# extends c++ class Slic3r::PrintObject (Print.xsp)
use strict;
use warnings;

use POSIX;
use List::Util qw(min max sum first any);
use Slic3r::Flow ':roles';
use Slic3r::Geometry qw(X Y Z PI scale unscale chained_path epsilon);
use Slic3r::Geometry::Clipper qw(diff diff_ex intersection intersection_ex union union_ex 
    offset offset_ex offset2 offset2_ex intersection_ppl CLIPPER_OFFSET_SCALE JT_MITER);
use Slic3r::Print::State ':steps';
use Slic3r::Surface ':types';


# TODO: lazy
sub fill_maker {
    my $self = shift;
    return Slic3r::Fill->new(bounding_box => $self->bounding_box);
}

sub region_volumes {
    my $self = shift;
    return [ map $self->get_region_volumes($_), 0..($self->region_count - 1) ];
}

sub layers {
    my $self = shift;
    return [ map $self->get_layer($_), 0..($self->layer_count - 1) ];
}

sub support_layers {
    my $self = shift;
    return [ map $self->get_support_layer($_), 0..($self->support_layer_count - 1) ];
}

# 1) Decides Z positions of the layers,
# 2) Initializes layers and their regions
# 3) Slices the object meshes
# 4) Slices the modifier meshes and reclassifies the slices of the object meshes by the slices of the modifier meshes
# 5) Applies size compensation (offsets the slices in XY plane)
# 6) Replaces bad slices by the slices reconstructed from the upper/lower layer
# Resulting expolygons of layer regions are marked as Internal.
#
# this should be idempotent
sub slice {
    my $self = shift;
    
    return if $self->step_done(STEP_SLICE);
    $self->set_step_started(STEP_SLICE);
    $self->print->status_cb->(10, "Processing triangulated mesh");

    $self->_slice;

    # detect slicing errors
    my $warning_thrown = 0;
    for my $i (0 .. ($self->layer_count - 1)) {
        my $layer = $self->get_layer($i);
        next unless $layer->slicing_errors;
        if (!$warning_thrown) {
            warn "The model has overlapping or self-intersecting facets. I tried to repair it, "
                . "however you might want to check the results or repair the input file and retry.\n";
            $warning_thrown = 1;
        }
        
        # try to repair the layer surfaces by merging all contours and all holes from
        # neighbor layers
        Slic3r::debugf "Attempting to repair layer %d\n", $i;
        
        foreach my $region_id (0 .. ($layer->region_count - 1)) {
            my $layerm = $layer->region($region_id);
            
            my (@upper_surfaces, @lower_surfaces);
            for (my $j = $i+1; $j < $self->layer_count; $j++) {
                if (!$self->get_layer($j)->slicing_errors) {
                    @upper_surfaces = @{$self->get_layer($j)->region($region_id)->slices};
                    last;
                }
            }
            for (my $j = $i-1; $j >= 0; $j--) {
                if (!$self->get_layer($j)->slicing_errors) {
                    @lower_surfaces = @{$self->get_layer($j)->region($region_id)->slices};
                    last;
                }
            }
            
            my $union = union_ex([
                map $_->expolygon->contour, @upper_surfaces, @lower_surfaces,
            ]);
            my $diff = diff_ex(
                [ map @$_, @$union ],
                [ map @{$_->expolygon->holes}, @upper_surfaces, @lower_surfaces, ],
            );
            
            $layerm->slices->clear;
            $layerm->slices->append($_)
                for map Slic3r::Surface->new
                    (expolygon => $_, surface_type => S_TYPE_INTERNAL),
                    @$diff;
        }
            
        # update layer slices after repairing the single regions
        $layer->make_slices;
    }
    
    # remove empty layers from bottom
    while (@{$self->layers} && !@{$self->get_layer(0)->slices}) {
        $self->delete_layer(0);
        for (my $i = 0; $i <= $#{$self->layers}; $i++) {
            $self->get_layer($i)->set_id( $self->get_layer($i)->id-1 );
        }
    }
    
    # simplify slices if required
    if ($self->print->config->resolution) {
        $self->_simplify_slices(scale($self->print->config->resolution));
    }
    
    die "No layers were detected. You might want to repair your STL file(s) or check their size or thickness and retry.\n"
        if !@{$self->layers};
    
    $self->set_typed_slices(0);
    $self->set_step_done(STEP_SLICE);
}

sub make_perimeters {
    my ($self) = @_;
    
    return if $self->step_done(STEP_PERIMETERS);
    
    # Temporary workaround for detect_surfaces_type() not being idempotent (see #3764).
    # We can remove this when idempotence is restored. This make_perimeters() method
    # will just call merge_slices() to undo the typed slices and invalidate posDetectSurfaces.
    if ($self->typed_slices) {
        $self->invalidate_step(STEP_SLICE);
    }
    
    # prerequisites
    $self->slice;
    
    $self->_make_perimeters;
}

# This will assign a type (top/bottom/internal) to $layerm->slices
# and transform $layerm->fill_surfaces from expolygon 
# to typed top/bottom/internal surfaces;
sub detect_surfaces_type {
    my ($self) = @_;
    
    # prerequisites
    $self->slice;
    
    $self->_detect_surfaces_type;
}

sub prepare_infill {
    my ($self) = @_;
    
    return if $self->step_done(STEP_PREPARE_INFILL);
    
    # This prepare_infill() is not really idempotent.
    # TODO: It should clear and regenerate fill_surfaces at every run 
    # instead of modifying it in place.
    $self->invalidate_step(STEP_PERIMETERS);
    $self->make_perimeters;
    
    # Do this after invalidating STEP_PERIMETERS because that would re-invalidate STEP_PREPARE_INFILL
    $self->set_step_started(STEP_PREPARE_INFILL);
    
    # prerequisites
    $self->detect_surfaces_type;
    
    $self->print->status_cb->(30, "Preparing infill");
    
    # decide what surfaces are to be filled
    $_->prepare_fill_surfaces for map @{$_->regions}, @{$self->layers};

    # this will detect bridges and reverse bridges
    # and rearrange top/bottom/internal surfaces
    $self->process_external_surfaces;

    # detect which fill surfaces are near external layers
    # they will be split in internal and internal-solid surfaces
    $self->discover_horizontal_shells;
    $self->clip_fill_surfaces;
    
    # the following step needs to be done before combination because it may need
    # to remove only half of the combined infill
    $self->bridge_over_infill;

    # combine fill surfaces to honor the "infill every N layers" option
    $self->combine_infill;
    
    $self->set_step_done(STEP_PREPARE_INFILL);
}

sub infill {
    my ($self) = @_;
    
    # prerequisites
    $self->prepare_infill;
    
    $self->_infill;
}

sub generate_support_material {
    my $self = shift;
    
    # prerequisites
    $self->slice;
    
    return if $self->step_done(STEP_SUPPORTMATERIAL);
    $self->set_step_started(STEP_SUPPORTMATERIAL);
    
    $self->clear_support_layers;
    
    if ((!$self->config->support_material
		    && $self->config->raft_layers == 0
		    && $self->config->support_material_enforce_layers == 0)
		  || scalar(@{$self->layers}) < 2
		  ) {
        $self->set_step_done(STEP_SUPPORTMATERIAL);
        return;
    }
    $self->print->status_cb->(85, "Generating support material");
    
    $self->_support_material->generate($self);
    
    $self->set_step_done(STEP_SUPPORTMATERIAL);
    my $stats = sprintf "Weight: %.1fg, Cost: %.1f" , $self->print->total_weight, $self->print->total_cost;
    $self->print->status_cb->(85, $stats);
}

sub _support_material {
    my ($self) = @_;
    
    my $first_layer_flow = Slic3r::Flow->new_from_width(
        width               => ($self->print->config->first_layer_extrusion_width || $self->config->support_material_extrusion_width),
        role                => FLOW_ROLE_SUPPORT_MATERIAL,
        nozzle_diameter     => $self->print->config->nozzle_diameter->[ $self->config->support_material_extruder-1 ]
                                // $self->print->config->nozzle_diameter->[0],
        layer_height        => $self->config->get_abs_value('first_layer_height'),
        bridge_flow_ratio   => 0,
    );
    
    return Slic3r::Print::SupportMaterial->new(
        print_config        => $self->print->config,
        object_config       => $self->config,
        first_layer_flow    => $first_layer_flow,
        flow                => $self->support_material_flow(FLOW_ROLE_SUPPORT_MATERIAL),
        interface_flow      => $self->support_material_flow(FLOW_ROLE_SUPPORT_MATERIAL_INTERFACE),
    );
}

# combine fill surfaces across layers
# Idempotence of this method is guaranteed by the fact that we don't remove things from
# fill_surfaces but we only turn them into VOID surfaces, thus preserving the boundaries.
sub combine_infill {
    my $self = shift;
    
    # define the type used for voids
    my %voidtype = (
        &S_TYPE_INTERNAL() => S_TYPE_INTERNALVOID,
    );
    
    # work on each region separately
    for my $region_id (0 .. ($self->print->region_count-1)) {
        my $region = $self->print->get_region($region_id);
        my $every = $region->config->infill_every_layers;
        next unless $every > 1 && $region->config->fill_density > 0;
        
        # limit the number of combined layers to the maximum height allowed by this regions' nozzle
        my $nozzle_diameter = min(
            $self->print->config->get_at('nozzle_diameter', $region->config->infill_extruder-1),
            $self->print->config->get_at('nozzle_diameter', $region->config->solid_infill_extruder-1),
        );
        
        # define the combinations
        my %combine = ();   # layer_idx => number of additional combined lower layers
        {
            my $current_height = my $layers = 0;
            for my $layer_idx (0 .. ($self->layer_count-1)) {
                my $layer = $self->get_layer($layer_idx);
                next if $layer->id == 0;  # skip first print layer (which may not be first layer in array because of raft)
                my $height = $layer->height;
                
                # check whether the combination of this layer with the lower layers' buffer
                # would exceed max layer height or max combined layer count
                if ($current_height + $height >= $nozzle_diameter + epsilon || $layers >= $every) {
                    # append combination to lower layer
                    $combine{$layer_idx-1} = $layers;
                    $current_height = $layers = 0;
                }
                
                $current_height += $height;
                $layers++;
            }
            
            # append lower layers (if any) to uppermost layer
            $combine{$self->layer_count-1} = $layers;
        }
        
        # loop through layers to which we have assigned layers to combine
        for my $layer_idx (sort keys %combine) {
            next unless $combine{$layer_idx} > 1;
            
            # get all the LayerRegion objects to be combined
            my @layerms = map $self->get_layer($_)->get_region($region_id),
                ($layer_idx - ($combine{$layer_idx}-1) .. $layer_idx);
            
            # only combine internal infill
            for my $type (S_TYPE_INTERNAL) {
                # we need to perform a multi-layer intersection, so let's split it in pairs
                
                # initialize the intersection with the candidates of the lowest layer
                my $intersection = [ map $_->expolygon, @{$layerms[0]->fill_surfaces->filter_by_type($type)} ];
                
                # start looping from the second layer and intersect the current intersection with it
                for my $layerm (@layerms[1 .. $#layerms]) {
                    $intersection = intersection_ex(
                        [ map @$_, @$intersection ],
                        [ map @{$_->expolygon}, @{$layerm->fill_surfaces->filter_by_type($type)} ],
                    );
                }
                
                my $area_threshold = $layerms[0]->infill_area_threshold;
                @$intersection = grep $_->area > $area_threshold, @$intersection;
                next if !@$intersection;
                Slic3r::debugf "  combining %d %s regions from layers %d-%d\n",
                    scalar(@$intersection),
                    ($type == S_TYPE_INTERNAL ? 'internal' : 'internal-solid'),
                    $layer_idx-($every-1), $layer_idx;
                
                # $intersection now contains the regions that can be combined across the full amount of layers
                # so let's remove those areas from all layers
                
                 my @intersection_with_clearance = map @{$_->offset(
                       $layerms[-1]->flow(FLOW_ROLE_SOLID_INFILL)->scaled_width    / 2
                     + $layerms[-1]->flow(FLOW_ROLE_PERIMETER)->scaled_width / 2
                     # Because fill areas for rectilinear and honeycomb are grown 
                     # later to overlap perimeters, we need to counteract that too.
                     + (($type == S_TYPE_INTERNALSOLID || $region->config->fill_pattern =~ /(rectilinear|grid|line|honeycomb)/)
                       ? $layerms[-1]->flow(FLOW_ROLE_SOLID_INFILL)->scaled_width
                       : 0)
                     )}, @$intersection;

                
                foreach my $layerm (@layerms) {
                    my @this_type   = @{$layerm->fill_surfaces->filter_by_type($type)};
                    my @other_types = map $_->clone, grep $_->surface_type != $type, @{$layerm->fill_surfaces};
                    
                    my @new_this_type = map Slic3r::Surface->new(expolygon => $_, surface_type => $type),
                        @{diff_ex(
                            [ map $_->p, @this_type ],
                            [ @intersection_with_clearance ],
                        )};
                    
                    # apply surfaces back with adjusted depth to the uppermost layer
                    if ($layerm->layer->id == $self->get_layer($layer_idx)->id) {
                        push @new_this_type,
                            map Slic3r::Surface->new(
                                expolygon        => $_,
                                surface_type     => $type,
                                thickness        => sum(map $_->layer->height, @layerms),
                                thickness_layers => scalar(@layerms),
                            ),
                            @$intersection;
                    } else {
                        # save void surfaces
                        push @new_this_type,
                            map Slic3r::Surface->new(expolygon => $_, surface_type => $voidtype{$type}),
                            @{intersection_ex(
                                [ map @{$_->expolygon}, @this_type ],
                                [ @intersection_with_clearance ],
                            )};
                    }
                    
                    $layerm->fill_surfaces->clear;
                    $layerm->fill_surfaces->append($_) for (@new_this_type, @other_types);
                }
            }
        }
    }
}

1;

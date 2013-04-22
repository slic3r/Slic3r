package Slic3r::Print::Object;
use Moo;

use List::Util qw(min sum first);
use Slic3r::ExtrusionPath ':roles';
use Slic3r::Geometry qw(Z PI scale unscale deg2rad rad2deg scaled_epsilon chained_path_points);
use Slic3r::Geometry::Clipper qw(diff diff_ex intersection_ex union_ex offset offset_ex collapse_ex);
use Slic3r::Surface ':types';

has 'print'             => (is => 'ro', weak_ref => 1, required => 1);
has 'input_file'        => (is => 'rw', required => 0);
has 'meshes'            => (is => 'rw', default => sub { [] });  # by region_id
has 'size'              => (is => 'rw', required => 1);
has 'copies'            => (is => 'rw', default => sub {[ [0,0] ]}, trigger => 1);
has 'layers'            => (is => 'rw', default => sub { [] });
has 'layer_height_ranges' => (is => 'rw', default => sub { [] }); # [ z_min, z_max, layer_height ]

sub BUILD {
    my $self = shift;
 	 
    # make layers taking custom heights into account
    my $print_z = my $slice_z = my $height = 0;
    
    # add raft layers
    for my $id (0 .. $Slic3r::Config->raft_layers-1) {
        $height = ($id == 0)
            ? $Slic3r::Config->get_value('first_layer_height')
            : $Slic3r::Config->layer_height;
        
        $print_z += $height;
        
        push @{$self->layers}, Slic3r::Layer->new(
            object  => $self,
            id      => $id,
            height  => $height,
            print_z => scale $print_z,
            slice_z => -1,
        );
    }
    
    # loop until we have at least one layer and the max slice_z reaches the object height
    my $max_z = unscale $self->size->[Z];
    while (!@{$self->layers} || ($slice_z - $height) <= $max_z) {
        my $id = $#{$self->layers} + 1;
        
        # assign the default height to the layer according to the general settings
        $height = ($id == 0)
            ? $Slic3r::Config->get_value('first_layer_height')
            : $Slic3r::Config->layer_height;
        
        # look for an applicable custom range
        if (my $range = first { $_->[0] <= $slice_z && $_->[1] > $slice_z } @{$self->layer_height_ranges}) {
            $height = $range->[2];
        
            # if user set custom height to zero we should just skip the range and resume slicing over it
            if ($height == 0) {
                $slice_z += $range->[1] - $range->[0];
                next;
            }
        }
        
        $print_z += $height;
        $slice_z += $height/2;
        
        ### Slic3r::debugf "Layer %d: height = %s; slice_z = %s; print_z = %s\n", $id, $height, $slice_z, $print_z;
        
        push @{$self->layers}, Slic3r::Layer->new(
            object  => $self,
            id      => $id,
            height  => $height,
            print_z => scale $print_z,
            slice_z => scale $slice_z,
        );
        
        $slice_z += $height/2;   # add the other half layer
    }
}

sub _trigger_copies {
    my $self = shift;
    return unless @{$self->copies} > 1;
    
    # order copies with a nearest neighbor search
    @{$self->copies} = @{chained_path_points($self->copies)}
}

sub layer_count {
    my $self = shift;
    return scalar @{ $self->layers };
}

sub get_layer_range {
    my $self = shift;
    my ($min_z, $max_z) = @_;
    
    # $min_layer is the uppermost layer having slice_z <= $min_z
    # $max_layer is the lowermost layer having slice_z >= $max_z
    my ($min_layer, $max_layer);

    my ($bottom, $top) = (0, $#{$self->layers});
    while (1) {
        my $mid = $bottom+int(($top - $bottom)/2);
        if ($mid == $top || $mid == $bottom) {
            $min_layer = $mid;
            last;
        }
        if ($self->layers->[$mid]->slice_z >= $min_z) {
            $top = $mid;
        } else {
            $bottom = $mid;
        }
    }
    $top = $#{$self->layers};
    while (1) {
        my $mid = $bottom+int(($top - $bottom)/2);
        if ($mid == $top || $mid == $bottom) {
            $max_layer = $mid;
            last;
        }
        if ($self->layers->[$mid]->slice_z < $max_z) {
            $bottom = $mid;
        } else {
            $top = $mid;
        }
    }
    return ($min_layer, $max_layer);
}

sub slice {
    my $self = shift;
    my %params = @_;
    
    # process facets
    for my $region_id (0 .. $#{$self->meshes}) {
        my $mesh = $self->meshes->[$region_id];  # ignore undef meshes
        
        my $apply_lines = sub {
            my $lines = shift;
            foreach my $layer_id (keys %$lines) {
                my $layerm = $self->layers->[$layer_id]->region($region_id);
                push @{$layerm->lines}, @{$lines->{$layer_id}};
            }
        };
        Slic3r::parallelize(
            disable => ($#{$mesh->facets} < 500),  # don't parallelize when too few facets
            items => [ 0..$#{$mesh->facets} ],
            thread_cb => sub {
                my $q = shift;
                my $result_lines = {};
                while (defined (my $facet_id = $q->dequeue)) {
                    my $lines = $mesh->slice_facet($self, $facet_id);
                    foreach my $layer_id (keys %$lines) {
                        $result_lines->{$layer_id} ||= [];
                        push @{ $result_lines->{$layer_id} }, @{ $lines->{$layer_id} };
                    }
                }
                return $result_lines;
            },
            collect_cb => sub {
                $apply_lines->($_[0]);
            },
            no_threads_cb => sub {
                for (0..$#{$mesh->facets}) {
                    my $lines = $mesh->slice_facet($self, $_);
                    $apply_lines->($lines);
                }
            },
        );
    }
    die "Invalid input file\n" if !@{$self->layers};
    
    # free memory
    $self->meshes(undef);
    
    # remove last layer(s) if empty
    pop @{$self->layers} while !map @{$_->lines}, @{$self->layers->[-1]->regions};
    
    foreach my $layer (@{ $self->layers }) {
        # make sure all layers contain layer region objects for all regions
        $layer->region($_) for 0 .. ($self->print->regions_count-1);
        
        Slic3r::debugf "Making surfaces for layer %d (slice z = %f):\n",
            $layer->id, unscale $layer->slice_z if $Slic3r::debug;
        
        # layer currently has many lines representing intersections of
        # model facets with the layer plane. there may also be lines
        # that we need to ignore (for example, when two non-horizontal
        # facets share a common edge on our plane, we get a single line;
        # however that line has no meaning for our layer as it's enclosed
        # inside a closed polyline)
        
        # build surfaces from sparse lines
        foreach my $layerm (@{$layer->regions}) {
            my ($slicing_errors, $loops) = Slic3r::TriangleMesh::make_loops($layerm->lines);
            $layer->slicing_errors(1) if $slicing_errors;
            $layerm->make_surfaces($loops);
            
            # free memory
            $layerm->lines(undef);
        }
        
        # merge all regions' slices to get islands
        $layer->make_slices;
    }
    
    # detect slicing errors
    my $warning_thrown = 0;
    for my $i (0 .. $#{$self->layers}) {
        my $layer = $self->layers->[$i];
        next unless $layer->slicing_errors;
        if (!$warning_thrown) {
            warn "The model has overlapping or self-intersecting facets. I tried to repair it, "
                . "however you might want to check the results or repair the input file and retry.\n";
            $warning_thrown = 1;
        }
        
        # try to repair the layer surfaces by merging all contours and all holes from
        # neighbor layers
        Slic3r::debugf "Attempting to repair layer %d\n", $i;
        
        foreach my $region_id (0 .. $#{$layer->regions}) {
            my $layerm = $layer->region($region_id);
            
            my (@upper_surfaces, @lower_surfaces);
            for (my $j = $i+1; $j <= $#{$self->layers}; $j++) {
                if (!$self->layers->[$j]->slicing_errors) {
                    @upper_surfaces = @{$self->layers->[$j]->region($region_id)->slices};
                    last;
                }
            }
            for (my $j = $i-1; $j >= 0; $j--) {
                if (!$self->layers->[$j]->slicing_errors) {
                    @lower_surfaces = @{$self->layers->[$j]->region($region_id)->slices};
                    last;
                }
            }
            
            my $union = union_ex([
                map $_->expolygon->contour, @upper_surfaces, @lower_surfaces,
            ]);
            my $diff = diff_ex(
                [ map @$_, @$union ],
                [ map $_->expolygon->holes, @upper_surfaces, @lower_surfaces, ],
            );
            
            @{$layerm->slices} = map Slic3r::Surface->new
                (expolygon => $_, surface_type => S_TYPE_INTERNAL),
                @$diff;
        }
            
        # update layer slices after repairing the single regions
        $layer->make_slices;
    }
    
    # remove empty layers from bottom
    my $first_object_layer_id = $Slic3r::Config->raft_layers;
    while (@{$self->layers} && !@{$self->layers->[$first_object_layer_id]->slices} && !map @{$_->thin_walls}, @{$self->layers->[$first_object_layer_id]->regions}) {
        splice @{$self->layers}, $first_object_layer_id, 1;
        for (my $i = $first_object_layer_id; $i <= $#{$self->layers}; $i++) {
            $self->layers->[$i]->id($i);
        }
    }
    
    warn "No layers were detected. You might want to repair your STL file and retry.\n"
        if !@{$self->layers};
}

sub make_perimeters {
    my $self = shift;
    
    # compare each layer to the one below, and mark those slices needing
    # one additional inner perimeter, like the top of domed objects-
    
    # this algorithm makes sure that at least one perimeter is overlapping
    # but we don't generate any extra perimeter if fill density is zero, as they would be floating
    # inside the object - infill_only_where_needed should be the method of choice for printing
    # hollow objects
    if ($Slic3r::Config->extra_perimeters && $Slic3r::Config->perimeters > 0 && $Slic3r::Config->fill_density > 0) {
        for my $region_id (0 .. ($self->print->regions_count-1)) {
            for my $layer_id (0 .. $self->layer_count-2) {
                my $layerm          = $self->layers->[$layer_id]->regions->[$region_id];
                my $upper_layerm    = $self->layers->[$layer_id+1]->regions->[$region_id];
                my $perimeter_spacing       = $layerm->perimeter_flow->scaled_spacing;
                
                my $overlap = $perimeter_spacing;  # one perimeter
                
                my $diff = diff_ex(
                    [ offset([ map @{$_->expolygon}, @{$layerm->slices} ], -($Slic3r::Config->perimeters * $perimeter_spacing)) ],
                    [ offset([ map @{$_->expolygon}, @{$upper_layerm->slices} ], -$overlap) ],
                );
                next if !@$diff;
                # if we need more perimeters, $diff should contain a narrow region that we can collapse
                
                $diff = diff_ex(
                    [ map @$_, @$diff ],
                    [ offset(
                        [ offset([ map @$_, @$diff ], -$perimeter_spacing) ],
                        +$perimeter_spacing
                    ) ],
                );
                next if !@$diff;
                # diff contains the collapsed area
                
                foreach my $slice (@{$layerm->slices}) {
                    my $extra_perimeters = 0;
                    CYCLE: while (1) {
                        # compute polygons representing the thickness of the hypotetical new internal perimeter
                        # of our slice
                        $extra_perimeters++;
                        my $hypothetical_perimeter = diff_ex(
                            [ offset($slice->expolygon, -($perimeter_spacing * ($Slic3r::Config->perimeters + $extra_perimeters-1))) ],
                            [ offset($slice->expolygon, -($perimeter_spacing * ($Slic3r::Config->perimeters + $extra_perimeters))) ],
                        );
                        last CYCLE if !@$hypothetical_perimeter;  # no extra perimeter is possible
                        
                        # only add the perimeter if there's an intersection with the collapsed area
                        my $intersection = intersection_ex(
                            [ map @$_, @$diff ],
                            [ map @$_, @$hypothetical_perimeter ],
                        );
                        last CYCLE if !@$intersection;
                        Slic3r::debugf "  adding one more perimeter at layer %d\n", $layer_id;
                        $slice->extra_perimeters($extra_perimeters);
                    }
                }
            }
        }
    }
    
    Slic3r::parallelize(
        items => sub { 0 .. ($self->layer_count-1) },
        thread_cb => sub {
            my $q = shift;
            $Slic3r::Geometry::Clipper::clipper = Math::Clipper->new;
            my $result = {};
            while (defined (my $layer_id = $q->dequeue)) {
                my $layer = $self->layers->[$layer_id];
                $layer->make_perimeters;
                $result->{$layer_id} ||= {};
                foreach my $region_id (0 .. $#{$layer->regions}) {
                    my $layerm = $layer->regions->[$region_id];
                    $result->{$layer_id}{$region_id} = {
                        perimeters      => $layerm->perimeters,
                        fill_surfaces   => $layerm->fill_surfaces,
                        thin_fills      => $layerm->thin_fills,
                    };
                }
            }
            return $result;
        },
        collect_cb => sub {
            my $result = shift;
            foreach my $layer_id (keys %$result) {
                foreach my $region_id (keys %{$result->{$layer_id}}) {
                    $self->layers->[$layer_id]->regions->[$region_id]->$_($result->{$layer_id}{$region_id}{$_})
                        for qw(perimeters fill_surfaces thin_fills);
                }
            }
        },
        no_threads_cb => sub {
            $_->make_perimeters for @{$self->layers};
        },
    );
}

sub detect_surfaces_type {
    my $self = shift;
    Slic3r::debugf "Detecting solid surfaces...\n";
    
    # prepare a reusable subroutine to make surface differences
    my $surface_difference = sub {
        my ($subject_surfaces, $clip_surfaces, $result_type, $layerm) = @_;
        my $expolygons = diff_ex(
            [ map @$_, @$subject_surfaces ],
            [ map @$_, @$clip_surfaces ],
            1,
        );
        return map Slic3r::Surface->new(expolygon => $_, surface_type => $result_type),
            grep $_->is_printable($layerm->perimeter_flow->scaled_width),
            @$expolygons;
    };
    
    for my $region_id (0 .. ($self->print->regions_count-1)) {
        for my $i (0 .. ($self->layer_count-1)) {
            my $layerm = $self->layers->[$i]->regions->[$region_id];
            
            # comparison happens against the *full* slices (considering all regions)
            my $upper_layer = $self->layers->[$i+1];
            my $lower_layer = $i > 0 ? $self->layers->[$i-1] : undef;
            
            my (@bottom, @top, @internal) = ();
            
            # find top surfaces (difference between current surfaces
            # of current layer and upper one)
            if ($upper_layer) {
                @top = $surface_difference->(
                    [ map $_->expolygon, @{$layerm->slices} ],
                    $upper_layer->slices,
                    S_TYPE_TOP,
                    $layerm,
                );
            } else {
                # if no upper layer, all surfaces of this one are solid
                @top = @{$layerm->slices};
                $_->surface_type(S_TYPE_TOP) for @top;
            }
            
            # find bottom surfaces (difference between current surfaces
            # of current layer and lower one)
            if ($lower_layer) {
                # lower layer's slices are already Surface objects
                @bottom = $surface_difference->(
                    [ map $_->expolygon, @{$layerm->slices} ],
                    $lower_layer->slices,
                    S_TYPE_BOTTOM,
                    $layerm,
                );
            } else {
                # if no lower layer, all surfaces of this one are solid
                @bottom = @{$layerm->slices};
                $_->surface_type(S_TYPE_BOTTOM) for @bottom;
            }
            
            # now, if the object contained a thin membrane, we could have overlapping bottom
            # and top surfaces; let's do an intersection to discover them and consider them
            # as bottom surfaces (to allow for bridge detection)
            if (@top && @bottom) {
                my $overlapping = intersection_ex([ map $_->p, @top ], [ map $_->p, @bottom ]);
                Slic3r::debugf "  layer %d contains %d membrane(s)\n", $layerm->id, scalar(@$overlapping);
                @top = $surface_difference->([map $_->expolygon, @top], $overlapping, S_TYPE_TOP, $layerm);
            }
            
            # find internal surfaces (difference between top/bottom surfaces and others)
            @internal = $surface_difference->(
                [ map $_->expolygon, @{$layerm->slices} ],
                [ map $_->expolygon, @top, @bottom ],
                S_TYPE_INTERNAL,
                $layerm,
            );
            
            # save surfaces to layer
            @{$layerm->slices} = (@bottom, @top, @internal);
            
            Slic3r::debugf "  layer %d has %d bottom, %d top and %d internal surfaces\n",
                $layerm->id, scalar(@bottom), scalar(@top), scalar(@internal);
        }
        
        # clip surfaces to the fill boundaries
        foreach my $layer (@{$self->layers}) {
            my $layerm = $layer->regions->[$region_id];
            my $fill_boundaries = [ map @$_, @{$layerm->fill_surfaces} ];
            @{$layerm->fill_surfaces} = ();
            foreach my $surface (@{$layerm->slices}) {
                my $intersection = intersection_ex(
                    [ $surface->p ],
                    $fill_boundaries,
                );
                push @{$layerm->fill_surfaces}, map Slic3r::Surface->new
                    (expolygon => $_, surface_type => $surface->surface_type),
                    @$intersection;
            }
        }
    }
}

sub clip_fill_surfaces {
    my $self = shift;
    return unless $Slic3r::Config->infill_only_where_needed;
    
    # We only want infill under ceilings; this is almost like an
    # internal support material.
    
    my $additional_margin = scale 3;
    
    my @overhangs = ();
    for my $layer_id (reverse 0..$#{$self->layers}) {
        my $layer = $self->layers->[$layer_id];
        
        # clip this layer's internal surfaces to @overhangs
        foreach my $layerm (@{$layer->regions}) {
            my @new_internal = map Slic3r::Surface->new(
                    expolygon       => $_,
                    surface_type    => S_TYPE_INTERNAL,
                ),
                @{intersection_ex(
                    [ map @$_, @overhangs ],
                    [ map @{$_->expolygon}, grep $_->surface_type == S_TYPE_INTERNAL, @{$layerm->fill_surfaces} ],
                )};
            @{$layerm->fill_surfaces} = (
                @new_internal,
                (grep $_->surface_type != S_TYPE_INTERNAL, @{$layerm->fill_surfaces}),
            );
        }
        
        # get this layer's overhangs
        if ($layer_id > 0) {
            my $lower_layer = $self->layers->[$layer_id-1];
            # loop through layer regions so that we can use each region's
            # specific overhang width
            foreach my $layerm (@{$layer->regions}) {
                my $overhang_width = $layerm->overhang_width;
                # we want to support any solid surface, not just tops
                # (internal solids might have been generated)
                push @overhangs, map $_->offset_ex($additional_margin), @{intersection_ex(
                    [ map @{$_->expolygon}, grep $_->surface_type != S_TYPE_INTERNAL, @{$layerm->fill_surfaces} ],
                    [ map @$_, map $_->offset_ex(-$overhang_width), @{$lower_layer->slices} ],
                )};
            }
        }
    }
}

sub bridge_over_infill {
    my $self = shift;
    return if $Slic3r::Config->fill_density == 1;
    
    for my $layer_id (1..$#{$self->layers}) {
        my $layer       = $self->layers->[$layer_id];
        my $lower_layer = $self->layers->[$layer_id-1];
        
        foreach my $layerm (@{$layer->regions}) {
            # compute the areas needing bridge math 
            my @internal_solid = grep $_->surface_type == S_TYPE_INTERNALSOLID, @{$layerm->fill_surfaces};
            my @lower_internal = grep $_->surface_type == S_TYPE_INTERNAL, map @{$_->fill_surfaces}, @{$lower_layer->regions};
            my $to_bridge = intersection_ex(
                [ map $_->p, @internal_solid ],
                [ map $_->p, @lower_internal ],
            );
            next unless @$to_bridge;
            Slic3r::debugf "Bridging %d internal areas at layer %d\n", scalar(@$to_bridge), $layer_id;
            
            # build the new collection of fill_surfaces
            {
                my @new_surfaces = grep $_->surface_type != S_TYPE_INTERNALSOLID, @{$layerm->fill_surfaces};
                push @new_surfaces, map Slic3r::Surface->new(
                        expolygon       => $_,
                        surface_type    => S_TYPE_INTERNALBRIDGE,
                    ), @$to_bridge;
                push @new_surfaces, map Slic3r::Surface->new(
                        expolygon       => $_,
                        surface_type    => S_TYPE_INTERNALSOLID,
                    ), @{diff_ex(
                        [ map $_->p, @internal_solid ],
                        [ map @$_, @$to_bridge ],
                        1,
                    )};
                @{$layerm->fill_surfaces} = @new_surfaces;
            }
            
            # exclude infill from the layers below if needed
            # see discussion at https://github.com/alexrj/Slic3r/issues/240
            # Update: do not exclude any infill. Sparse infill is able to absorb the excess material.
            if (0) {
                my $excess = $layerm->extruders->{infill}->bridge_flow->width - $layerm->height;
                for (my $i = $layer_id-1; $excess >= $self->layers->[$i]->height; $i--) {
                    Slic3r::debugf "  skipping infill below those areas at layer %d\n", $i;
                    foreach my $lower_layerm (@{$self->layers->[$i]->regions}) {
                        my @new_surfaces = ();
                        # subtract the area from all types of surfaces
                        foreach my $group (Slic3r::Surface->group(@{$lower_layerm->fill_surfaces})) {
                            push @new_surfaces, map $group->[0]->clone(expolygon => $_),
                                @{diff_ex(
                                    [ map $_->p, @$group ],
                                    [ map @$_, @$to_bridge ],
                                )};
                            push @new_surfaces, map Slic3r::Surface->new(
                                expolygon       => $_,
                                surface_type    => S_TYPE_INTERNALVOID,
                            ), @{intersection_ex(
                                [ map $_->p, @$group ],
                                [ map @$_, @$to_bridge ],
                            )};
                        }
                        @{$lower_layerm->fill_surfaces} = @new_surfaces;
                    }
                    
                    $excess -= $self->layers->[$i]->height;
                }
            }
        }
    }
}

sub discover_horizontal_shells {
    my $self = shift;
    
    Slic3r::debugf "==> DISCOVERING HORIZONTAL SHELLS\n";
    
    for my $region_id (0 .. ($self->print->regions_count-1)) {
        for (my $i = 0; $i < $self->layer_count; $i++) {
            my $layerm = $self->layers->[$i]->regions->[$region_id];
            
            if ($Slic3r::Config->solid_infill_every_layers && ($i % $Slic3r::Config->solid_infill_every_layers) == 0) {
                $_->surface_type(S_TYPE_INTERNALSOLID)
                    for grep $_->surface_type == S_TYPE_INTERNAL, @{$layerm->fill_surfaces};
            }
            
            foreach my $type (S_TYPE_TOP, S_TYPE_BOTTOM) {
                # find slices of current type for current layer
                # get both slices and fill_surfaces before the former contains the perimeters area
                # and the latter contains the enlarged external surfaces
                my $solid = [ map $_->expolygon, grep $_->surface_type == $type, @{$layerm->slices}, @{$layerm->fill_surfaces} ];
                next if !@$solid;
                Slic3r::debugf "Layer %d has %s surfaces\n", $i, ($type == S_TYPE_TOP ? 'top' : 'bottom');
                
                my $solid_layers = ($type == S_TYPE_TOP)
                    ? $Slic3r::Config->top_solid_layers
                    : $Slic3r::Config->bottom_solid_layers;
                for (my $n = $type == S_TYPE_TOP ? $i-1 : $i+1; 
                        abs($n - $i) <= $solid_layers-1; 
                        $type == S_TYPE_TOP ? $n-- : $n++) {
                    
                    next if $n < 0 || $n >= $self->layer_count;
                    Slic3r::debugf "  looking for neighbors on layer %d...\n", $n;
                    
                    my @neighbor_fill_surfaces = @{$self->layers->[$n]->regions->[$region_id]->fill_surfaces};
                    
                    # find intersection between neighbor and current layer's surfaces
                    # intersections have contours and holes
                    my $new_internal_solid = intersection_ex(
                        [ map @$_, @$solid ],
                        [ map $_->p, grep { $_->surface_type == S_TYPE_INTERNAL || $_->surface_type == S_TYPE_INTERNALSOLID } @neighbor_fill_surfaces ],
                        undef, 1,
                    );
                    next if !@$new_internal_solid;
                    
                    # make sure the new internal solid is wide enough, as it might get collapsed when
                    # spacing is added in Fill.pm
                    {
                        my $margin = 3 * $layerm->solid_infill_flow->scaled_width; # require at least this size
                        my $too_narrow = diff_ex(
                            [ map @$_, @$new_internal_solid ],
                            [ offset([ offset([ map @$_, @$new_internal_solid ], -$margin) ], +$margin) ],
                            1,
                        );
                        
                        # if some parts are going to collapse, let's grow them and add the extra area to the neighbor layer
                        # as well as to our original surfaces so that we support this additional area in the next shell too
                        if (@$too_narrow) {
                            # make sure our grown surfaces don't exceed the fill area
                            my @grown = map @$_, @{intersection_ex(
                                [ offset([ map @$_, @$too_narrow ], +$margin) ],
                                [ map $_->p, @neighbor_fill_surfaces ],
                            )};
                            $new_internal_solid = union_ex([ @grown, (map @$_, @$new_internal_solid) ]);
                            $solid = union_ex([ @grown, (map @$_, @$solid) ]);
                        }
                    }
                    
                    # internal-solid are the union of the existing internal-solid surfaces
                    # and new ones
                    my $internal_solid = union_ex([
                        ( map $_->p, grep $_->surface_type == S_TYPE_INTERNALSOLID, @neighbor_fill_surfaces ),
                        ( map @$_, @$new_internal_solid ),
                    ]);
                    
                    # subtract intersections from layer surfaces to get resulting internal surfaces
                    my $internal = diff_ex(
                        [ map $_->p, grep $_->surface_type == S_TYPE_INTERNAL, @neighbor_fill_surfaces ],
                        [ map @$_, @$internal_solid ],
                        1,
                    );
                    Slic3r::debugf "    %d internal-solid and %d internal surfaces found\n",
                        scalar(@$internal_solid), scalar(@$internal);
                    
                    # assign resulting internal surfaces to layer
                    my $neighbor_fill_surfaces = $self->layers->[$n]->regions->[$region_id]->fill_surfaces;
                    @$neighbor_fill_surfaces = ();
                    push @$neighbor_fill_surfaces, Slic3r::Surface->new
                        (expolygon => $_, surface_type => S_TYPE_INTERNAL)
                        for @$internal;
                    
                    # assign new internal-solid surfaces to layer
                    push @$neighbor_fill_surfaces, Slic3r::Surface->new
                        (expolygon => $_, surface_type => S_TYPE_INTERNALSOLID)
                        for @$internal_solid;
                    
                    # assign top and bottom surfaces to layer
                    foreach my $s (Slic3r::Surface->group(grep { $_->surface_type == S_TYPE_TOP || $_->surface_type == S_TYPE_BOTTOM } @neighbor_fill_surfaces)) {
                        my $solid_surfaces = diff_ex(
                            [ map $_->p, @$s ],
                            [ map @$_, @$internal_solid, @$internal ],
                            1,
                        );
                        push @$neighbor_fill_surfaces, $s->[0]->clone(expolygon => $_)
                            for @$solid_surfaces;
                    }
                }
            }
        }
    }
}

# combine fill surfaces across layers
sub combine_infill {
    my $self = shift;
    return unless $Slic3r::Config->infill_every_layers > 1 && $Slic3r::Config->fill_density > 0;
    my $every = $Slic3r::Config->infill_every_layers;
    
    my $layer_count = $self->layer_count;
    my @layer_heights = map $self->layers->[$_]->height, 0 .. $layer_count-1;
    
    for my $region_id (0 .. ($self->print->regions_count-1)) {
        # limit the number of combined layers to the maximum height allowed by this regions' nozzle
        my $nozzle_diameter = $self->print->regions->[$region_id]->extruders->{infill}->nozzle_diameter;
        
        # define the combinations
        my @combine = ();   # layer_id => thickness in layers
        {
            my $current_height = my $layers = 0;
            for my $layer_id (1 .. $#layer_heights) {
                my $height = $self->layers->[$layer_id]->height;
                
                if ($current_height + $height >= $nozzle_diameter || $layers >= $every) {
                    $combine[$layer_id-1] = $layers;
                    $current_height = $layers = 0;
                }
                
                $current_height += $height;
                $layers++;
            }
        }
        
        # skip bottom layer
        for my $layer_id (1 .. $#combine) {
            next unless ($combine[$layer_id] // 1) > 1;
            my @layerms = map $self->layers->[$_]->regions->[$region_id],
                ($layer_id - ($combine[$layer_id]-1) .. $layer_id);
            
            # process internal and internal-solid infill separately
            for my $type (S_TYPE_INTERNAL, S_TYPE_INTERNALSOLID) {
                # we need to perform a multi-layer intersection, so let's split it in pairs
                
                # initialize the intersection with the candidates of the lowest layer
                my $intersection = [ map $_->expolygon, grep $_->surface_type == $type, @{$layerms[0]->fill_surfaces} ];
                
                # start looping from the second layer and intersect the current intersection with it
                for my $layerm (@layerms[1 .. $#layerms]) {
                    $intersection = intersection_ex(
                        [ map @$_, @$intersection ],
                        [ map @{$_->expolygon}, grep $_->surface_type == $type, @{$layerm->fill_surfaces} ],
                    );
                }
                
                my $area_threshold = $layerms[0]->infill_area_threshold;
                @$intersection = grep $_->area > $area_threshold, @$intersection;
                next if !@$intersection;
                Slic3r::debugf "  combining %d %s regions from layers %d-%d\n",
                    scalar(@$intersection),
                    ($type == S_TYPE_INTERNAL ? 'internal' : 'internal-solid'),
                    $layer_id-($every-1), $layer_id;
                
                # $intersection now contains the regions that can be combined across the full amount of layers
                # so let's remove those areas from all layers
                
                 my @intersection_with_clearance = map $_->offset(
                       $layerms[-1]->solid_infill_flow->scaled_width    / 2
                     + $layerms[-1]->perimeter_flow->scaled_width / 2
                     # Because fill areas for rectilinear and honeycomb are grown 
                     # later to overlap perimeters, we need to counteract that too.
                     + (($type == S_TYPE_INTERNALSOLID || $Slic3r::Config->fill_pattern =~ /(rectilinear|honeycomb)/)
                       ? $layerms[-1]->solid_infill_flow->scaled_width * &Slic3r::INFILL_OVERLAP_OVER_SPACING
                       : 0)
                     ), @$intersection;

                
                foreach my $layerm (@layerms) {
                    my @this_type   = grep $_->surface_type == $type, @{$layerm->fill_surfaces};
                    my @other_types = grep $_->surface_type != $type, @{$layerm->fill_surfaces};
                    
                    my @new_this_type = map Slic3r::Surface->new(expolygon => $_, surface_type => $type),
                        @{diff_ex(
                            [ map @{$_->expolygon}, @this_type ],
                            [ @intersection_with_clearance ],
                        )};
                    
                    # apply surfaces back with adjusted depth to the uppermost layer
                    if ($layerm->id == $layer_id) {
                        push @new_this_type,
                            map Slic3r::Surface->new(
                                expolygon        => $_,
                                surface_type     => $type,
                                thickness        => sum(map $_->height, @layerms),
                                thickness_layers => scalar(@layerms),
                            ),
                            @$intersection;
                    } else {
                        # save void surfaces
                        push @this_type,
                            map Slic3r::Surface->new(expolygon => $_, surface_type => S_TYPE_INTERNALVOID),
                            @{intersection_ex(
                                [ map @{$_->expolygon}, @this_type ],
                                [ @intersection_with_clearance ],
                            )};
                    }
                    
                    @{$layerm->fill_surfaces} = (@new_this_type, @other_types);
                }
            }
        }
    }
}

sub generate_support_material {
    my $self = shift;
    return if $self->layer_count < 2;
    
    my $margin      = scale 5;
    my $keep_margin = scale 3;
    my $threshold_rad;
    if ($Slic3r::Config->support_material_threshold) {
        $threshold_rad = deg2rad($Slic3r::Config->support_material_threshold + 1);  # +1 makes the threshold inclusive
        Slic3r::debugf "Threshold angle = %d°\n", rad2deg($threshold_rad);
    }
    
    # Move down?
    my $flow            = $self->print->support_material_flow;
    my $circle_distance = 8 * $flow->scaled_width;
    my $circle;
    {
        # TODO: make sure teeth between circles are compatible with support material flow
        my $r = 3 * $flow->scaled_width;
        $circle = Slic3r::Polygon->new([ map [ $r * cos $_, $r * sin $_ ], (5*PI/3, 4*PI/3, PI, 2*PI/3, PI/3, 0) ]);
    }
    my $pattern_spacing = ($Slic3r::Config->support_material_spacing > $flow->spacing)
        ? $Slic3r::Config->support_material_spacing
        : $flow->spacing;
    
    # determine contact areas
    my %contact  = ();  # layer_id => [ expolygons ]
    my %overhang = ();  # layer_id => [ expolygons ] - this stores the actual overhang supported by each contact layer
    for my $layer_id (1 .. $#{$self->layers}) {
        my $layer = $self->layers->[$layer_id];
        my $lower_layer = $self->layers->[$layer_id-1];
        
        # detect contact areas needed to support this layer
        my (@overhang, @contact) = ();
        foreach my $layerm (@{$layer->regions}) {
            my $fw = $layerm->perimeter_flow->scaled_width;
            my $diff;
            
            # If a threshold angle was specified, use a different logic for detecting overhangs.
            if (defined $threshold_rad) {
                my $d = scale $lower_layer->height * ((cos $threshold_rad) / (sin $threshold_rad));
                $diff = diff(
                    [ offset([ map $_->p, @{$layerm->slices} ], -$d) ],
                    [ map @$_, @{$lower_layer->slices} ],
                );
                $diff = diff(
                    [ offset($diff, $d - $fw/2) ],
                    [ map @$_, @{$lower_layer->slices} ],
                );
            } else {
                $diff = diff(
                    [ offset([ map $_->p, @{$layerm->slices} ], -$fw/2) ],
                    [ map @$_, @{$lower_layer->slices} ],
                );
                # $diff now contains the ring or stripe comprised between the boundary of 
                # lower slices and the centerline of the last perimeter in this overhanging layer.
            }
            
            next if !@$diff;
            push @overhang, @{union_ex($diff)};
            
            # Let's define the required contact area by using a max gap of half the upper 
            # extrusion width. 
            $diff = diff_ex(
                [ offset($diff, $fw/2 + $margin) ],
                [ offset([ map @$_, @{$lower_layer->slices} ], $fw/2) ],
            );
            push @contact, @$diff;
        }
        next if !@contact;
        
        # now apply the contact areas to the layer were they need to be made
        {
            # get the average nozzle diameter used on this layer
            my @nozzle_diameters = map $_->nozzle_diameter,
                map { $_->perimeter_flow, $_->solid_infill_flow }
                @{$layer->regions};
            my $nozzle_diameter = sum(@nozzle_diameters)/@nozzle_diameters;
            
            # find the layer of our contact areas
            my $h = $nozzle_diameter;
            my $contact_layer = $layer;
            while ($h > $contact_layer->height) {
                $h -= $contact_layer->height;
                my $next = $contact_layer->id - 1;
                last if $next < 0;
                $contact_layer = $self->layers->[$next];
            }
            
            $contact_layer->support_material_contact_height($contact_layer->height - $h);
            $contact{$contact_layer->id}  = [ @contact ];
            $overhang{$contact_layer->id} = [ @overhang ];
        }
    }
    
    # determine depth for each layer (similar logic as combine_infill) using the maximum allowed layer height
    my $support_material_height = $flow->nozzle_diameter;
    
    # define the combinations
    my @combine = ();   # layer_id => thickness in layers
    {
        my @layer_heights = map $self->layers->[$_]->height, 0..$#{$self->layers};
        my $current_height = 0;
        my @layers = ();
        for my $layer_id (1 .. $#layer_heights) {  # Layer 0 is not available for combination because of the flange!
            my $height = $self->layers->[$layer_id]->height;
            
            if ($current_height + $height > $support_material_height) {
                $combine[$layer_id-1] = [@layers];
                $current_height = 0;
                @layers = ();
            }
            
            $current_height += $height;
            push @layers, $layer_id;
        }
    }
    
    # Let's now determine shells (interface layers) and normal support below them.
    my %interface = ();  # layer_id => [ expolygons ]
    my %support   = ();  # layer_id => [ expolygons ]
    for my $layer_id (sort keys %contact) {
        my $this = $contact{$layer_id};
        # count contact layer as interface layer
        for (my $i = $layer_id-1; $i >= 0 && $i > $layer_id-$Slic3r::Config->support_material_interface_layers; $i--) {
            my $layer = $self->layers->[$i];
            
            # Compute interface area on this layer as diff of upper contact area
            # (or upper interface area) and layer slices.
            # This diff is responsible of the contact between support material and
            # the top surfaces of the object. We should probably offset the layer 
            # slices before performing the diff, but this needs investigation.
            $this = $interface{$i} = diff_ex(
                [
                    (map @$_, @$this),
                    (map @$_, @{ $interface{$i} || [] }),
                ],
                [
                    (map @$_, @{$layer->slices}),
                    (map @$_, @{$contact{$i}}),
                ],
            );
        }
        
        # determine what layers does our support belong to
        for (my $i = $layer_id-$Slic3r::Config->support_material_interface_layers; $i >= 0; $i -= $combine[$i] ? @{$combine[$i]} : 1) {
            my $layer = $self->layers->[$i];
            
            # Compute support area on this layer as diff of upper support area
            # and layer slices.
            $this = $support{$i} = diff_ex(
                [
                    (map @$_, @$this),
                    (map @$_, @{ $support{$i} || [] }),
                ],
                [
                    (map @$_, @{$layer->slices}),
                    (map @$_, @{$contact{$i}}),
                    (map @$_, @{$interface{$i}}),
                ],
            );
        }
    }

    Slic3r::debugf "Generating patterns\n";
    
    # prepare fillers
    my $pattern = $Slic3r::Config->support_material_pattern;
    my @angles = ($Slic3r::Config->support_material_angle);
    if ($pattern eq 'rectilinear-grid') {
        $pattern = 'rectilinear';
        push @angles, $angles[0] + 90;
    }
    
    my $fill = Slic3r::Fill->new(print => $self->print);
    my %fillers = (
        interface   => $fill->filler('rectilinear'),
        support     => $fill->filler($pattern),
    );
    
    # if pattern is not cross-hatched, rotate the interface pattern by 90° degrees
    $fillers{interface}->angle($Slic3r::Config->support_material_angle + 90);
    
    my $interface_spacing = $Slic3r::Config->support_material_interface_spacing;
    my $interface_density = $interface_spacing == 0 ? 1 : $flow->spacing / $interface_spacing;
    my $support_spacing = $Slic3r::Config->support_material_spacing;
    my $support_density = $support_spacing == 0 ? 1 : $flow->spacing / $support_spacing;
    
    my $process_layer = sub {
        my ($layer_id) = @_;
        my $layer = $self->layers->[$layer_id];
        
        my $result = {};
        
        # contact
        if ($contact{$layer_id} && @{$contact{$layer_id}}) {
            # find centerline of the external loop of the contours
            my @loops = offset([ map @$_, @{$contact{$layer_id}} ], -$flow->scaled_width/2);
        
            # apply a pattern to the loop
            {
                my @positions = map Slic3r::Polygon->new($_)->split_at_first_point->regular_points($circle_distance), @loops;
                # TODO: only consider positions close to the object
                @loops = @{diff(
                    [ @loops ],
                    [ map $circle->clone->translate(@$_), @positions ],
                )};
            }
            
            # make a second loop
            my @inner = offset(
                [ offset([ @loops ], -1.5*$flow->scaled_spacing) ], 
                +0.5*$flow->scaled_spacing,
            );
            @loops = @{ Boost::Geometry::Utils::multi_polygon_multi_linestring_intersection(
                [ offset_ex([ map @$_, @{$overhang{$layer_id}} ], +$keep_margin) ],
                [ map Slic3r::Polygon->new($_)->split_at_first_point, @loops, @inner ],
            ) };
            @loops = map Slic3r::ExtrusionPath->pack(
                polyline        => Slic3r::Polyline->new(@$_),
                role            => EXTR_ROLE_SUPPORTMATERIAL,
                height          => $layer->support_material_contact_height,
                flow_spacing    => $flow->spacing,
            ), @loops;
            
            # fill the interior
            my @paths = ();
            foreach my $expolygon (@{ union_ex([ offset([ @inner ], -$flow->scaled_spacing) ]) }) {
                my @p = $fillers{interface}->fill_surface(
                    Slic3r::Surface->new(expolygon => $expolygon),
                    density         => $interface_density,
                    flow_spacing    => $flow->spacing,
                );
                my $params = shift @p;
                
                push @paths, map Slic3r::ExtrusionPath->new(
                    polyline        => Slic3r::Polyline->new(@$_),
                    role            => EXTR_ROLE_SUPPORTMATERIAL,
                    height          => $layer->support_material_contact_height,
                    flow_spacing    => $params->{flow_spacing},
                ), @p;
            }
            
            $result->{contact} = [ @loops, @paths ];
        }
        
        # interface
        {
            my @paths = ();
            foreach my $expolygon (@{ $interface{$layer_id} }) {
                my @p = $fillers{interface}->fill_surface(
                    Slic3r::Surface->new(expolygon => $expolygon),
                    density         => $interface_density,
                    flow_spacing    => $flow->spacing,
                );
                my $params = shift @p;
                
                push @paths, map Slic3r::ExtrusionPath->new(
                    polyline        => Slic3r::Polyline->new(@$_),
                    role            => EXTR_ROLE_SUPPORTMATERIAL,
                    height          => undef,
                    flow_spacing    => $params->{flow_spacing},
                ), @p;
            }
            
            $result->{interface} = [ @paths ];
        }
        
        # support or flange
        {
            $fillers{support}->angle($angles[ ($layer_id  / ($combine[$layer_id] ? @{$combine[$layer_id]} : 1)) % @angles ]);
            my $density         = $support_density;
            my $flow_spacing    = $flow->spacing;
            
            # base flange
            if ($layer_id == 0) {
                $density        = 0.5;
                $flow_spacing   = $self->print->first_layer_support_material_flow->spacing;
            }
            
            my @paths = ();
            foreach my $expolygon (@{ $support{$layer_id} }) {
                my @p = $fillers{support}->fill_surface(
                    Slic3r::Surface->new(expolygon => $expolygon),
                    density         => $density,
                    flow_spacing    => $flow_spacing,
                );
                my $params = shift @p;
                
                push @paths, map Slic3r::ExtrusionPath->new(
                    polyline        => Slic3r::Polyline->new(@$_),
                    role            => EXTR_ROLE_SUPPORTMATERIAL,
                    height          => $combine[$layer_id] ? sum(map $self->layers->[$_]->height, @{$combine[$layer_id]}) : undef,
                    flow_spacing    => $params->{flow_spacing},
                ), @p;
            }
            
            $result->{support} = [ @paths ];
        }
        
        # islands
        $result->{islands} = union_ex([
            map @$_, @{$contact{$layer_id}}, @{$interface{$layer_id}}, @{$support{$layer_id}},
        ]);
        
        return $result;
    };
    
    my $apply = sub {
        my ($layer_id, $result) = @_;
        my $layer = $self->layers->[$layer_id];
        $layer->support_contact_fills(Slic3r::ExtrusionPath::Collection->new(paths => $result->{contact})) if $result->{contact};
        $layer->support_fills(Slic3r::ExtrusionPath::Collection->new(paths => [ @{$result->{interface}}, @{$result->{support}} ]));
        $layer->support_islands($result->{islands});
    };
    Slic3r::parallelize(
        items => [ 0 .. $#{$self->layers} ],
        thread_cb => sub {
            my $q = shift;
            $Slic3r::Geometry::Clipper::clipper = Math::Clipper->new;
            my $result = {};
            while (defined (my $layer_id = $q->dequeue)) {
                $result->{$layer_id} = $process_layer->($layer_id);
            }
            return $result;
        },
        collect_cb => sub {
            my $result = shift;
            $apply->($_, $result->{$_}) for keys %$result;
        },
        no_threads_cb => sub {
            $apply->($_, $process_layer->($_)) for 0 .. $#{$self->layers};
        },
    );
}

1;

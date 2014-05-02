package Slic3r::Layer::Region;
use Moo;

use List::Util qw(sum first);
use Slic3r::ExtrusionPath ':roles';
use Slic3r::Flow ':roles';
use Slic3r::Geometry qw(PI A B scale unscale chained_path points_coincide);
use Slic3r::Geometry::Clipper qw(union_ex diff_ex intersection_ex 
    offset offset_ex offset2 offset2_ex union_pt diff intersection
    union diff);
use Slic3r::Surface ':types';

has 'layer' => (
    is          => 'ro',
    weak_ref    => 1,
    required    => 1,
    handles     => [qw(id slice_z print_z height object print)],
);
has 'region'            => (is => 'ro', required => 1, handles => [qw(config)]);
has 'infill_area_threshold' => (is => 'lazy');

# collection of surfaces generated by slicing the original geometry
# divided by type top/bottom/internal
has 'slices' => (is => 'rw', default => sub { Slic3r::Surface::Collection->new });

# collection of extrusion paths/loops filling gaps
has 'thin_fills' => (is => 'rw', default => sub { Slic3r::ExtrusionPath::Collection->new });

# collection of surfaces for infill generation
has 'fill_surfaces' => (is => 'rw', default => sub { Slic3r::Surface::Collection->new });

# collection of expolygons representing the bridged areas (thus not needing support material)
has 'bridged' => (is => 'rw', default => sub { Slic3r::ExPolygon::Collection->new });

# collection of polylines representing the unsupported bridge edges
has 'unsupported_bridge_edges' => (is => 'rw', default => sub { Slic3r::Polyline::Collection->new });

# ordered collection of extrusion paths/loops to build all perimeters
has 'perimeters' => (is => 'rw', default => sub { Slic3r::ExtrusionPath::Collection->new });

# ordered collection of extrusion paths to fill surfaces
has 'fills' => (is => 'rw', default => sub { Slic3r::ExtrusionPath::Collection->new });

# debug - gaps
has 'dbg_gaps' => (is => 'rw', default => sub { Slic3r::Surface::Collection->new });

sub _build_infill_area_threshold {
    my $self = shift;
    return $self->flow(FLOW_ROLE_SOLID_INFILL)->scaled_spacing ** 2;
}

sub flow {
    my ($self, $role, $bridge, $width) = @_;
    return $self->region->flow(
        $role,
        $self->layer->height,
        $bridge // 0,
        $self->layer->id == 0,
        $width,
        $self->object,
    );
}

sub make_perimeters {
    my $self = shift;
    
    my $perimeter_flow      = $self->flow(FLOW_ROLE_PERIMETER);
    my $mm3_per_mm          = $perimeter_flow->mm3_per_mm($self->height);
    my $pwidth              = $perimeter_flow->scaled_width;
    my $pspacing            = $perimeter_flow->scaled_spacing;
    my $solid_infill_flow   = $self->flow(FLOW_ROLE_SOLID_INFILL);
    my $ispacing            = $solid_infill_flow->scaled_spacing;
    my $gap_area_threshold  = $pwidth ** 2;
    
    # Calculate the minimum required spacing between two adjacent traces.
    # This should be equal to the nominal flow spacing but we experiment
    # with some tolerance in order to avoid triggering medial axis when
    # some squishing might work. Loops are still spaced by the entire
    # flow spacing; this only applies to collapsing parts.
    my $min_spacing = $pspacing * (1 - &Slic3r::INSET_OVERLAP_TOLERANCE);
    
    $self->perimeters->clear;
    $self->fill_surfaces->clear;
    $self->thin_fills->clear;
    $self->dbg_gaps->clear;
    my @contours    = ();    # array of Polygons with ccw orientation
    my @holes       = ();    # array of Polygons with cw orientation
    my @thin_walls  = ();    # array of ExPolygons
    
    # we need to process each island separately because we might have different
    # extra perimeters for each one
    foreach my $surface (@{$self->slices}) {
        # detect how many perimeters must be generated for this island
        my $loop_number = $self->config->perimeters + ($surface->extra_perimeters || 0);
        
        my @last = @{$surface->expolygon};
        my @gaps = ();    # array of ExPolygons
        if ($loop_number > 0) {
            # we loop one time more than needed in order to find gaps after the last perimeter was applied
            for my $i (1 .. ($loop_number+1)) {  # outer loop is 1
                my @offsets = ();
                if ($i == 1) {
                    # the minimum thickness of a single loop is:
                    # width/2 + spacing/2 + spacing/2 + width/2
                    @offsets = @{offset2(
                        \@last,
                        -(0.5*$pwidth + 0.5*$min_spacing - 1),
                        +(0.5*$min_spacing - 1),
                    )};
                    
                    # look for thin walls
                    if ($self->config->thin_walls) {
                        my $diff = diff_ex(
                            \@last,
                            offset(\@offsets, +0.5*$pwidth),
                            1,  # medial axis requires non-overlapping geometry
                        );
                        push @thin_walls, @$diff;
                    }
                } else {
                    @offsets = @{offset2(
                        \@last,
                        -(1.0*$pspacing + 0.5*$min_spacing - 1),
                        +(0.5*$min_spacing - 1),
                    )};
                
                    # look for gaps
                    if ($self->print->config->gap_fill_speed > 0 && $self->config->fill_density > 0) {
                        # not using safety offset here would "detect" very narrow gaps
                        # (but still long enough to escape the area threshold) that gap fill
                        # won't be able to fill but we'd still remove from infill area
                        my $diff = diff_ex(
                            offset(\@last, -0.5*$pspacing),
                            offset(\@offsets, +0.5*$pspacing + 10),  # safety offset
                        );
                        push @gaps, grep abs($_->area) >= $gap_area_threshold, @$diff;
                    }
                }
            
                last if !@offsets;
                last if $i > $loop_number; # we were only looking for gaps this time
            
                # clone polygons because these ExPolygons will go out of scope very soon
                @last = @offsets;
                foreach my $polygon (@offsets) {
                    if ($polygon->is_counter_clockwise) {
                        push @contours, $polygon;
                    } else {
                        push @holes, $polygon;
                    }
                }
            }
        }

        if (0) {
            require "Slic3r/SVG.pm";
            Slic3r::SVG::output(
                "perimeters_b_" . $self->layer->id . ".svg",
                no_arrows => 1,
                green_polygons_outline => \@contours,
                blue_polygons_outline => \@holes,
                magenta_polygons => \@last,
                cyan_expolygons => \@gaps,
                );
        }
    
        # fill gaps
        if (@gaps) {
            if (0) {
                require "Slic3r/SVG.pm";
                Slic3r::SVG::output(
                    "gaps.svg",
                    expolygons => \@gaps,
                );
            }
            
            # where $pwidth < thickness < 2*$pspacing, infill with width = 1.5*$pwidth
            # where 0.5*$pwidth < thickness < $pwidth, infill with width = 0.5*$pwidth
            my @gap_sizes = (
                [ $pwidth, 2*$pspacing, unscale 1.5*$pwidth ],
                [ 0.5*$pwidth, $pwidth, unscale 0.5*$pwidth ],
            );
            foreach my $gap_size (@gap_sizes) {
                my @gap_fill = $self->_fill_gaps(@$gap_size, \@gaps);
                $self->thin_fills->append(@gap_fill);
            
                # Make sure we don't infill narrow parts that are already gap-filled
                # (we only consider this surface's gaps to reduce the diff() complexity).
                # Growing actual extrusions ensures that gaps not filled by medial axis
                # are not subtracted from fill surfaces (they might be too short gaps
                # that medial axis skips but infill might join with other infill regions
                # and use zigzag).
                my $w = $gap_size->[2];
                my @filled = map {
                    @{($_->isa('Slic3r::ExtrusionLoop') ? $_->split_at_first_point : $_)
                        ->polyline
                        ->grow(scale $w/2)};
                } @gap_fill;
                @last = @{diff(\@last, \@filled)};
            }
        }
        
        # create one more offset to be used as boundary for fill
        # we offset by half the perimeter spacing (to get to the actual infill boundary)
        # and then we offset back and forth by half the infill spacing to only consider the
        # non-collapsing regions
        my $min_perimeter_infill_spacing = $ispacing * (1 - &Slic3r::INSET_OVERLAP_TOLERANCE);
        $self->fill_surfaces->append(
            map Slic3r::Surface->new(expolygon => $_, surface_type => S_TYPE_INTERNAL),  # use a bogus surface type
            @{offset2_ex(
                [ map @{$_->simplify_p(&Slic3r::SCALED_RESOLUTION)}, @{union_ex(\@last)} ],
                -($pspacing/2 + $min_perimeter_infill_spacing/2),
                +$min_perimeter_infill_spacing/2,
            )}
        );
    }
    
    
    # process thin walls by collapsing slices to single passes
    my @thin_wall_polylines = ();
    if (@thin_walls) {
        # the following offset2 ensures almost nothing in @thin_walls is narrower than $min_width
        # (actually, something larger than that still may exist due to mitering or other causes)
        my $min_width = $pwidth / 4;
        @thin_walls = @{offset2_ex([ map @$_, @thin_walls ], -$min_width/2, +$min_width/2)};
        
        # the maximum thickness of our thin wall area is equal to the minimum thickness of a single loop
        @thin_wall_polylines = map @{$_->medial_axis($pwidth + $pspacing, $min_width)}, @thin_walls;
        Slic3r::debugf "  %d thin walls detected\n", scalar(@thin_wall_polylines) if $Slic3r::debug;
        
        if (1) {
            require "Slic3r/SVG.pm";
            Slic3r::SVG::output(
                "medial_axis_".$self->layer->id.".svg",
                no_arrows => 1,
                expolygons      => \@thin_walls,
                green_polylines => [ map $_->polygon->split_at_first_point, @{$self->perimeters} ],
                red_polylines   => \@thin_wall_polylines,
            );
        }
    }
    
    # find nesting hierarchies separately for contours and holes
    my $contours_pt = union_pt(\@contours);
    my $holes_pt    = union_pt(\@holes);
    
    # prepare a coderef for traversing the PolyTree object
    # external contours are root items of $contours_pt
    # internal contours are the ones next to external
    my $traverse;
    $traverse = sub {
        my ($polynodes, $depth, $is_contour) = @_;
        
        # convert all polynodes to ExtrusionLoop objects
        my $collection = Slic3r::ExtrusionPath::Collection->new;
        my @children = ();
        foreach my $polynode (@$polynodes) {
            my $polygon = ($polynode->{outer} // $polynode->{hole})->clone;
            
            # return ccw contours and cw holes
            # GCode.pm will convert all of them to ccw, but it needs to know
            # what the holes are in order to compute the correct inwards move
            if ($is_contour) {
                $polygon->make_counter_clockwise;
            } else {
                $polygon->make_clockwise;
            }
            
            my $role = EXTR_ROLE_PERIMETER;
            if ($is_contour ? $depth == 0 : !@{ $polynode->{children} }) {
                # external perimeters are root level in case of contours
                # and items with no children in case of holes
                $role = EXTR_ROLE_EXTERNAL_PERIMETER;
            } elsif ($depth == 1 && $is_contour) {
                $role = EXTR_ROLE_CONTOUR_INTERNAL_PERIMETER;
            }
            
            $collection->append(Slic3r::ExtrusionLoop->new(
                polygon         => $polygon,
                role            => $role,
                mm3_per_mm      => $mm3_per_mm,
                width           => $perimeter_flow->width,
                height          => $self->height,
            ));
            
            # save the children
            push @children, $polynode->{children};
        }

        # if we're handling the top-level contours, add thin walls as candidates too
        # in order to include them in the nearest-neighbor search
        if ($is_contour && $depth == 0) {
            foreach my $polyline (@thin_wall_polylines) {
                $collection->append(Slic3r::ExtrusionPath->new(
                    polyline        => $polyline,
                    role            => EXTR_ROLE_EXTERNAL_PERIMETER,
                    mm3_per_mm      => $mm3_per_mm,
                    width           => $perimeter_flow->width,
                    height          => $self->height,
                ));
            }
        }
        
        # use a nearest neighbor search to order these children
        # TODO: supply second argument to chained_path() too?
        # Optimization: since islands are going to be sorted by slice anyway in the 
        # G-code export process, we skip chained_path here
        my ($sorted_collection, @orig_indices);
        if ($is_contour && $depth == 0) {
            $sorted_collection = $collection;
            @orig_indices = (0..$#$sorted_collection);
        } else {
            $sorted_collection = $collection->chained_path_indices(0);
            @orig_indices = @{$sorted_collection->orig_indices};
        }
        
        my @loops = ();
        foreach my $loop (@$sorted_collection) {
            my $orig_index = shift @orig_indices;
            
            if ($loop->isa('Slic3r::ExtrusionPath')) {
                push @loops, $loop->clone;
            } else {
                # if this is an external contour find all holes belonging to this contour(s)
                # and prepend them
                if ($is_contour && $depth == 0) {
                    # $loop is the outermost loop of an island
                    my @holes = ();
                    for (my $i = 0; $i <= $#$holes_pt; $i++) {
                        if ($loop->polygon->contains_point($holes_pt->[$i]{outer}->first_point)) {
                            push @holes, splice @$holes_pt, $i, 1;  # remove from candidates to reduce complexity
                            $i--;
                        }
                    }
                    
                    # order holes efficiently
                    @holes = @holes[@{chained_path([ map {($_->{outer} // $_->{hole})->first_point} @holes ])}];
                    
                    push @loops, reverse map $traverse->([$_], 0, 0), @holes;
                }
                
                # traverse children and prepend them to this loop
                push @loops, $traverse->($children[$orig_index], $depth+1, $is_contour);
                push @loops, $loop->clone;
            }
        }
        return @loops;
    };
    
    # order loops from inner to outer (in terms of object slices)
    my @loops = $traverse->($contours_pt, 0, 1);
    
    # if brim will be printed, reverse the order of perimeters so that
    # we continue inwards after having finished the brim
    # TODO: add test for perimeter order
    @loops = reverse @loops
        if $self->print->config->external_perimeters_first
            || ($self->layer->id == 0 && $self->print->config->brim_width > 0);
    
    # append perimeters
    $self->perimeters->append(@loops);
}

sub _fill_gaps {
    my ($self, $min, $max, $w, $gaps) = @_;
    
    my $this = diff_ex(
        offset2([ map @$_, @$gaps ], -$min/2, +$min/2),
        offset2([ map @$_, @$gaps ], -$max/2, +$max/2),
        1,
    );

    my $flow = $self->flow(FLOW_ROLE_SOLID_INFILL, 0, $w);
    my %path_args = (
        role        => EXTR_ROLE_GAPFILL,
        mm3_per_mm  => $flow->mm3_per_mm($self->height),
        width       => $flow->width,
        height      => $self->height,
    );
    my @polylines = map @{$_->medial_axis($max, $min/2)}, @$this;
    
    Slic3r::debugf "  %d gaps filled with extrusion width = %s\n", scalar @$this, $w
        if @$this;
    
    return map {
        $_->isa('Slic3r::Polygon')
            ? Slic3r::ExtrusionLoop->new(polygon => $_, %path_args)->split_at_first_point  # should we keep these as loops?
            : Slic3r::ExtrusionPath->new(polyline => $_, %path_args),
    } @polylines;
}

sub prepare_fill_surfaces {
    my $self = shift;
    
    # if no solid layers are requested, turn top/bottom surfaces to internal
    if ($self->config->top_solid_layers == 0) {
        $_->surface_type(S_TYPE_INTERNAL) for @{$self->fill_surfaces->filter_by_type(S_TYPE_TOP)};
    }
    if ($self->config->bottom_solid_layers == 0) {
        $_->surface_type(S_TYPE_INTERNAL)
            for @{$self->fill_surfaces->filter_by_type(S_TYPE_BOTTOM)}, @{$self->fill_surfaces->filter_by_type(S_TYPE_BOTTOMBRIDGE)};
    }
        
    # turn too small internal regions into solid regions according to the user setting
    if ($self->config->fill_density > 0) {
        my $min_area = scale scale $self->config->solid_infill_below_area; # scaling an area requires two calls!
        $_->surface_type(S_TYPE_INTERNALSOLID)
            for grep { $_->area <= $min_area } @{$self->fill_surfaces->filter_by_type(S_TYPE_INTERNAL)};
    }
}

sub process_external_surfaces {
    my ($self, $lower_layer) = @_;
    
    my @surfaces = @{$self->fill_surfaces};
    my $margin = scale &Slic3r::EXTERNAL_INFILL_MARGIN;
 
    if (1) {
    require "Slic3r/SVG.pm";
    Slic3r::SVG::output(
        "surfaces_before_" . $self->layer->id . ".svg",
        red_polygons   => [ map $_->p, grep $_->surface_type == S_TYPE_TOP, @surfaces ],
        green_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_BOTTOM, @surfaces ],
        blue_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_BOTTOMBRIDGE, @surfaces ],
        magenta_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNAL, @surfaces ],
        cyan_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNALSOLID, @surfaces ],
        gray_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNALBRIDGE, @surfaces ],
        magenta_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNALVOID, @surfaces ],
        no_arrows => 1,
            );
    }
  
    my @bottom = ();
    foreach my $surface (grep $_->is_bottom, @surfaces) {
        my $grown = $surface->expolygon->offset_ex(+$margin);
        
        # detect bridge direction before merging grown surfaces otherwise adjacent bridges
        # would get merged into a single one while they need different directions
        # also, supply the original expolygon instead of the grown one, because in case
        # of very thin (but still working) anchors, the grown expolygon would go beyond them
        my $angle;
        if ($lower_layer) {
            my $bridge_detector = Slic3r::Layer::BridgeDetector->new(
                expolygon       => $surface->expolygon,
                lower_slices    => $lower_layer->slices,
                extrusion_width => $self->flow(FLOW_ROLE_INFILL, $self->height, 1)->scaled_width,
            );
            Slic3r::debugf "Processing bridge at layer %d:\n", $self->id;
            $angle = $bridge_detector->detect_angle;
            
            if (defined $angle && $self->object->config->support_material) {
                $self->bridged->append(@{ $bridge_detector->coverage($angle) });
                $self->unsupported_bridge_edges->append(@{ $bridge_detector->unsupported_edges }); 
            }
        }
        
        push @bottom, map $surface->clone(expolygon => $_, bridge_angle => $angle), @$grown;
    }
    
    my @top = ();
    foreach my $surface (grep $_->surface_type == S_TYPE_TOP, @surfaces) {
        # give priority to bottom surfaces
        my $grown = diff_ex(
            $surface->expolygon->offset(+$margin),
            [ map $_->p, @bottom ],
        );
        push @top, map $surface->clone(expolygon => $_), @$grown;
    }
    
    # if we're slicing with no infill, we can't extend external surfaces
    # over non-existent infill
    my @fill_boundaries = $self->config->fill_density > 0
        ? @surfaces
        : grep $_->surface_type != S_TYPE_INTERNAL, @surfaces;
    
    # intersect the grown surfaces with the actual fill boundaries
    my @new_surfaces = ();
    foreach my $group (@{Slic3r::Surface::Collection->new(@top, @bottom)->group}) {
        push @new_surfaces,
            map $group->[0]->clone(expolygon => $_),
            @{intersection_ex(
                [ map $_->p, @$group ],
                [ map $_->p, @fill_boundaries ],
                1,  # to ensure adjacent expolygons are unified
            )};
    }
    
    # subtract the new top surfaces from the other non-top surfaces and re-add them
    my @other = grep $_->surface_type != S_TYPE_TOP && !$_->is_bottom, @surfaces;
    foreach my $group (@{Slic3r::Surface::Collection->new(@other)->group}) {
        push @new_surfaces, map $group->[0]->clone(expolygon => $_), @{diff_ex(
            [ map $_->p, @$group ],
            [ map $_->p, @new_surfaces ],
        )};
    }
    $self->fill_surfaces->clear;
    $self->fill_surfaces->append(@new_surfaces);

    if (1) {
    require "Slic3r/SVG.pm";
    Slic3r::SVG::output(
        "surfaces_" . $self->layer->id . ".svg",
        red_polygons   => [ map $_->p, grep $_->surface_type == S_TYPE_TOP, @new_surfaces ],
        green_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_BOTTOM, @new_surfaces ],
        blue_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_BOTTOMBRIDGE, @new_surfaces ],
        yellow_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNAL, @new_surfaces ],
        cyan_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNALSOLID, @new_surfaces ],
        gray_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNALBRIDGE, @new_surfaces ],
        magenta_polygons => [ map $_->p, grep $_->surface_type == S_TYPE_INTERNALVOID, @new_surfaces ],
        no_arrows => 1,
            );
    }

}

1;

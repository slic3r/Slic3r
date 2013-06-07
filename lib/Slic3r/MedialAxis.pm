package Slic3r::MedialAxis;
use strict;

use List::Util qw(first min max);
use Boost::Geometry::Utils qw(polygon_medial_axis);
use Slic3r::Geometry qw(line_intersection);
use Slic3r::Geometry::Clipper qw(union_ex diff_ex);

my $pi = 4 * atan2(1, 1);

sub new {
    my $class = shift;
    my $polygon = $_[0]; # expolygon: [[contour], [hole1], [hole2], ...]
    my $self;
    if (ref($_[0]) =~ /(ARRAY|Polygon)/) {
        # polygon_medial_axis() should probably do it's own
        # polygon simplification that's limited to removing
        # degeneracies and near-degeneracies that Boost::Voronoi can't handle.
        # Until then, do an ExPolygon simplify here that should remove almost
        # duplicate vertices which can come from safety_offset() cycle upstream.
        $self = polygon_medial_axis($_[0]->simplify(2));
        bless $_, 'Slic3r::MedialAxis::Edge' for @{$self->{edges}};
        bless $_->vertex0, 'Slic3r::MedialAxis::Vertex' for @{$self->{edges}};
    }
    elsif (ref $_[0] eq 'HASH') {
        $self->{edges}    = $_[0]->{edges};
        $self->{vertices} = $_[0]->{vertices};
        }
    
    $self->{max_radius} = $self->{edges}->[0]->radius;

    foreach my $edge (@{$self->{edges}}) {
        if ($self->{max_radius} < $edge->radius) {
            $self->{max_radius} = $edge->radius;
        }
        
        # find the P1 control point for the quadratic Bezier curve
        # that represents the parabolic arc
        if ($edge->curved && $edge->internal) {
            my $line1 = [[@{$edge->vertex0}[0, 1]], 
                         [$edge->vertex0->[0] + 100000 * CORE::cos($edge->theta),
                          $edge->vertex0->[1] + 100000 * CORE::sin($edge->theta)]
                        ];
            my $line2 = [[@{$edge->next->vertex0}[0, 1]],
                         [$edge->next->vertex0->[0] + 100000 * CORE::cos($edge->twin->theta),
                          $edge->next->vertex0->[1] + 100000 * CORE::sin($edge->twin->theta)]
                        ];

            # If $edge->[CURVED] is false the edge is just a straight
            # line segment. line_intersection() will return undef (false) when
            # the lines are practically parallel, so in that case the 
            # curved edge will in effect be downgraded to a straight segment.

            $edge->curved(line_intersection($line1, $line2));

        }
    }
    
    #Slic3r::MedialAxis::EdgeView::edges_svg($self->{edges}) if ($DB::svg && 1||$DB::current_layer_id == 22);

    bless $self, $class;
    return $self;
}

sub offset_interval_filter {
    my $self = shift;
    my ($tool_diameter, $count, $initial_offset, $bracket, $initial_bracket, $nudge) = @_;
    
    $nudge //= 0;
    
    my @dblines;
        
    # If $initial_offset is undef, default to a half $tool_diameter.
    
    $initial_offset //= ($tool_diameter / 2);
    
    # While the different offset levels put a lower bound
    # on radius for each level, $bracket puts an upper bound on the radius.
    # $bracket will be added to each level's offset to determine
    # the upper radius bound.
    # $bracket should be useful for limiting results to just thinwall
    # or thin gap subsets of the offset edges.
    
    $bracket //= $tool_diameter + $nudge;
    $initial_bracket //= $bracket; # not used below yet - maybe don't want this

    my $max_intervals = int( ($self->{max_radius} - $initial_offset) 
                                   / ($tool_diameter)  )
                                        + 1;
    
    # If $count is undef or zero, do as many $tool_diameter 
    # offsets as will fit per edge.

    my $intervals_count = $count ? min($count, $max_intervals) : $max_intervals;

    # calculate all offset distances
    my @offset = map $_ * $tool_diameter + $initial_offset + $nudge, (0 .. $intervals_count);

    # an "interval" is a set of polyedges all at the same offset level
    # so one interval per offset level, and zero or more polyedges per interval.
    # Should rethink terms here to harmonize with Slic3r and other CNC vocabulary.
    my @intervals;
    $#intervals = $#offset;
    $_ = [] for @intervals;

    # these help keep track of polyedges typically at the start and end of
    # an interval that should really be one polyedge, so they can be merged
    my @last_loop_starts = map 0, (0 .. $intervals_count);
    my $last_loop_start = 0;
    
    # process each edge - medial axis edges should be in a sequence
    # corresponding to the order of the polygons that generated them
    # though there may be jumps from one polygon to the next, typically
    # when hole polygons are present
    for (my $i = 0; $i < @{$self->{edges}}; $i++) {
        my $e  = $self->{edges}->[$i];

        my $er1  = $e->radius;
        my $er2  = $e->next->radius;
        my $er_max = ($er1 > $er2) ? $er1 : $er2;
        my $er_min = ($er1 > $er2) ? $er2 : $er1;
        my $ephi = $e->phi;
        # TODO: fix this upstream in Boost::Geometry::Utils
        # phi should always be between 0 and +pi
        # but sometimes you get something very close to -pi, 
        # which probably should be +pi - but needs investigation
        $ephi += 2*&Slic3r::Geometry::PI if $ephi < 0;

        # TODO: enforce a minimum resolution - corners interpreted by resolution
        # or tool diameter are better modeled as circular arcs that the 
        # tool edge will touch - meaning edge radius can never go to zero
        # ... not using this yet - might just turn into "lower bracket"
        my $resolution = $tool_diameter/2;

        my $ep = $e->prev;
        my $en = $e->next;

        my $edge_max_intervals = int( ($er_max - $initial_offset) 
                                        / $tool_diameter  )
                                             + 1;

        my $edge_intervals = $count ? min($count, $edge_max_intervals) : $edge_max_intervals;

        for (my $j = 0; $j < $edge_intervals; $j++) {
            push @{$intervals[$j]}, [] if ! @{$intervals[$j]};

            # edge list order matches edge->next order, except where
            # one edge loop subset ends and the next begins
            
            my $edge_intersect = ($er1 > $offset[$j] && $er2 <= $offset[$j]) 
                               ? 1 : 0;

            my $bracket_threshold = defined($bracket)
                                  ? ($offset[$j]) + $bracket
                                  : $er_max + 1;
            
            # 
            # TODO: make phi threshold used here a parameter
            # TODO: avoid this hard threshold for sharp corners - ramp the depth
            #       of incursion into sharp corners relative to sharpness/depth

            my $shallow_corner =   $ephi < &Slic3r::Geometry::deg2rad(60)
                                || $ephi > &Slic3r::Geometry::deg2rad(120)
                               ? 1 : 0;

            # now make sure that's not a false positive by scanning the corner
            # ahead or behind to see if it has any non-shallow phi angles
            if ($shallow_corner && $ephi < &Slic3r::Geometry::deg2rad(60)) {
                my $n = $e->next;
                while ($n != $e->twin) {
                    if (   $n->phi < &Slic3r::Geometry::deg2rad(60)
                        || $n->phi > &Slic3r::Geometry::deg2rad(120)) {
                        $n = $n->next;
                        next;
                    } else { 
                        $shallow_corner = 0;
                        last;
                    }
                }
            }

            if ($shallow_corner && $ephi > &Slic3r::Geometry::deg2rad(120)) {
                my $p = $e->prev;
                while ($p != $e->twin) {
                    if (   $p->phi < &Slic3r::Geometry::deg2rad(60)
                        || $p->phi > &Slic3r::Geometry::deg2rad(120)) {
                        $p = $p->prev;
                        next;
                    } else { 
                        $shallow_corner = 0;
                        last;
                    }
                }
            }

            my $twinturn = (  ($edge_intersect
                                # both edges involved in the twinturn
                                # should have twinturn == true, not just the
                                # the first one going into the turn
                                || (   $er2 >  $offset[$j]
                                    && $er1 <= $offset[$j] )
                               )
                            && ($j != 0 
                                || 
                                $shallow_corner # checking $shallow_corner might be superfluous
                               )
                           ) ? 1 : 0;
            
            push @{$intervals[$j]}, []  if ($er1 > $bracket_threshold ) && @{$intervals[$j]->[-1]};
            push @{$intervals[$j]}, []  if ($j != 0 && $er1 && $er1 <= $offset[$j] && $er2 > $offset[$j]) && @{$intervals[$j]->[-1]};

            # decide whether to keep this edge
            if ($er_min <= $bracket_threshold
                && (!$shallow_corner || $twinturn)
               ) {

                if (0) { # DEBUG - collect keeper edges for display later
                    push @dblines, 
                         [[$e->vertex0->[0], $e->vertex0->[1]], 
                         [$e->next->vertex0->[0], $e->next->vertex0->[1]]];               
                }

                # make the view on the edge that represents the
                # current offset, and any special circumstances related to
                # thin sections or the upper radius bracket limit
                push @{$intervals[$j]->[-1]},  Slic3r::MedialAxis::EdgeView->new(
                              edge => $e,
                              offset => $offset[$j],
                              resolution => $resolution,
                              interpolate => ($er1  > $bracket_threshold || $er2  > $bracket_threshold)
                                  ? $bracket_threshold
                                  : undef,
                              intersect => $edge_intersect,
                              twinturn => $twinturn
                              );
                
                # set up next, prev, and twin references between EdgeViews
                if (@{$intervals[$j]->[-1]} > 1) {
                    $intervals[$j]->[-1]->[-2]->next($intervals[$j]->[-1]->[-1]);
                    $intervals[$j]->[-1]->[-1]->prev($intervals[$j]->[-1]->[-2]);
                }
                if (@{$intervals[$j]->[-1]} > 1 &&
                    $intervals[$j]->[-1]->[-1]->prev->isa('Slic3r::MedialAxis::EdgeView') &&
                    $twinturn && $intervals[$j]->[-1]->[-1]->prev->edge
                              == $intervals[$j]->[-1]->[-1]->edge->twin) {
                    $intervals[$j]->[-1]->[-1]->prev->twin($intervals[$j]->[-1]->[-1]);
                    $intervals[$j]->[-1]->[-1]->twin($intervals[$j]->[-1]->[-1]->prev);
                }
            }

            # handle loop end
            if ($i > $last_loop_start 
                && ($i == $#{$self->{edges}} || $en != $self->{edges}->[$i + 1])
                ) {

                # A loop just ended.
                # Does the last sequence of edges include the entire loop?
                # If not, we may have two or more subsequences of the loop
                # collected so far. The first of those subsequences may start
                # with the first edge of the loop. If so, and if the last edge 
                # we processed was a keeper, we should merge that last sequence 
                # and the sequence holding the beginning of the loop.

                # If the last edge we added wasn't cut short, and
                # if we have at least two subsets of edges
                # and the last and the first for this loop
                # each have at least one edge ...
                
                if (   @{$intervals[$j]} > 1
                    && @{$intervals[$j]->[-1]}
                    && !$intervals[$j]->[-1]->[-1]->intersect
                    && @{$intervals[$j]->[$last_loop_starts[$j]]}) {
                
                    # see if the last edge we collected abuts the first 
                    # edge in subset that comes at the start of the loop.
                    # It's not as simple as checking edge->next. We need
                    # to check all of the edges emanating out of the next
                    # vertex. So that's what the following does.
                
                    my $match = 0;
                    my $oe = $intervals[$j]->[-1]->[-1]->edge->twin->prev->twin;
                    while(!($match = $oe == $intervals[$j]->[$last_loop_starts[$j]]->[0]->edge)
                          && $oe != $intervals[$j]->[-1]->[-1]->edge->twin
                         ) {
                        $oe = $oe->prev->twin;
                    }
                
                    # If it's a match, splice out the last subset and prepend
                    # it's edge list to the loop start subset
                
                    if ($match) {
                        my ($last_one) = splice(@{$intervals[$j]}, -1, 1);
                        unshift @{$intervals[$j]->[$last_loop_starts[$j]]},
                                @$last_one;
                        # fix prev, next, twin references
                        if ($#{$intervals[$j]->[$last_loop_starts[$j]]} > $#$last_one) {
                            if (   $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one]->edge->twin
                                == $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one + 1]->edge) {
                                $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one]->twin($intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one + 1]);
                                $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one + 1]->twin($intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one]);
                                $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one]->twinturn(1);
                                $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one + 1]->twinturn(1);
                            }
                            $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one]->next($intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one + 1]);
                            $intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one + 1]->prev($intervals[$j]->[$last_loop_starts[$j]]->[$#$last_one]);
                        }
                    }
                }
                push @{$intervals[$j]}, [];
                $last_loop_starts[$j] = $#{$intervals[$j]};
            } else {
                push @{$intervals[$j]}, []  if ($er2  > $bracket_threshold) && @{$intervals[$j]->[-1]};
                push @{$intervals[$j]}, []  if ($j != 0 && $er1 > $offset[$j] && $er2 && $er2 <= $offset[$j]) && @{$intervals[$j]->[-1]};
            }
        }
        $last_loop_start = $i if ($i > $last_loop_start && $ep != $self->{edges}->[$i - 1]);
    }

    $DB::svg->appendLines({style=>"stroke:green;stroke-width:40000;"}, @dblines) if @dblines && $DB::svg;

    foreach my $interval (@intervals) {
        @$interval  = grep @$_ > 0, @$interval;
    }

    # eliminate some troublesome two-edge but zero-length fragments
    foreach my $interval (@intervals) {
        for (my $pi = $#$interval; $pi > -1; $pi--) {
            my $p = $interval->[$pi];
            if (@$p == 2
               && $p->[0]->twinturn 
               && $p->[1]->twinturn
               &&  abs($p->[0]->start_point->[0] - $p->[1]->end_point->[0]) < 2
               &&  abs($p->[0]->start_point->[1] - $p->[1]->end_point->[1]) < 2
               ) {
               $DB::svg->appendPolylines({style=>"stroke:yellow;stroke-width:40000;stroke-linecap:round;"}, [map $_->points, @$p]) if $DB::svg;
               splice @$interval, $pi, 1;
            }
        }
    }
    
    # Merge contiguous sequences.
    # Some sequences of edges will be in two fragments - the last part of the 
    # sequence will come first, and later the first part of the sequence is
    # collected. So merge those subsequences where the later one's last
    # edge has an edge->next reference to the earlier one's first edge.
    foreach my $interval (@intervals) {
        for (my $i = $#{$interval}; $i > -1; $i--) {
            for (my $j = 0; $j < $i; $j++) {
                next if $i == $j;
                if (@{$interval->[$i]} && @{$interval->[$j]} ) {
                    if (   ($interval->[$i]->[-1]->edge->next       == $interval->[$j]->[0]->edge
                         || $interval->[$i]->[-1]->edge->twin->prev == $interval->[$j]->[0]->edge->twin
                         || $interval->[$i]->[-1]->edge->twin       == $interval->[$j]->[0]->edge->twin->prev
                           )
                        && $interval->[$i]->[-1]->intersect
                       ) {
                        my $ind = $#{$interval->[$i]};
                        unshift @{$interval->[$j]}, @{ splice @{$interval}, $i, 1 };
                        $interval->[$j]->[$ind]->next($interval->[$j]->[$ind + 1]);
                        $interval->[$j]->[$ind + 1]->next($interval->[$j]->[$ind]);
                        last;
                    }
                    elsif ((  $interval->[$j]->[-1]->edge->next       == $interval->[$i]->[0]->edge
                           || $interval->[$j]->[-1]->edge->twin->prev == $interval->[$i]->[0]->edge->twin
                           || $interval->[$j]->[-1]->edge->twin       == $interval->[$i]->[0]->edge->twin->prev
                           )
                          && $interval->[$j]->[-1]->intersect
                        ) {
                        my $ind = $#{$interval->[$j]};
                        push @{$interval->[$j]}, @{ splice @{$interval}, $i, 1 };
                        $interval->[$j]->[$ind]->next($interval->[$j]->[$ind + 1]);
                        $interval->[$j]->[$ind + 1]->next($interval->[$j]->[$ind]);
                        last;
                    }                
                }
            }
        }
    }

    # fix up prev & next refs
    foreach my $interval (@intervals) {
        foreach my $polyedge (@$interval) {
            for (my $i = 0; $i < $#$polyedge; $i++) {
                if ($polyedge->[$i + 1] != $polyedge->[$i]->next) {
                    $polyedge->[$i]->next($polyedge->[$i + 1]);
                }
                if ($polyedge->[$i + 1]->prev != $polyedge->[$i]) {
                    $polyedge->[$i + 1]->prev($polyedge->[$i]);
                }
            }
        }
    }

    if (0) {
        foreach my $interval (@intervals) {
            $DB::svg->appendPolylines(
                {style=>"stroke:red;stroke-width:20000;fill:none;"},
                map {[map {$_->points} @$_]} @$interval
            ) if $DB::svg;
            $DB::svg->appendPoints(
                {r=>200000, style=>"fill:red;"},
                map {map {[@{$_}[0,1]]} ($_->[-1]->points)} @$interval
            ) if $DB::svg;
            $DB::svg->appendPoints(
                {r=>200000, style=>"fill:green;"},
                map {map {[@{$_}[0,1]]} ($_->[0]->points)} @$interval
            ) if $DB::svg;
        }
    }
    
    foreach my $interval (@intervals) {
        @$interval = map Slic3r::MedialAxis::EdgeCollection->new($_), @$interval;
    }

    return @intervals;
}

# get_offset_fragments()
# Convert a set of lists of MedialAxis::EdgeViews
# (each set typically having the same offset value set for all the 
# EdgeViews in it) to a set of lists of points representing Polygons 
# or Polylines. Makes a distinction between "left" and "right"
# about the medial axis, and usually only generates point sequences
# from one side or the other, as requested.

sub get_offset_fragments {
    my ($intervals, $keep_marked, $width) = @_;
    my @interval_thin_paths;

    for (my $i = 0; $i < @$intervals; $i++) {

        my @new_outer_frags = (Slic3r::MedialAxis::EdgeCollection->new);

        foreach my $polyedge (@{$intervals->[$i]}) {
            $_->visited(0) for @$polyedge;
        }

        @{$intervals->[$i]} = grep @$_ > 1, @{$intervals->[$i]};

        # pre-process any loops
        # this should handle cases where the part is so thin that it's 
        # completely thinwall, so that what we're dealing with is
        # a medial axis edge loop instead of a clipped fragment edge sequence
        # It looks like all we need to do is re-wind the list of edges so that
        # the one corresponding to the polygon's widest point comes first.
        # That's enough for later code to do the right thing without needing to
        # know if it's a loop or fragment.

        for (my $j = $#{$intervals->[$i]}; $j > -1; $j--) {

            my $polyedge = $intervals->[$i]->[$j];
            next if $polyedge->[-1]->edge->next != $polyedge->[0]->edge;
            my $max_r_index = 0;
            for (1 .. $#$polyedge) {
                $max_r_index = $_ if $polyedge->[$max_r_index]->radius <= $polyedge->[$_]->radius;
            }
            if ($max_r_index > 0) {
                $polyedge->[$max_r_index]->prev($polyedge->[$max_r_index - 1]);
                $polyedge->[$max_r_index - 1]->next($polyedge->[$max_r_index]);
                $polyedge->[$#$polyedge]->next($polyedge->[0]);
                $polyedge->[0]->prev($polyedge->[$#$polyedge]);
                @$polyedge = (@{$polyedge}[$max_r_index .. $#$polyedge], 
                              @{$polyedge}[0 .. $max_r_index - 1]
                             );
            }
        }

        foreach my $polyedge (@{$intervals->[$i]}) {

            #Slic3r::MedialAxis::EdgeView::edges_svg($polyedge) if ($DB::svg && $DB::current_layer_id >= 20 && $DB::current_layer_id <= 24);

            my @polyedge = @$polyedge;

            #$DB::svg->appendPolylines({style=>"stroke:lightblue;opacity:1;stroke-width:40000;fill:none;"},  [map {$_->points} @polyedge]) if (@polyedge) && $DB::svg;
            #Slic3r::MedialAxis::EdgeView::edges_svg(\@polyedge) if ($DB::current_layer_id == 46);

            # Initial pass sets up a rough left-right edge distinction
            # for thin wall sections, that we will then refine.
            
            if (@polyedge 
                && first {!$_->visited} @polyedge
                ) {
                foreach my $edge (@polyedge) {
                    if ($edge->visited != 2) {
                        $edge->twin->visited(2);
                        $edge->visited(1);
                    }
                }
            }
            
            # For thin wall sections, calls to points() for edges that have
            # a radius smaller than the thin wall threshold will generally
            # return no points, so we can discard those edges, reducing the
            # left-right distinction task to dealing with just one level of
            # branching.

            #$DB::svg->appendRaw('<g>') if $DB::svg;            
            #$DB::svg->appendPolylines({style=>"stroke:pink;opacity:1;stroke-width:20000;fill:none;"},  [map {$_->points} grep {$_->visited != 2 || $_->visited != 1} @polyedge]) if (grep {$_->visited != 2 || $_->visited != 1} @polyedge) && $DB::svg;
            #$DB::svg->appendPolylines({style=>"stroke:yellow;opacity:1;stroke-width:20000;fill:none;"},  [map {$_->points} grep {$_->visited==2} @polyedge]) if (grep {$_->visited==2} @polyedge) && $DB::svg;
            #$DB::svg->appendPolylines({style=>"stroke:pink;opacity:1;stroke-width:20000;fill:none;"},    [map {$_->points} grep {$_->visited==1} @polyedge]) if (grep {$_->visited==1} @polyedge) && $DB::svg;
            #$DB::svg->appendPoints({style=>"",r=>20000},  map {[@{$_->start_point},$_->visited == 1 ? 'blue':'red']} @polyedge) if $DB::svg;
            #$DB::svg->appendRaw('</g>');            


            # maybe... if twin has same color, but is on a different edge sequence, it should really have a different color
            # might happen with hole loops up against outer loops 
            for (my $i = 0; $i < $#polyedge; $i++) {
               if (
                      $polyedge[$i]->twin->visited == $polyedge[$i]->visited
                   # && ? if twin not in this polyedge
                   ) {
                   print "left-right distinction probably foiled by hole\n";
                   # yes this did happen sometimes
                   # ... but not seeing it now, after other improvements ...
                   # should it be fixed here or somehow avoided before this point?
                   #$polyedge[$i]->twin->visited($polyedge[$i]->visited == 1 ? 2 : 1);                            
               }
            }

            # these two for()s do one level of branch visited flag fixup
            # and one level should be all we need when handling just the 
            # edges with points when radius < offset

            for (my $i = 1; $i < $#polyedge; $i++) {
                if (
                       $polyedge[$i - 1]->visited == 2
                    && $polyedge[$i]->visited     == 1
                    && $polyedge[$i + 1]->visited == 2
                    ) {
                    $polyedge[$i]->visited(2);                            
                }
            }
            for (my $i = 1; $i < $#polyedge; $i++) {
                if (
                       $polyedge[$i - 1]->visited == 1
                    && $polyedge[$i]->visited     == 2
                    && $polyedge[$i + 1]->visited == 1
                    ) {
                    $polyedge[$i]->visited(1);                            
                }
            }                        

            # For loops, do the same branch fixup across ends.
            if ($polyedge[-1]->edge->next == $polyedge[0]->edge) {
                if (
                       $polyedge[-2]->visited == 2
                    && $polyedge[-1]->visited == 1
                    && $polyedge[0]->visited  == 2
                    ) {
                    $polyedge[-1]->visited(2);                            
                }
                if (
                       $polyedge[-1]->visited == 2
                    && $polyedge[0]->visited  == 1
                    && $polyedge[1]->visited  == 2
                    ) {
                    $polyedge[0]->visited(2);                            
                }
                if (
                       $polyedge[-2]->visited == 1
                    && $polyedge[-1]->visited == 2
                    && $polyedge[0]->visited  == 1
                    ) {
                    $polyedge[-1]->visited(1);                            
                }
                if (
                       $polyedge[-1]->visited == 1
                    && $polyedge[0]->visited  == 2
                    && $polyedge[1]->visited  == 1
                    ) {
                    $polyedge[0]->visited(1);
                }
            }

            #$DB::svg->appendRaw('<g>');
            #$DB::svg->appendPolylines({style=>"stroke:white;opacity:0.5;stroke-width:20000;fill:none;"},  [map {$_->points} grep {$_->visited == 2} @polyedge]) if (grep {$_->visited == 2} @polyedge) && $DB::svg;
            #$DB::svg->appendPolylines({style=>"stroke:purple;opacity:0.5;stroke-width:20000;fill:none;"}, [map {$_->points} grep {$_->visited == 1} @polyedge]) if (grep {$_->visited == 1} @polyedge) && $DB::svg;
            #$DB::svg->appendPoints({style=>"",r=>15000},  map {[@{$_->start_point},$_->visited == 1 ? 'green':'yellow']} @polyedge) if $DB::svg;
            #$DB::svg->appendRaw('</g>') if $DB::svg;
            
            # add the left or right edge to our collection
            push(@new_outer_frags, Slic3r::MedialAxis::EdgeCollection->new) if @{$new_outer_frags[-1]};
            foreach my $edge (@polyedge) {
                if (!$keep_marked) {
                    push(@{$new_outer_frags[-1]}, $edge) if $edge->visited == 1;
                    push(@new_outer_frags, Slic3r::MedialAxis::EdgeCollection->new)
                        if ($edge->visited == 2 && @{$new_outer_frags[-1]});
                } else {
                    push(@{$new_outer_frags[-1]}, $edge) if $edge->visited == 2;
                    push(@new_outer_frags, Slic3r::MedialAxis::EdgeCollection->new)
                        if ($edge->visited == 1 && @{$new_outer_frags[-1]});
                }
            }
        }

        @new_outer_frags = grep @$_, @new_outer_frags;

        # $DB::svg->appendRaw("<g>");
        # $DB::svg->appendPolylines({markerstart => 'url(#dirstart)', style=>'opacity:0.4;stroke-width:'.($width).';stroke:red;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, map {[$_->points]} grep @$_, @new_outer_frags) if $DB::svg;
        # $DB::svg->appendRaw("</g>");

        combine_polyedge_fragments(\@new_outer_frags);
        
        # $DB::svg->appendRaw("<g>");
        # $DB::svg->appendPolylines({markerstart => 'url(#dirstart)', style=>'opacity:0.4;stroke-width:'.($width).';stroke:blue;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, map {[$_->points]} grep @$_, @new_outer_frags) if $DB::svg;
        # $DB::svg->appendRaw("</g>");

        my $thin_paths = [];

        ################################################################################
        # junction trim
        # There may be some medial axis fragment ends that overlap the middle of
        # other fragements. Detect this and trim back those ends.

        my $junc_trim_width = $width * 0.7; # less than 1 for a little overlap
                                            # where the end of one thinwall butts
                                            # against the side of another thinwall

        # Mark all vertices but the ones at the ends of each fragment.
        # Where a vertex occurs at the end of one fragment but also in the
        # middle of another, it will end up marked, and that's what tells us
        # that that fragment end needs to be trimmed back.
        
        foreach my $frag (grep @$_ > 2, @new_outer_frags) {
            #$DB::svg->appendPolylines({markerstart => 'url(#dirstart)', style=>'opacity:1;stroke-width:'.($width*0.35).';stroke:purple;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, [$frag->points]) if $DB::svg;
            
            # Set both vertex0 and vertex1 visited -
            # even though setting vertex1 is usually redundant -
            # because the sequence of EdgeViews might sometimes skip over some 
            # Edges in the original sequence, making vertex1 not the same as 
            # the next EdgeView's vertex0.

            my $first_already = $frag->[0]->vertex0->visited;
            my $last_already = $frag->[-1]->vertex1->visited;

            $frag->[0]->vertex1->visited(1);
            for (1 .. $#$frag - 1) {
                $frag->[$_]->vertex0->visited(1) ;
                $frag->[$_]->vertex1->visited(1) ;
            }

            $frag->[-1]->vertex0->visited(1);

            # Rarely, the end vertex is shared with the second-to-last edge,
            # so it gets marked when it shouldn't. Restore visited value of 0
            # if that's what happened.
            
            $frag->[0]->vertex0->visited(0)  if !$first_already && $frag->[0]->vertex0  == $frag->[1]->vertex0;
            $frag->[-1]->vertex1->visited(0) if !$last_already  && $frag->[-1]->vertex1 == $frag->[-2]->vertex1;

        }
        foreach my $frag (grep @$_, @new_outer_frags) {

            # Ends of complete loops should survive - often loops
            # won't be involved in any clipping or merging, and can
            # later on be detected by the overlapping start and end,
            # and be treated as normal Polygons.
            my $is_loop = $frag->[-1]->edge->next == $frag->[0]->edge;

            my $trim_front = $frag->[0]->vertex0->visited && !$is_loop;
            my $trim_back = $frag->[-1]->vertex1->visited && !$is_loop;

            # convert EdgeCollections to Polylines
            # TODO: toward dynamic flow: the points should include original
            #       underlying edge radius as a third coordinate ( or a modified
            #       radius value that considers an EdgeView offset). But at the 
            #       moment, the various points() functions only sometimes
            #       include that, and then some downstream code can't handle
            #       three-coordinate Slic3r::Points. (A 2D Circle class might
            #       be good to convey the path points with radius info that we
            #       need to modify flow continuously along the path.)

            push @$thin_paths, Slic3r::Polyline->new(map [$_->[0], $_->[1]], $frag->points);
            
            #$DB::svg->appendRaw("<g>");
            #$DB::svg->appendPolylines({markerstart => 'url(#dirstar)', style=>'opacity:0.4;stroke-width:'.($width).';stroke:aqua;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, $thin_paths->[-1]) if $DB::svg;
            #$DB::svg->appendPoints({r=>11000*15, style=>'opacity:1;fill:aqua;'}, (map [$_->vertex0->[0],$_->vertex0->[1],($_->vertex0->visited?'red':'green')], @{$frag}),
            #                                               [$frag->[-1]->vertex1->[0],$frag->[-1]->vertex1->[1],($frag->[-1]->vertex1->visited?'red':'green')]
            #) if $DB::svg;
            
            if ($trim_front && @{$thin_paths->[-1]} > 1) {
                my $circle = [@{$frag->[0]->vertex0->point}, $junc_trim_width];
                my $count = @{$thin_paths->[-1]};
                $thin_paths->[-1]->clip_end_with_circle(1, $circle);                
                #$DB::svg->appendCircles({style=>'stroke-width:'.($junc_trim_width/17).';stroke:'.($frag->[0]->vertex0->visited?'yellow':'orange').';fill:none;'}, $circle, [@{$circle}[0,1],$circle->[2]/17]) if $DB::svg;
                my $f = $frag->[0]->vertex0->visited ? $frag->[0] : $frag->[0]->edge->prev;
                for (0 .. ($count - @{$thin_paths->[-1]}) - 0) {$f->vertex0->visited(0);$f->vertex1->visited(0);$f=$f->next}
                if ($frag->[0]->vertex0->visited) {
                    $frag->[0]->vertex0->visited(0);
                } else {
                   $frag->[0]->edge->prev->vertex0->visited(0);
                }
            }
            if ($trim_back && @{$thin_paths->[-1]} > 1) {
                my $circle = [@{$frag->[-1]->vertex1->point}, $junc_trim_width];
                my $count = @{$thin_paths->[-1]};
                $thin_paths->[-1]->clip_end_with_circle(0, $circle);
                #$DB::svg->appendCircles({style=>'stroke-width:'.($junc_trim_width/17).';stroke:'.($frag->[-1]->vertex1->visited?'blue':'aqua').';fill:none;'}, $circle, [@{$circle}[0,1],$circle->[2]/17]) if $DB::svg;
                my $f = $frag->[-1]->vertex1->visited ? $frag->[-1] : $frag->[-1]->edge->next;
                for (0 .. ($count - @{$thin_paths->[-1]}) - 0) {$f->vertex1->visited(0);$f->vertex0->visited(0);$f=$f->prev}
                if ($frag->[-1]->vertex1->visited) {
                    $frag->[-1]->vertex1->visited(0);
                } else {
                   $frag->[-1]->edge->next->vertex1->visited(0);
                }
            }
            #$DB::svg->appendPolylines({markerstart => 'url(#dirstar)', style=>'opacity:0.8;stroke-width:'.($width).';stroke:aqua;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, $thin_paths->[-1]) if $DB::svg;
            #$DB::svg->appendRaw("</g>");

        }
        
        #reset visited to zero for vertices
        foreach my $frag (grep @$_ > 2, @new_outer_frags) {
            for (0 .. $#$frag) {
                $frag->[$_]->vertex0->visited(0) ;
                $frag->[$_]->vertex1->visited(0) ;
            }
        }
        #
        ################################################################################

        # cull any really short results
        @$thin_paths = grep {   @$_ >  2
            || (@$_ == 2 && &Slic3r::Geometry::distance_between_points($_->[0], $_->[1]) > &Slic3r::SCALED_RESOLUTION)
            } @$thin_paths;

        push @interval_thin_paths, $thin_paths;
    }

    # reset visited values to zero for edges
    for (my $i = 0; $i < @$intervals; $i++) {
        foreach my $polyedge (@{$intervals->[$i]}) {
            $_->visited(0) for @$polyedge;
        }
    }

    return \@interval_thin_paths;
}


sub combine_polyedge_fragments {
    my ($frags, $match_dist) = @_;
    
    for (my $i = $#$frags; $i > -1; $i--) {
        my $this = $frags->[$i];
        for (my $j = 0; $j < $i; $j++) {
            my $other = $frags->[$j];
            if      ($this->[0]->edge->prev == $other->[0]->edge->twin
                     ||
                     $this->[0]->prev->edge == $other->[0]->edge->twin
                     ||
                     $this->[0]->prev->edge == $other->[0]->twin->edge
                    ) {
                my @new_this =  map {$_->twin} reverse @{splice @$frags, $i, 1};
                $other->[0]->prev($new_this[-1]);
                $new_this[-1]->next($other->[0]);
                unshift @{$other}, @new_this;
                last;
            } elsif ($this->[-1]->edge == $other->[-1]->edge->next->twin
                     ||
                     $this->[-1]->edge == $other->[-1]->next->edge->twin
                     ||
                     $this->[-1]->edge == $other->[-1]->next->twin->edge
                    ) {
                my @new_this =  map {$_->twin} reverse @{splice @$frags, $i, 1};
                $other->[-1]->next($new_this[0]);
                $new_this[0]->prev($other->[-1]);
                push @{$other}, @new_this;
                last;
            } elsif ($this->[-1]->edge->next == $other->[0]->edge
                     ||
                     $this->[-1]->next->edge == $other->[0]->edge
                    ) {
                my @new_this = @{splice @$frags, $i, 1};
                $other->[0]->prev($new_this[-1]);
                $new_this[-1]->next($other->[0]);
                unshift @{$other}, @new_this;
                last;
            } elsif ($this->[0]->edge->prev == $other->[-1]->edge
                     ||
                     $this->[0]->prev->edge == $other->[-1]->edge
                    ) {
                my @new_this = @{splice @$frags, $i, 1};
                $other->[-1]->next($new_this[0]);
                $new_this[0]->prev($other->[-1]);
                push @{$other}, @new_this;
                last;
            }
        }
    }
}


sub merge_expolygon_and_medial_axis_fragments {
    my ($expolygon, $offset_expolygons, $medial_axis_fragments, $distance, $direction) = @_;

    my @ma = @$medial_axis_fragments;
    my @all_expolygons = @$offset_expolygons;
    my @loops_and_paths;

    return $offset_expolygons if !@ma;

    # Get expolygon that is the original minus the offset polygon(s).
    # This is the zone where thin walls and points live.
    my $diff = Slic3r::Geometry::Clipper::diff_ex(
        $expolygon,
        [map @$_, @all_expolygons]
        );
    #$DB::svg->appendPolygons({style=>'opacity:0.8;stroke-width:'.(0*$distance/12).';stroke:pink;fill:pink;stroke-linecap:round;stroke-linejoin:round;'}, @$diff) if $DB::svg;

    my @ma_as_polylines = map Slic3r::Polyline->new($_), 
        # Filter the point coords to just have x and y - because somewhere 
        # downstream extra coords (probably radius here) cause far-out
        # false points to be inserted between each legit point.
        # (But eventually we will want to let any radius coords through here
        # to do dynamic flow.)
                          map [map [@{$_}[0,1]], @$_],
                          grep @$_ > 1, @ma;
    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/3).';stroke:brown;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @ma_as_polylines) if $DB::svg;

    return $offset_expolygons if !@ma_as_polylines;
    
    # Get the medial axis fragments that are in the thin wall zone.
    my @boost_trimmed;
    if (@{$diff->[0]} > 1) {
        @boost_trimmed = map @{Boost::Geometry::Utils::polygon_multi_linestring_intersection
                              ($_, [@ma_as_polylines])
                             }, @$diff;
    } else {
        # no hole in the thin wall zone, so don't need to clip
        @boost_trimmed = @ma_as_polylines;
    }

    bless $_, 'Slic3r::Polyline' for @boost_trimmed;
    bless $_, 'Slic3r::Point' for map @$_, @boost_trimmed;
    
    #$DB::svg->appendPolylines({markerstart => 'url(#dirstart)', style=>'opacity:0.8;stroke-width:'.($distance).';stroke:aqua;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @boost_trimmed) if $DB::svg;

    # Break ExPolygons into fragments where they intersect with the medial axis.

    my @offset_clipped = map $_->clip_with_polylines(\@ma_as_polylines), map $_->contour, @all_expolygons;
    my @offset_polygons = grep $_->isa('Slic3r::Polygon'), (@offset_clipped);
    my @offset_polylines = grep !$_->isa('Slic3r::Polygon'), (@offset_clipped);
    my @offset_holes_clipped = map $_->clip_with_polylines(\@ma_as_polylines), map $_->holes, @all_expolygons;
    my @offset_holes_polygons = grep $_->isa('Slic3r::Polygon'), (@offset_holes_clipped);
    my @offset_holes_polylines = grep !$_->isa('Slic3r::Polygon'), (@offset_holes_clipped);

    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/5).';stroke:pink;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @offset_polylines) if $DB::svg;

    # Trim one end of each polygon fragment for tool radius.
    # This also has the effect of determining which way a medial axis 
    # fragment will "turn" as it meets the original polygon boundary.

    foreach my $offset_polyline (@offset_polylines, @offset_holes_polylines) {
        # Alternate "turn" direction each layer
        #my $clip_front = $self->layer->id % 2;
        my $clip_front = $direction;
        my $circle = $clip_front > 0 
                 ? [$offset_polyline->[0]->[0],  $offset_polyline->[0]->[1],  $distance]
                 : [$offset_polyline->[-1]->[0], $offset_polyline->[-1]->[1], $distance];
        $offset_polyline->clip_end_with_circle($clip_front, $circle);
        # This is to get any medial axis fragments to always be added to 
        # the _end_ of a polyline derived from a Clipper offset loop, so we're
        # always _going out_ into a thin point, not starting at the tip of a
        # thin point. This helps a bit with thin points that are also overhangs,
        # so the extrusion doesn't start where there's little or no support.
        # Where this reverse happens, the path replacing the original contour
        # will then be clockwise, which could be a problem downstream.
        @{$offset_polyline} = reverse @{$offset_polyline} if !$clip_front;
    }
    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/6).';stroke:purple;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @offset_polylines) if $DB::svg;

    # Splice together medial axis fragments
    # and polygon fragments into continuous paths.

    my @all_frags = grep @$_ > 1, ( @offset_polylines, @offset_holes_polylines, @boost_trimmed);
    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/6).';stroke:blue;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @all_frags) if $DB::svg;

    Slic3r::Geometry::combine_polyline_fragments(\@all_frags, $distance/20);
    my @ex_frags = map Slic3r::ExPolygon->new($_), @all_frags;
    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/8).';stroke:red;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, map $_->contour, @ex_frags) if $DB::svg;

    # Hack Polylines into ExPolygons
    # unless the medial axis derived fragment really is a complete loop
    foreach (@ex_frags) {
        if (Slic3r::Geometry::points_coincide($_->[0]->[-1], $_->[0]->[0])) {
            pop @{$_->[0]};
        } else { bless($_->[0], 'Slic3r::Polyline'); }
            
    }
    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/8).';stroke:red;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, map $_->contour, @ex_frags) if $DB::svg;

    # Preserve polygons and holes that didn't get split up by the medial axis.
    push @ex_frags, @{union_ex([@offset_polygons, @offset_holes_polygons])};

    return \@ex_frags;
}


# EdgeCollection

# Collection of Edges or EdgeViews
# Edge sequences that are subsets of the complete edge collection of the 
# half-edge graph, or sequences that omit some contiguous edges of the orignal 
# graph, will likely need special handling when returning lists of points.
# We don't want to clutter Edge or EdgeView objects with logic regarding their 
# position in arbitrary sequences. We'll put that sort of logic here.

package Slic3r::MedialAxis::EdgeCollection;

use strict;

sub new {
    my $class = shift;
    my $self = $_[0] // [];
    bless $self, $class;
    return $self;
}

sub start_point {
    my $self = shift;
    return @{$self} ? $self->[0]->start_point(@_) : (); 
}

sub end_point   {
    my $self = shift;
    return @{$self} ? $self->[-1]->end_point(@_)  : ();
}
    
sub mid_points {
    return () if !@{$_[0]};
    my ($self, $resolution, $tool_diameter) = @_;
    return grep $_, ($self->[0]->mid_points($resolution, $tool_diameter),
            (map {($_->start_point($resolution, $tool_diameter), 
                   $_->mid_points($resolution, $tool_diameter)
                  )} @{$self}[1..$#$self])
           );
}

sub points {
    my ($self, $resolution, $tool_diameter) = @_;
    return grep $_, ($self->start_point($resolution, $tool_diameter),
                     $self->mid_points($resolution, $tool_diameter), 
                     $self->end_point($resolution, $tool_diameter)
                    );
}

# Vertex
# A bit nicer interface to the half-edge graph vertices
package Slic3r::MedialAxis::Vertex;
use strict;

sub new {
    my $class = shift;
    my $self = $_[0];
    bless $self, $class;
    return $self;
}
sub point   { [$_[0]->[0], $_[0]->[1]]; }
sub radius  { $_[0]->[2]; }
sub circle  { [$_[0]->[0], $_[0]->[1], $_[0]->[2]]; }
sub edge    { $_[0]->[3]; }
sub visited { $_[0]->[4] = $_[1] if @_ > 1; $_[0]->[4]; }

# Edge
# A bit nicer interface to the half-edge graph edges
package Slic3r::MedialAxis::Edge;
use strict;

use constant SOURCE_INDEX => 0;
use constant VERTEX0  =>  1;
use constant TWIN     =>  2;
use constant NEXT     =>  3;
use constant PREV     =>  4;
use constant THETA    =>  5;
use constant PHI      =>  6;
use constant CURVED   =>  7;
use constant PRIMARY  =>  8;
use constant INTERNAL =>  9;
use constant FOOT     => 10;
use constant VISITED  => 11;
use constant X => 0;
use constant Y => 1;
use constant RADIUS => 2;
use constant VEDGE => 3;
use constant VVISITED => 4;

sub new {
    my $class = shift;
    my $self = $_[0];
    bless $self, $class;
    return $self;
}

# generate accessors for the edge data fields
eval('sub '.lc($_).' {return $_[0]->['.$_.'];} ') 
   for qw(SOURCE_INDEX VERTEX0 TWIN NEXT PREV THETA PHI PRIMARY INTERNAL FOOT);
sub vertex1 { $_[0]->next->vertex0 }
sub point0  { $_[0]->foot }
sub point1  { $_[0]->next->foot }
sub radius  { $_[0]->vertex0->[RADIUS] }
sub curved  { $_[0]->[CURVED] = $_[1] if @_ > 1; $_[0]->[CURVED] }
sub visited { $_[0]->[VISITED] = $_[1] if @_ > 1; $_[0]->[VISITED] }
sub edge    { $_[0] }


sub start_point { $_[0]->point0 }
sub end_point { $_[0]->next->point0 }
sub mid_points { return () } # TODO - curve samples, or seg samples, to some resolution

sub rot_next { $_[0]->twin->next }
sub rot_prev { $_[0]->prev->twin }


# EdgeView
# Extends the medial axis half-edge graph Edge objects to enable 
# offset edges or polygons and radius filters.

package Slic3r::MedialAxis::EdgeView;
use strict;

use constant EDGE        => 0;
use constant OFFSET      => 1;
use constant RESOLUTION  => 2;
use constant INTERPOLATE => 3;
use constant INTERSECT   => 4;
use constant TWINTURN    => 5;
use constant EVTWIN      => 6;
use constant EVNEXT      => 7;
use constant EVPREV      => 8;

# generate accessors for most of the underlying edge data fields
eval('sub '.lc($_).' {return $_[0]->[0]->[&Slic3r::MedialAxis::Edge::'.$_.'];}')
    for qw(SOURCE_INDEX VERTEX0 THETA PHI CURVED PRIMARY INTERNAL FOOT);

sub new {
    my $class = shift;
    my %args = @_;
    my $self = [
        map delete $args{$_}, qw(edge offset resolution interpolate intersect twinturn twin next prev)
    ];
    bless $self, $class;
    return $self;
}

sub edge        { return $_[0]->[EDGE]; }
sub offset      { return $_[0]->[OFFSET]; }
sub resolution  { return $_[0]->[RESOLUTION]; }
sub intersect   { return $_[0]->[INTERSECT]; }
sub interpolate { return $_[0]->[INTERPOLATE]; }
sub radius      { return $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::VERTEX0]->[&Slic3r::MedialAxis::Edge::RADIUS]; }
sub visited     { my $self = shift; return $self->edge->visited(@_); }
sub twinturn    { $_[0]->[TWINTURN] = $_[1] if @_ > 1; return $_[0]->[TWINTURN]; }

# Override next, prev, twin for Edges
# If the EdgeView doesn't already have a reference to 
# another EdgeView for any of these, upgrade the underlying next/prev/twin
# Edge to an EdgeView, and save and return that.

sub next {
    $_[0]->[EVNEXT] = $_[1] if @_ > 1;
    return $_[0]->[EVNEXT]
           ? $_[0]->[EVNEXT]
           : ($_[0]->[INTERSECT] && $_[0]->[TWINTURN] 
             ? $_[0]->[EVNEXT] = Slic3r::MedialAxis::EdgeView->new(
                   edge => $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::TWIN],
                   offset => $_[0]->offset,
                   resolution => $_[0]->resolution,
                   prev => $_[0]
               )
             : $_[0]->[EVNEXT] = Slic3r::MedialAxis::EdgeView->new(
                   edge => $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::NEXT],
                   offset => $_[0]->offset,
                   resolution => $_[0]->resolution,
                   prev => $_[0]
               )
             );
}
sub prev {
    $_[0]->[EVPREV] = $_[1] if @_ > 1;
    return $_[0]->[EVPREV]
           ? $_[0]->[EVPREV]
           : ($_[0]->[INTERSECT] && $_[0]->[TWINTURN] 
             ? $_[0]->[EVPREV] = Slic3r::MedialAxis::EdgeView->new(
                   edge => $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::TWIN],
                   offset => $_[0]->offset,
                   resolution => $_[0]->resolution,
                   next => $_[0],
               )
             : $_[0]->[EVPREV] = Slic3r::MedialAxis::EdgeView->new(
                   edge => $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::PREV],
                   offset => $_[0]->offset,
                   resolution => $_[0]->resolution,
                   next => $_[0],
               )
             );
}
sub twin {
    $_[0]->[EVTWIN] = $_[1] if @_ > 1;
    return $_[0]->[EVTWIN]
           ? $_[0]->[EVTWIN]
           : $_[0]->[EVTWIN] = Slic3r::MedialAxis::EdgeView->new(
                   edge => $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::TWIN],
                   offset => $_[0]->offset,
                   resolution => $_[0]->resolution,
                   twin => $_[0],
             );
}

# point0, point_i and point1 return points on, or offset from, 
# the original polygon.
# point0 is the location of the edge's foot on the polygon.
# point1 is the location of the next edge's foot on the polygon.
# The segment between those two points lies on one of the
# original polygon segments, or is offset and parallel to it.

sub point0 {
    my ($self, $off, $raw) = @_;

    $off //= $self->offset;
    
    my $thetaphi = $self->theta + $self->phi;
    
    if (   !$raw
        && $self->offset >= $self->radius
        && $self->offset >= $self->next->radius) {        
        return [$self->vertex0->[0], $self->vertex0->[1]];
    }

    # Normally positive offset is from the polygon inward.
    # For now, if it's negative we'll offset from the medial axis edge outward.
    # But this convention might change with the evolution of the Edge and 
    # EdgeView models.
    my $radius =  defined($off) && $off > 0 ? $self->radius - $off : -$off;
    
    my $pointx = $radius * cos($thetaphi) + $self->vertex0->[0];
    my $pointy = $radius * sin($thetaphi) + $self->vertex0->[1];
    
    # watch for any cases of offset distance coming out with too much error
    if (0) {
        my $too_much_error; # e.g.: to watch for > 10% error, set to 0.1
        my  $calc_off = sqrt(($self->foot->[0] - $pointx)**2 + ($self->foot->[1] - $pointy)**2);
        if ($self->offset && abs($self->offset - $calc_off)/$self->offset > $too_much_error) {
            print "too much offset error: ",
                ($self->offset ? $self->offset : 'undef'),
                " vs ",$calc_off,"\n";
        }
    }
    return [$pointx, $pointy];
    }

sub point1 { 
    my ($self, $off, $raw) = @_;
    my $p;
    
    $off //= $self->offset;
    
    $p = $self->next->point0($off, $raw);
    
    if (  $self->edge->next == $self->edge->twin
       || $self->next->edge == $self->twin->edge 
       ) {
        # (note we're checking underlying Edges, not EdgeViews)
        # account for phi not being the same for consecutive edges
        # in corners, where the medial axis touches the polygon,
        # or where a similar "twinturn" happens
        $p = _reflect($p, $self->vertex0, $self->vertex1);
    }
    return $p;
    }

# point_i is an interpolated point between point0 and point1.
# $target is a target radius
# The idea is to find the point between point0 and point1 where the
# interpolated radius matches target. So this is only meant to work when
# $target is within the radius range of the edge.
# By default, this will return the interpolated point where the offset (from
# the polygon) segment intersects this medial axis edge (assuming it does).
# But other radius targets can be used for getting an interpolated point at, for
# example, some minimum or maximum radius threshold, if the edge stradles
# that threshold.

sub point_i {
    my ($self, $target) = @_;

    $target //=  $self->offset;
    
    my ($p1, $p2);
    if ($target == $self->offset) {
        $p1 = $self->vertex0;
        $p2 = $self->vertex1;
    } else {
        $p1 = $self->point0($self->offset, 1);
        $p2 = $self->point1($self->offset, 1);
    }

    my $r1 = $self->radius;
    my $r2 = $self->next->radius;
    
    my $factor = abs(($target - ($r1 < $r2 ? $r1 : $r2)) / ($r1 - $r2));
    $factor = 1 - $factor if $r1 > $r2;

    return [$p1->[0] + $factor * ($p2->[0] - $p1->[0]),
            $p1->[1] + $factor * ($p2->[1] - $p1->[1]),
           ];
    }

# vertex0, and vertex1 are the edge's start and end points,
# while vertex_i is an interpolated point between them

sub vertex0 { $_[0]->edge->vertex0 }
sub vertex1 { $_[0]->edge->next->vertex0 }
sub vertex_i {
    my ($self, $factor) = @_;
    my ($p1, $p2) = ($self->vertex0, $self->vertex1);
    return [$p1->[0] + $factor * ($p2->[0] - $p1->[0]),
            $p1->[1] + $factor * ($p2->[1] - $p1->[1])
           ];
}

# start_point(), end_point() and mid_points() for an EdgeView take into account
# radius, offset, interpolate and twinturn options.
# Use these three methods to get points expressing the intended 
# "view" of an EdgeView.

sub start_point {
    my $self = shift;
    my $ret;
    if (defined($self->interpolate) && $self->interpolate < $self->radius) {
        $ret = $self->point_i($self->interpolate);
    } elsif ($self->radius < $self->offset && $self->next->radius > $self->offset) {
        # the medial axis edge-intersecting point
        if ($self->twinturn) { $ret = $self->point_i; }
        else { $ret = $self->vertex0; }
    } elsif ($self->radius < $self->offset && $self->next->radius < $self->offset) {
        $ret = $self->vertex0;
    } else {
        $ret = $self->point0;
    }
    return $ret;
}

sub end_point {
    my $self = shift;
    my $ret;
    if (defined($self->interpolate) && $self->interpolate > $self->radius) {
        $ret = $self->point_i($self->interpolate);
    } elsif ($self->radius > $self->offset && $self->next->radius < $self->offset) {
        # the medial axis edge-intersecting point
        if ($self->twinturn) { $ret= $self->point_i;}
        else { $ret = $self->vertex1; }
    } elsif ($self->radius < $self->offset && $self->next->radius < $self->offset) {
        $ret = $self->vertex1;
    } else {
        $ret = $self->point1;
    }
    return $ret;
}

sub mid_points {
    my ($self, $resolution, $tool_diameter) = @_;
    if ( 
        !$self->twinturn &&
          (  ($self->radius > $self->offset && $self->next->radius < $self->offset)
          || ($self->radius < $self->offset && $self->next->radius > $self->offset)
          )
        ) {
        # the medial axis edge-intersecting point
        return ($self->point_i);
    } elsif (
        0 && 
        $self->curved
        ) {
        # TODO: curve samples at given resolution
        
    } else {
        return ();
    }
}

sub points {
    my $self = shift;
    
    return grep $_, (
             $self->start_point, 
             $self->mid_points, 
             $self->end_point
             );
}

# debug visual output
sub edge_svg {
    my $self = shift;
    
    return if !$DB::svg;
    
    $DB::svg->appendRaw('<g>');
    
    # the edges of the cell
    $DB::svg->appendCurves({style=>"stroke:darkgray;stroke-width:20000;opacity:0.4;fill:gray;"},
      [ [@{$self->vertex0}[0,1]], 
        ($self->curved ? [@{$self->curved}, @{$self->vertex1}[0,1]]
                       : [                  @{$self->vertex1}[0,1]]), 
        [@{$self->edge->point1}[0,1]], 
        [@{$self->edge->point0}[0,1]], 
        []
      ]
    ); 
    
    # the offset-from-polygon segment
    $DB::svg->appendPolylines({style=>"stroke:white;stroke-width:40000;fill:none;"},
      [ grep $_, ( $self->start_point, $self->mid_points, $self->end_point )]
     ) if (grep $_, ( $self->start_point, $self->mid_points, $self->end_point )) > 1; 
    
    # the start point, any midpoints, and the end point of the offset segment
    $DB::svg->appendPoints({r=>20000,style=>"opacity:0.4;"},
       map [@{$_->[0]}[0,1],$_->[1]], grep $_->[0], 
          (      [$self->start_point || undef, 'green' ], 
            (map [$_                 || undef, 'yellow'], $self->mid_points), 
                 [$self->end_point   || undef, 'red'   ] )
     );

    $DB::svg->appendRaw('</g>');

}

# debug visual output
sub edges_svg {
    my $edges = shift;

    return if !$DB::svg;
    
    my @quads = map {
          [ [@{$_->vertex0}[0,1]], 
            ($_->curved ? [@{$_->curved}, @{$_->vertex1}[0,1]]
                           : [            @{$_->vertex1}[0,1]]), 
            [@{$_->edge->point1}[0,1]], 
            [@{$_->edge->point0}[0,1]], 
            []
          ]
        } @$edges;
    my @quads_curved = map {
          [ [@{$_->vertex0}[0,1]], 
            ($_->curved ? [@{$_->curved}, @{$_->vertex1}[0,1]]
                           : [            @{$_->vertex1}[0,1]]), 
            [@{$_->edge->point1}[0,1]], 
            [@{$_->edge->point0}[0,1]], 
            []
          ]
        } grep $_->curved, @$edges;
    my @theta_rays = map {
        [ $_->vertex0, [$_->vertex0->[0] + 200000*cos($_->theta), $_->vertex0->[1] + 200000*sin($_->theta)]]
        } @$edges;
    my @phi_rays = map {
        [ $_->vertex0, [$_->vertex0->[0] + 200000*cos($_->theta + $_->phi), $_->vertex0->[1] + 200000*sin($_->theta + $_->phi)]]
        } @$edges;
    my @offset_segs_whole = map {
        [ grep $_, ( $_->point0($_->offset/6, 1), $_->point1($_->offset/6, 1) )]
        } @$edges if $edges->[0]->isa('Slic3r::MedialAxis::EdgeView');
    my @offset_segs = map {
        [ grep $_, ( $_->start_point, $_->mid_points, $_->end_point )]
        } @$edges if $edges->[0]->isa('Slic3r::MedialAxis::EdgeView');
    my @offset_pts = map {
        map [@{$_->[0]}[0,1],$_->[1]], grep $_->[0], 
          (      [$_->start_point || undef, 'green' ], 
            (map [$_              || undef, 'yellow'], $_->mid_points), 
                 [$_->end_point   || undef, 'red'   ] )
        } @$edges if $edges->[0]->isa('Slic3r::MedialAxis::EdgeView');
    $DB::svg->appendRaw('<g>');
    
    # the edges of the cell
    $DB::svg->appendCurves({style=>"stroke:darkgray;stroke-width:20000;opacity:0.4;fill:gray;"},
      @quads
    ); 
    # the curved edges of the cell
    $DB::svg->appendCurves({style=>"stroke:darkgray;stroke-width:20000;opacity:0.4;fill:green;"},
      @quads_curved
    ); 
    
    # the offset-from-polygon segment without any "interpolate" or "intersect" limits
    $DB::svg->appendPolylines({style=>"stroke:orange;opacity:0.5;stroke-width:40000;fill:none;"},
      grep @$_>1, @offset_segs_whole) if $edges->[0]->isa('Slic3r::MedialAxis::EdgeView');
    # the offset-from-polygon segment
    $DB::svg->appendPolylines({style=>"stroke:white;stroke-width:40000;fill:none;"},
      grep @$_>1, @offset_segs) if $edges->[0]->isa('Slic3r::MedialAxis::EdgeView');
    # theta rays
    $DB::svg->appendPolylines({style=>"stroke:aqua;stroke-width:20000;fill:none;",markerend => 'url(#dirend)'},
      @theta_rays);
    # phi rays
    $DB::svg->appendPolylines({style=>"stroke:purple;stroke-width:20000;fill:none;",markerend => 'url(#dirend)'},
      @phi_rays);
    # the start point, any midpoints, and the end point of the offset segment
    $DB::svg->appendPoints({r=>20000,style=>"opacity:0.4;"},
       @offset_pts) if $edges->[0]->isa('Slic3r::MedialAxis::EdgeView');

    $DB::svg->appendRaw('</g>');

}

sub _rotate_2d {
    my ($p1, $theta, $origin) = @_;
    return [(  ($p1->[0] - $origin->[0]) * CORE::cos($theta) 
             - ($p1->[1] - $origin->[1]) * CORE::sin($theta)
            ) + $origin->[0],
            (  ($p1->[1] - $origin->[1]) * CORE::cos($theta) 
             + ($p1->[0] - $origin->[0]) * CORE::sin($theta)
            ) + $origin->[1]
           ];
}

sub _reflect {
    my ($p1, $p2, $p3) = @_;
    my $dy = $p3->[1] - $p2->[1];
    my $dx = $p3->[0] - $p2->[0];
    if ($dy == 0 && $dx == 0) {
        # warn "Can't reflect about a point\n";
        return $p1;
        }
    my $theta = atan2($dy, $dx);
    $p1 = _rotate_2d($p1, -$theta, $p2);
    $p1->[1] -= $p2->[1];
    $p1->[1] *= -1.0;
    $p1->[1] += $p2->[1];
    return _rotate_2d($p1, $theta, $p2);
}


# EdgeViewSkel [UNDER DEVELOPMENT]
# Straight skeleton variation on EdgeView

# Attempt to mimic "straight skeleton" offsets, which is closer to 
# what we get from Clipper. (miter limit related details and the 
# effects of -1, +0.5 offset cycle not accounted for though)

# The extra $tool_diameter parameter below
# can hopefully be avoided and removed. But it's not clear yet whether we'll
# want or need that to do some smart clipping of offset segments that end
# up hitting the medial axis.

package Slic3r::MedialAxis::EdgeViewSkel;
use strict;
use base 'Slic3r::MedialAxis::EdgeView';
use List::Util qw(min);

sub start_point {
    my ($self, $resolution, $tool_diameter) = @_;
    if (!$self->curved
    || Slic3r::Geometry::points_coincide($self->edge->point0, $self->edge->point1)) {
        return $self->SUPER::start_point($self);
    } elsif ($self->curved && !Slic3r::Geometry::points_coincide($self->edge->point0, $self->edge->point1)) {
        if ($self->offset > min($self->edge->twin->radius, $self->edge->twin->next->radius)) {
            my $ev = Slic3r::MedialAxis::EdgeViewSkel->new(edge => $self->edge->twin, offset => $self->offset);
            return $ev->mid_points($resolution, $tool_diameter);
        }
    } else {
        return ();
    }
}

sub end_point {
    my ($self, $resolution, $tool_diameter) = @_;
    if (!$self->curved
    #|| !Slic3r::Geometry::points_coincide($self->edge->point0, $self->edge->point1)
    ) {
        return $self->SUPER::end_point($self);
    } else {
        return ();
    }
}
sub mid_points {
    my ($self, $resolution, $tool_diameter) = @_;
    if (!$self->curved &&
        !$self->twinturn &&
          (  ($self->radius > $self->offset && $self->next->radius < $self->offset)
          || ($self->radius < $self->offset && $self->next->radius > $self->offset)
          )
        ) {
        # the medial axis edge-intersecting point
        return ($self->point_i);
    } elsif (
        $self->curved
        && Slic3r::Geometry::points_coincide($self->edge->point0, $self->edge->point1)
        ) {        

        # the main straight skeleton related modifications are in here

        my @ret;
        
        my $p1 = $self->point0($self->offset,'raw');
        
        my $e2 = $self->next;
        # We want to skip over a sequence of consecutive curved edges that have
        # the same "pivot point" on the polygon.
        # There might also be straight edges in that sequence that have the
        # same point0 and point1 (same as pivot point), because the other side
        # of the MIC is tanget to a point too, and the MIC is just growing 
        # (with center tracing out a straight ma edge) as it passes through
        # the two points.
        while ($e2->next  && Slic3r::Geometry::points_coincide($e2->edge->point1, $self->edge->point1)) {
           $e2 = $e2->next;
        }

        $DB::svg->appendPoints({r=>200000,style=>'fill:purple;'}, $e2->point0($self->offset,'raw')) if $DB::svg;
        
        my $p2 = $e2->point0($self->offset,'raw');
        my $a1 = atan2(  $self->prev->point1($self->offset,'raw')->[1]
                       - $self->prev->point0($self->offset,'raw')->[1],
                         $self->prev->point1($self->offset,'raw')->[0]
                       - $self->prev->point0($self->offset,'raw')->[0]
                      );
        my $a2 = atan2(  $e2->point0($self->offset,'raw')->[1]
                       - $e2->point1($self->offset,'raw')->[1],
                         $e2->point0($self->offset,'raw')->[0]
                       - $e2->point1($self->offset,'raw')->[0]
                      );
        my $r_min =  min($e2->radius, $self->radius);
        my $r_max =  $r_min == $e2->radius ? $self->radius : $e2->radius;
        my $line1 = [[@{$p1}[0, 1]], 
                     [$p1->[0] + ($r_min) * CORE::cos($a1),
                      $p1->[1] + ($r_min) * CORE::sin($a1)]
                    ];
        my $line2 = [[@{$p2}[0, 1]],
                     [$p2->[0] + ($r_min) * CORE::cos($a2),
                      $p2->[1] + ($r_min) * CORE::sin($a2)]
                    ];

        my $intersect = Slic3r::Geometry::line_intersection($line1, $line2, 1);
        #my $intersect = (Slic3r::Geometry::_line_intersection(map @$_, @$line1, @$line2))->[0];

        my $hyp_end = undef;

        if (!$intersect) {
            my $hp1 = $self->point0($r_min, 'raw');
            my $hp2 = $e2->point0($r_min, 'raw');
            #my $hp2 = $self->twin->point1($r_min, 'raw');
            #my $hp2 = $self->curved; # worked when there was only one curved edge involved
            #$DB::svg->appendPoints({r=>400000,style=>'fill:pink;'},$self->curved) if $DB::svg;
            my $p3 = $self->twin->point1($self->offset, 'raw');
            my $a3 = atan2(  $self->twin->point0($self->offset,'raw')->[1]
                           - $self->twin->point1($self->offset,'raw')->[1],
                             $self->twin->point0($self->offset,'raw')->[0]
                           - $self->twin->point1($self->offset,'raw')->[0]
                          );
            my $hline1 = [[@{$hp1}[0, 1]], 
                          [$hp1->[0] + $r_max * CORE::cos($a1),
                           $hp1->[1] + $r_max * CORE::sin($a1)]
                         ];
            my $hline2 = [[@{$hp2}[0, 1]],
                          [$hp2->[0] + $r_max * CORE::cos($a2),
                           $hp2->[1] + $r_max * CORE::sin($a2)]
                         ];
            $line1     = [[@{$p1}[0, 1]], 
                          [$p1->[0] + $r_max * CORE::cos($a1),
                           $p1->[1] + $r_max * CORE::sin($a1)]
                         ];
            $line2     = [[@{$p3}[0, 1]],
                          [$p3->[0] + $r_max * CORE::cos($a3),
                           $p3->[1] + $r_max * CORE::sin($a3)]
                         ];
            $hyp_end = (Slic3r::Geometry::_line_intersection(map @$_, @$hline1, @$hline2));
            $hyp_end = ref($hyp_end) ? $hyp_end->[0] : $self->curved;
            #$hyp_end = $self->curved;
        }

        #$DB::svg->appendRaw('<g>') if $DB::svg;
        #$DB::svg->appendPoints({r=>200000,style=>'fill:red;'}, $intersect) if $intersect && $DB::svg;
        #$DB::svg->appendPoints({r=>400000,style=>'fill:pink;'},$self->edge->point0) if $DB::svg;
        #$DB::svg->appendLines({style=>'stroke:pink;stroke-width:100000;'}, $line1, $line2) if $DB::svg;
        #$DB::svg->appendLines({style=>'stroke:blue;stroke-width:100000;'},[[@{$self->vertex0}[0,1]], $hyp_end]) if $hyp_end && $DB::svg;
        #$DB::svg->appendPoints({r=>200000,style=>'fill:yellow;'},$hyp_intersect) if $hyp_intersect && $DB::svg;

        if ($hyp_end) {
            my $hyp_intersect_1 = Slic3r::Geometry::line_intersection($line1, [[@{$self->vertex0}[0,1]], $hyp_end], 1);
            my $hyp_intersect_2 = Slic3r::Geometry::line_intersection($line2, [[@{$self->vertex0}[0,1]], $hyp_end], 1);
            #$DB::svg->appendPoints({r=>200000,style=>'fill:blue;'},$self->vertex0, $e2->vertex0) if $DB::svg;
            #$DB::svg->appendPoints({r=>200000,style=>'fill:orange;'},grep $_, ($hyp_intersect_1, $hyp_intersect_2)) if $DB::svg;
            @ret = ($hyp_intersect_1, $hyp_intersect_2);
        }
        
        #$DB::svg->appendRaw('</g>') if $DB::svg;
        @ret = $intersect ? ($intersect) : ();
        return @ret;
    } else {
        return ();
    }
}


# consider doing circular arcs variant, where instead of parabolic arcs around
# corners, you do circular arcs, resulting in parallel offsets that have the
# same spacing between the curved lines as they do for straight.
# Arcs would need to be clipped where they intersect medial axis edges.
#package Slic3r::MedialAxis::EdgeViewArc;
#use strict;
#use base 'Slic3r::MedialAxis::EdgeView';        
# ...

1;

package Slic3r::MedialAxis;
use strict;
#use warnings;

use List::Util qw(first min max);
use Boost::Geometry::Utils qw(polygon_medial_axis);
use Slic3r::Geometry qw(line_intersection);
use Slic3r::Geometry::Clipper qw(union_ex diff_ex);

my $pi = 4*atan2(1,1);
my $medial_axis_hpp_debug_styles = '.ine1linearprimary circle {fill:green;}'.
    '.ine1linearprimary line {stroke:purple;stroke-width:15000;}'.
    '.ine1linearsecondary circle {fill:lightgreen;}'.
    '.ine1linearsecondary line {stroke:purple;stroke-dasharray:3000 3000;stroke-width:15000;}'.
    '.cirevtpt {fill:white;}'.
    '.cirevtcir {fill:none;stroke:gray;stroke-width:10000}'.
    '.edge12 {stroke:green;stroke-width:7000;stroke-dasharray:2000 2000;}'.
    '.edge23 {stroke:lightgreen;stroke-width:7000;stroke-dasharray:2000 2000;}'.
    '.siteseg1 {stroke:brown;stroke-width:40000;opacity:0.4;stroke-dasharray:5000 5000;}'.
    '.siteseg3 {stroke:orange;stroke-width:40000;opacity:0.4;stroke-dasharray:5000 5000;}'.
    '.sitept1 {fill:brown;opacity:0.4;}'.
    '.sitept3 {fill:orange;opacity:0.4;}'.
    '.evtfoot  {fill:red;stroke:white;stroke-width:20000;}'.
    '.evtfoot2 {fill:red;stroke:blue;stroke-width:20000;}'.
    '.evtfoot3 {fill:gray;stroke:red;stroke-width:20000;}'.
    '.evtfootc {fill:pink;stroke:blue;stroke-width:20000;}'.
    '.evtfootworking {fill:yellow;stroke:green;stroke-width:20000;}'.
    '.edge_missing_foot {stroke:yellow;stroke-width:230000;opacity:0.5;stroke-dasharray:8000 8000;}'
    ;

# doesn't have to be medial axis specific
# this should be just a half-edge graph representation that can work for
# Voronoi or medial axis edges, maybe

sub new {
    my $class = shift;
    my $polygon = $_[0]; # expolygon: [[contour], [hole1], [hole2], ...]
    my $self;
    if (ref($_[0]) =~ /(ARRAY|Polygon)/) {
        $self = polygon_medial_axis($_[0]);
        bless $_, 'Slic3r::MedialAxis::Edge' for @{$self->{edges}};

        if (0 && $DB::svg) {
        $DB::svg->appendStyle($medial_axis_hpp_debug_styles);
        $DB::svg->appendRaw($self->{events});# if ($DB::current_layer_id > 88 && $DB::current_layer_id < 93);
        }
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
        
        if ($edge->curved && $edge->internal) {
            # my $tan_angle_1 = (atan2($edge->[FOOT]->[Y] - $edge->[POINT]->[Y],
                                     # $edge->[FOOT]->[X] - $edge->[POINT]->[X]) 
                             # + atan2($edge->[TWIN]->[NEXT]->[FOOT]->[Y] - $edge->[POINT]->[Y],
                                     # $edge->[TWIN]->[NEXT]->[FOOT]->[X] - $edge->[POINT]->[X])
                               # ) / 2;
            # my $tan_angle_2 = (atan2($edge->[NEXT]->[FOOT]->[Y] - $edge->[NEXT]->[POINT]->[Y],
                                     # $edge->[NEXT]->[FOOT]->[X] - $edge->[NEXT]->[POINT]->[X]) 
                             # + atan2($edge->[TWIN]->[FOOT]->[Y] - $edge->[NEXT]->[POINT]->[Y],
                                     # $edge->[TWIN]->[FOOT]->[X] - $edge->[NEXT]->[POINT]->[X])
                               # ) / 2;
            my $tan_angle_1 = (atan2($edge->foot->[1] - $edge->vertex0->[1],
                                     $edge->foot->[0] - $edge->vertex0->[0]) 
                             + atan2($edge->twin->next->foot->[1] - $edge->vertex0->[1],
                                     $edge->twin->next->foot->[0] - $edge->vertex0->[0])
                               ) / 2;
            my $tan_angle_2 = (atan2($edge->next->foot->[1] - $edge->next->vertex0->[1],
                                     $edge->next->foot->[0] - $edge->next->vertex0->[0]) 
                             + atan2($edge->twin->foot->[1] - $edge->next->vertex0->[1],
                                     $edge->twin->foot->[0] - $edge->next->vertex0->[0])
                               ) / 2;
            
            my $line1 = [[@{$edge->vertex0}[0, 1]], 
                         [$edge->vertex0->[0] + 100000 * CORE::cos($tan_angle_1),
                          $edge->vertex0->[1] + 100000 * CORE::sin($tan_angle_1)]
                        ];
            my $line2 = [[@{$edge->next->vertex0}[0, 1]],
                         [$edge->next->vertex0->[0] + 100000 * CORE::cos($tan_angle_2),
                          $edge->next->vertex0->[1] + 100000 * CORE::sin($tan_angle_2)]
                        ];

            $edge->curved(line_intersection($line1, $line2));

            # if $edge->[CURVED] is false the edge is straight
            # line_intersection() will return undef (false) when
            # the lines are practically parallel,
            # so in that case the edge will now be treated as straight
        }
    }
    bless $self, $class;
    return $self;
}

sub offset_interval_filter {
    my $self = shift;
    my ($tool_diameter, $count, $initial_offset, $bracket, $initial_bracket, $nudge) = @_;
    
    $nudge //= 0;
    
    my @dblines;
    
    # If $count is undef or zero, do as many $tool_diameter 
    # offsets as will fit per edge.
    # If $initial_offset is undef, default to a half $tool_diameter.
    $initial_offset //= ($tool_diameter / 2);
    
    # While the different offset levels put a lower bound
    # on radius for each level, $bracket puts an upper bound on the radius.
    # $bracket will be added to each level's offset to determine
    # the upper radius bound.
    # $bracket should be useful for limiting results to just thinwall
    # or thin gap subsets of the offset edges.
    $bracket //= $tool_diameter + $nudge;
    $initial_bracket //= $bracket;
    #$bracket=undef;

    my $max_intervals = int( ($self->{max_radius} - $initial_offset) 
                                   / ($tool_diameter)  )
                                        + 1;
    #print "max intervals $max_intervals\n";
    my $intervals_count = $count ? min($count, $max_intervals) : $max_intervals;
    #print "intervals $intervals_count\n";
    my @offset = map $_ * $tool_diameter + $initial_offset + $nudge, (0 .. $intervals_count);
    my @intervals;
    $#intervals = $#offset;
    $_ = [] for @intervals;
    my @last_loop_starts = map 0, (0 .. $intervals_count);
    my $last_loop_start = 0;

    for (my $i = 0; $i < @{$self->{edges}}; $i++) {
        my $e  = $self->{edges}->[$i];
        #print "edge: ",$self->{edges}->[$i],"\n";
        my $er1  = $e->radius;
        my $er2  = $e->next->radius;
        my $er_max = ($er1 > $er2) ? $er1 : $er2;
        my $er_min = ($er1 > $er2) ? $er2 : $er1;
        my $ephi = $e->phi;
        while ($ephi >  $pi) { $ephi-=2*$pi; }
        while ($ephi < -$pi) { $ephi+=2*$pi; }

        # enforce a minimum resolution - corners interpreted by resolution
        # or tool diameter are better modeled as circular arcs that the 
        # tool edge will touch - meaning edge radius can never go to zero
        my $resolution = $tool_diameter/2;

        my $ep = $e->prev;
        my $en = $e->next;

        my $edge_max_intervals = int( ($er_max - $initial_offset) 
                                        / $tool_diameter  )
                                             + 1;
        #print "edge max ints: $edge_max_intervals\n";
        my $edge_intervals = $count ? min($count, $edge_max_intervals) : $edge_max_intervals;

        for (my $j = 0; $j < $edge_intervals; $j++) {
            push @{$intervals[$j]}, [] if !(@{$intervals[$j]} > 0);
            #push @{$intervals[$j]}, [] if !@{$intervals[$j]};
            # edge list order matches edge->next order, except where
            # one edge loop subset ends and the next begins

            
            my $edge_intersect = ($er1 >  $offset[$j] && $er2 <= $offset[$j]) 
                               ? 1 : 0;

            my $bracket_threshold = $bracket 
                                  ? ($offset[$j]) + $bracket
                                  : $er_max + 1;
            
            # TODO: make phi threshold used here a parameter
            my $corner = (  $ephi < &Slic3r::Geometry::deg2rad(60)
                         || $ephi > &Slic3r::Geometry::deg2rad(120) )
                       ? 1 : 0;

            my $twinturn = (  ($edge_intersect
                                # both edges involved in the twinturn
                                # should have twinturn == true, not just the
                                # the first one going into the turn
                                || (   $er2 >  $offset[$j]
                                    && $er1 <= $offset[$j] )
                               )
                            && ($j != 0 ||
                                $corner # checking $corner might be superfluous
                               )
                           ) ? 1 : 0;
            
            push @{$intervals[$j]}, []  if ($er1 > $bracket_threshold ) && @{$intervals[$j]->[-1]};
            push @{$intervals[$j]}, []  if ($j != 0 && $er1 && $er1 <= $offset[$j] && $er2 > $offset[$j]) && @{$intervals[$j]->[-1]};

# corner issue not settled...
# can we get around it?
#$corner = 0;
            if ($er_min <= $bracket_threshold
                && (!$corner || $twinturn
                   
                   # trying to fix problem of false positive corner detection
                   || $er_min >= $offset[$j]
                   
                   )) {

                # debug - save keeper edges for display
                #push @dblines, [[$e->vertex0->[0], $e->vertex0->[1]], 
                #                [$e->next->vertex0->[0], $e->next->vertex0->[1]]
                #               ];

                push @{$intervals[$j]->[-1]},  Slic3r::MedialAxis::EdgeView->new(
                              $e,
                              $offset[$j],
                              $resolution,
                              ($er1  > $bracket_threshold || $er2  > $bracket_threshold)
                                  ? $bracket_threshold
                                  : undef,
                              $edge_intersect,
                              $twinturn
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
                # colloected so far. The first of those subsequences may start
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
                        $DB::svg->appendPoints({r=>100000, style=>"fill:aqua;"},
                          [$intervals[$j]->[-1]->[-1]->point->[0], $intervals[$j]->[-1]->[-1]->point->[1]],
                          [$intervals[$j]->[$last_loop_starts[$j]]->[0]->point->[0], $intervals[$j]->[$last_loop_starts[$j]]->[0]->point->[1]],
                          ) if 0 && $DB::svg;
                        $DB::svg->appendPolylines({style=>"stroke:aqua;stroke-width:30000;fill:none;"},
                          [map [@{$_->point}[0,1]], @{$intervals[$j]->[$last_loop_starts[$j]]}],
                          [map [@{$_->point}[0,1]], @{$intervals[$j]->[-1]}]
                          ) if 0 && $DB::svg;

                        # doesn't work quite the same when done in one line
                        # so do the splice and unshift separately, unless you
                        # figure out why it didn't work as one line
                        my ($last_one) = splice(@{$intervals[$j]}, -1, 1);
                        unshift @{$intervals[$j]->[$last_loop_starts[$j]]},
                                @$last_one;
                        
                        # fix prev,next,twin references
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

    $DB::svg->appendLines({style=>"stroke:green;stroke-width:40000;"}, @dblines
    ) if 1 && $DB::svg;

    foreach my $interval (@intervals) {
        @$interval  = grep @$_ > 0, @$interval;
    }
    
    # Merge contiguous sequences.
    # Some sequences of edges will be in two fragments - the last part of the 
    # sequence will come first, and later the first part of the sequence is
    # collected. So merge those subsequences where the later one's last
    # edge has an edge->next reference to the earlier one's first edge.
    foreach my $interval (@intervals) {
        # if later one ends where earlier one begins, prepend later to earlier
#$DB::svg->appendPolylines({style=>"stroke:blue;stroke-width:30000;fill:none;"}, map {[map {$_->points} @$_]} @$interval) if 1 && $DB::svg;
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
    foreach my $interval (@intervals) {
#        $DB::svg->appendPolylines({style=>"stroke:red;stroke-width:20000;fill:none;"}, map {[map {$_->points} @$_]} @$interval) if 1 && $DB::svg;
    }

    return @intervals;
}

# get_offset_fragments()
# Convert a set of lists of MedialAxis::EdgeViews
# (each set typically having the same offset value set for all the EdgeViews in it)
# to a set of lists of points representing Polygons or Polylines.
# Makes a distinction between "left" and "right" about the medial axis, and only
# generates point sequences from one side or the other, as requested.

sub get_offset_fragments {
    my ($intervals, $keep_marked) = @_;
    my @interval_thin_paths;

for (my $i = 0; $i < @$intervals; $i++) {
foreach my $polyedge (@{$intervals->[$i]}) {
print "visited\n" if @{$polyedge} && ($polyedge->[0]->visited || $polyedge->[-1]->visited);
}
}

    for (my $i = 0; $i < @$intervals; $i++) {
        my @new_outer_frags = ([]);
        foreach my $polyedge (@{$intervals->[$i]}) {
            $_->visited(0) for @$polyedge;
        }

        @{$intervals->[$i]} = grep @$_ > 1, @{$intervals->[$i]};

        foreach my $polyedge (@{$intervals->[$i]}) {

# this didn't fix immediate issue
# but seems like a good strategy in genereral here to make sure we're
# only dealing with one level of branching when "thin wall" is anything less
# than _2_ extrusion widths, not one
# so fix other thing, then come back and consider this
#$DB::svg->appendRaw('<g>');
#my @polyedge = map {$_->[1]/=2.1;$_} grep scalar($_->points) > 0, map {$_->[1]*=2.1;$_} @$polyedge;

#my $lp = $polyedge->[-1]->end_point;

#print "len:",scalar(@$polyedge),"\n";

my @polyedge = map {$_->[1]*=2.1;$_} @$polyedge;
$polyedge[-1]->next->[1] *= 2.1;

my @polyedge = grep scalar($_->points) > 0, @polyedge;

#$DB::svg->appendPolylines({style=>"stroke:blue;opacity:0.51;stroke-width:50000;fill:none;"},  [map {$_->points} @polyedge]) if (@polyedge) && $DB::svg;
@polyedge = map {$_->[1]/=2.1;$_} @polyedge;
$polyedge[-1]->next->[1] /= 2.1;
#$DB::svg->appendPolylines({style=>"stroke:lightblue;opacity:1;stroke-width:40000;fill:none;"},  [map {$_->points} @polyedge]) if (@polyedge) && $DB::svg;
#$DB::svg->appendRaw('</g>');

            # Initial pass sets up a rough left-right edge distinction
            # for thin wall sections, that we will then refine.
            # Processing an earlier polyedge can take care of one that comes
            # later, so don't reprocess that later one if eiter end has already
            # been marked.
            #if (@{$polyedge} && !$polyedge->[0]->visited) {
            #    foreach my $edge (@$polyedge) {
            if (@polyedge && !$polyedge[0]->visited) {
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
        #$DB::svg->appendRaw('<g>');
        #    my @polyedge = grep scalar($_->points) > 0, @$polyedge;
        #    #my @polyedge = grep 1 || scalar($_->points) > 0, @$polyedge;
        #$DB::svg->appendRaw('</g>');
            # TODO Should save and use those points instead of regenerating 
            # them at the end. 
#$DB::svg->appendRaw('<g>');            
#$DB::svg->appendPolylines({style=>"stroke:red;opacity:1;stroke-width:20000;fill:none;"},  [map {$_->points} grep {$_->visited != 2 || $_->visited != 1} @polyedge]) if (grep {$_->visited != 2 || $_->visited != 1} @polyedge) && $DB::svg;
#$DB::svg->appendPolylines({style=>"stroke:yellow;opacity:1;stroke-width:20000;fill:none;"},  [map {$_->points} grep {$_->visited==2} @polyedge]) if (grep {$_->visited==2} @polyedge) && $DB::svg;
#$DB::svg->appendPolylines({style=>"stroke:pink;opacity:1;stroke-width:20000;fill:none;"},    [map {$_->points} grep {$_->visited==1} @polyedge]) if (grep {$_->visited==1} @polyedge) && $DB::svg;
#$DB::svg->appendPoints({style=>"",r=>20000},  map {[@{$_->start_point},$_->visited == 1 ? 'blue':'red']} @polyedge) if $DB::svg;
#$DB::svg->appendRaw('</g>');            

if (0) {
            # Fix up visited to be same for all consecutive edges (even out 
            # to nested branches) at start and finish of edge sequence.
            my $start_mark = $polyedge[0]->visited;
            #foreach my $edge (@polyedge) {
            for (my $i = 0; $i < @polyedge; $i++) {
                my $edge = $polyedge[$i];
                my $nextedge = $i != $#polyedge ? $polyedge[$i+1] : $edge->next; # not ideal
                if ($edge->radius >= $edge->offset*2
|| ($edge->radius == 0 && $nextedge->radius >= $edge->offset*2)
                   
                   ) {
                    $edge->visited($start_mark);                            
                } else {last;}
                #last if $edge->radius <= $edge->offset && $nextedge->radius <= $edge->offset
            }
            if ($polyedge[0]->twin == $polyedge[-1]) {
                $start_mark = $start_mark == 1 ? 2 : 1;
            }
            #foreach my $edge (reverse @polyedge) {
            for (my $i = $#polyedge; $i > -1; $i--) {
                my $edge = $polyedge[$i];
                my $nextedge = $i != $#polyedge ? $polyedge[$i+1] : $edge->next; # not ideal
                if ($nextedge->radius >= $edge->offset*2
|| ($nextedge->radius == 0 && $edge->radius >= $edge->offset*2)
                   ) {
                    $edge->visited($start_mark);                            
                } else {last;}
                #last if $edge->radius <= $edge->offset && $nextedge->radius <= $edge->offset
            }
#$DB::svg->appendRaw('<g>');
#$DB::svg->appendPolylines({style=>"stroke:white;opacity:0.5;stroke-width:20000;fill:none;"},  [map {$_->points} grep {$_->visited == 2} @polyedge]) if (grep {$_->visited == 2} @polyedge) && $DB::svg;
#$DB::svg->appendPolylines({style=>"stroke:purple;opacity:0.5;stroke-width:20000;fill:none;"}, [map {$_->points} grep {$_->visited == 1} @polyedge]) if (grep {$_->visited == 1} @polyedge) && $DB::svg;
#$DB::svg->appendPoints({style=>"",r=>15000},  map {[@{$_->start_point},$_->visited == 1 ? 'green':'yellow']} @polyedge) if $DB::svg;
#$DB::svg->appendRaw('</g>');
}
            # these two for()s do one level of branch visited flag fixup
            # and one level should be all we need when handling just the 
            # edges with points when radius < offset
if (1) {
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
}
            
            # add the left or right edge to our collection
            push(@new_outer_frags, []) if @{$new_outer_frags[-1]};
            foreach my $edge (@polyedge) {
                if (!$keep_marked) {                        
                    push(@{$new_outer_frags[-1]}, $edge) if $edge->visited == 1;
                    push(@new_outer_frags, []) if ($edge->visited == 2 && @{$new_outer_frags[-1]});
                } else {
                    push(@{$new_outer_frags[-1]}, $edge) if $edge->visited == 2;
                    push(@new_outer_frags, []) if ($edge->visited == 1 && @{$new_outer_frags[-1]});
                }
            }
        }

        # filter out frags that don't actually go into thin sections

        @new_outer_frags = grep {
            #$DB::svg->appendPolylines({style=>"stroke:orange;opacity:1;stroke-width:20000;fill:none;"},  [map {$_->points} grep {$_->visited == 2} @$_]) if (grep {$_->visited == 2} @$_) && $DB::svg;
            #$DB::svg->appendPolylines({style=>"stroke:lime;opacity:1;stroke-width:20000;fill:none;"},    [map {$_->points} grep {$_->visited == 1} @$_]) if (grep {$_->visited == 1} @$_) && $DB::svg;

             (grep {
                  (
                   (       $_->radius  >= $_->offset*2
                   && $_->next->radius <  $_->offset*2
                   && $_->next->radius != 0
                   && ! $_->twinturn
                   )
                  ||
                   (        $_->radius <  $_->offset*2
                   && $_->next->radius >= $_->offset*2
                   && $_->radius != 0
                   && ! $_->twinturn
                   )
                 )
             } @$_)
             # || 
              # (grep {
                  # (        $_->radius <= $_->offset*2
                  # && $_->next->radius <= $_->offset*2
                  # )                     
              # } @$_)
              || (grep {
                   ($_->radius < $_->offset*2.1 && ! $_->twinturn)                     
               } @$_)
              
              
             # || (@$_ &&
                     # (    
                      # # special case from above sets this up to detect to keep
                      # $_->[-1]->radius == 0
                      # ||
                      # # similar where we start at edge coming out of corner
                      # # TWINTURN is the key to distinguishing those cases
                      # # from other corner cases we mean to filter out, where TWINTURN == 1 or true
                        # ($_->[0]->radius == 0
                         # && !$_->[0]->twinturn
                        # )
                     # )
                 # )
            } @new_outer_frags
            if 1;

if (0) { # instead of these, try using an endpoint() function on the last EdgeView edge when collecting final points after this disabled section
    
        # for last edge, add copy of edge after last edge, to 
        # get the end point of the last edge
        for (my $i = 0; $i < @new_outer_frags; $i++) {
            if (@{$new_outer_frags[$i]} 
                #skip loop case handled next
                && $new_outer_frags[$i]->[-1]->next != $new_outer_frags[$i]->[0]->edge
               ) {
                my $end = Slic3r::MedialAxis::EdgeView->new(
                    #($edge, $offset, $resolution, $interpolate, $intersect, $twinturn)
                    $new_outer_frags[$i]->[-1]->next,
                    $new_outer_frags[$i]->[-1]->offset,
                    $new_outer_frags[$i]->[-1]->[&Slic3r::MedialAxis::EdgeView::RESOLUTION],
                    undef,
                    0,
                    0
                    );
                push @{$new_outer_frags[$i]}, $end;
            }
        }
}
if (0) {
        # for loops, add copy of first edge at end, to close the loop
        # (will be trimmed for extrusion radius later)
        for (my $i = 0; $i < @new_outer_frags; $i++) {
            if (@{$new_outer_frags[$i]}
                && $new_outer_frags[$i]->[-1]->next == $new_outer_frags[$i]->[0]->edge
               ) {
                my $end = Slic3r::MedialAxis::EdgeView->new(@{$new_outer_frags[$i]->[0]});
                $end->[&Slic3r::MedialAxis::EdgeView::INTERPOLATE] = undef;
                $end->[&Slic3r::MedialAxis::EdgeView::INTERSECT] = 0; # turn off if it was on
                push @{$new_outer_frags[$i]}, $end;
            }
        }
}

        my $thin_paths = [
            grep @$_ > 1, 
            map [(map {(grep $_, ($_->start_point, $_->mid_points))} @$_),
                 ($_->[-1]->end_point // ())
                ],
            grep @$_, @new_outer_frags];

        @$thin_paths = grep {   @$_ >  2
            || (@$_ == 2 && &Slic3r::Geometry::distance_between_points($_->[0], $_->[1]) > &Slic3r::SCALED_RESOLUTION)
            } @$thin_paths;

        push @interval_thin_paths, $thin_paths;
    }
    return \@interval_thin_paths;
}

sub merge_expolygon_and_medial_axis_fragments {
    my ($expolygon, $offset_expolygons, $medial_axis_fragments, $distance, $direction) = @_;

    my @ma = @$medial_axis_fragments;
    my @all_expolygons = @$offset_expolygons;
    my @loops_and_paths;

    if (!@ma) {
        return $offset_expolygons;
    }

    # Get expolygon that is the original minus the offset polygon(s).
    # This is the zone where thin walls and points live.
    my $diff = Slic3r::Geometry::Clipper::diff_ex(
        $expolygon,
        [map @$_, @all_expolygons]
        );
    #$DB::svg->appendPolygons({style=>'opacity:0.8;stroke-width:'.(0*$distance/12).';stroke:pink;fill:pink;stroke-linecap:round;stroke-linejoin:round;'}, @$diff) if $DB::svg;

    # need to reimplement trim at junctions for this boost::polygon::voronoi based version
    #Slic3r::MedialAxis::trim_polyedge_junctions_by_radius(\@ma, $distance);

    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance).';stroke:green;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, map Slic3r::Polyline->new([map $_, @$_]), @ma) if $DB::svg;

    my @ma_as_polylines = map Slic3r::Polyline->new($_), grep @$_ > 1, @ma;
    $DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/3).';stroke:brown;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @ma_as_polylines) if $DB::svg;

    return $offset_expolygons if !@ma_as_polylines;
    
    # Get the medial axis fragments that are in the thin wall zone.
    my @boost_trimmed = map @{Boost::Geometry::Utils::polygon_multi_linestring_intersection
                              ($_, [@ma_as_polylines])
                             }, @$diff;

    bless $_, 'Slic3r::Polyline' for @boost_trimmed;
    bless $_, 'Slic3r::Point' for map @$_, @boost_trimmed;
    
    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance).';stroke:aqua;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @boost_trimmed) if $DB::svg;

    # Break ExPolygons into fragments where they intersect with the medial axis.

    my @offset_clipped = map $_->clip_with_polylines(\@ma_as_polylines), map $_->contour, @all_expolygons;
    my @offset_polygons = grep $_->isa('Slic3r::Polygon'), (@offset_clipped);
    my @offset_polylines = grep !$_->isa('Slic3r::Polygon'), (@offset_clipped);
    my @offset_holes_clipped = map $_->clip_with_polylines(\@ma_as_polylines), map $_->holes, @all_expolygons;
    my @offset_holes_polygons = grep $_->isa('Slic3r::Polygon'), (@offset_holes_clipped);
    my @offset_holes_polylines = grep !$_->isa('Slic3r::Polygon'), (@offset_holes_clipped);

    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/2).';stroke:aqua;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @offset_polylines) if $DB::svg;


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
    }
    #$DB::svg->appendPolylines({style=>'opacity:0.8;stroke-width:'.($distance/4).';stroke:purple;fill:none;stroke-linecap:round;stroke-linejoin:round;'}, @offset_polylines) if $DB::svg;

    # Splice together medial axis fragments
    # and polygon fragments into continuous paths.

    my @all_frags = grep @$_ > 1, (@offset_polylines, @offset_holes_polylines, @boost_trimmed);

    Slic3r::Geometry::combine_polyline_fragments(\@all_frags, $distance/20);

    # TODO: some of those might have turned back into polygons
    # and some of those could be holes
    # should detect that and leave them that way instead of treating as polylines
    # and put the holes with other untouched holes
    # Then, need to get fragmented holes in with the normal holes too, to 
    # preserve the intended ordering from the hole sorting that happens below.
    # But somehow that has to handle Polylines for it's nesting tests.
    # ... reconsider above - things have changed a bit since then

    my @ex_frags = map Slic3r::ExPolygon->new($_), @all_frags;

    # Hack Polylines into ExPolygons.
    bless($_->[0], 'Slic3r::Polyline') for @ex_frags;

    # Preserve polygons and holes that didn't get split up by the medial axis.
    push @ex_frags, @{union_ex([@offset_polygons, @offset_holes_polygons])};

    return \@ex_frags;
}

# Edge
# A bit nicer interface to the half-edge graph edges
package Slic3r::MedialAxis::Edge;
use strict;

use constant SOURCE_INDEX => 0;
use constant POINT    => 1; # should rename this 'vertex0' and use 'point' for the polygon point, or foot
use constant TWIN     => 2;
use constant NEXT     => 3;
use constant PREV     => 4;
use constant THETA    => 5;
use constant PHI      => 6;
use constant CURVED   => 7;
use constant PRIMARY  => 8;
use constant INTERNAL => 9;
use constant FOOT     => 10;
use constant VISITED  => 11;
use constant X => 0;
use constant Y => 1;
use constant RADIUS => 2;

sub new {
    my $class = shift;
    my $self = $_[0];
    bless $self, $class;
    return $self;
}

# generate accessors for the edge data fields
eval('sub '.lc($_).' {return $_[0]->['.$_.'];} ') 
   for qw(SOURCE_INDEX POINT TWIN NEXT PREV THETA PHI PRIMARY INTERNAL FOOT);
sub vertex0 { $_[0]->point }
sub vertex1 { $_[0]->next->point }
sub point0 { $_[0]->foot }
sub point1 { $_[0]->next->foot }
sub radius { $_[0]->point->[RADIUS] }
sub curved { $_[0]->[CURVED] = $_[1] if @_ > 1; $_[0]->[CURVED] }
sub visited { $_[0]->[VISITED] = $_[1] if @_ > 1; $_[0]->[VISITED] }

sub start_point { $_[0]->point0 }
sub end_point { $_[0]->next->point0 }
sub mid_points { return () } # TODO - curve samples, or seg samples, to some resolution

sub rot_next { $_[0]->twin->next }
sub rot_prev { $_[0]->prev->twin }


# EdgeView
# Extends the medial axis half-edge graph Edge objects to enable 
# offset polygons and radius filters radius filters.

package Slic3r::MedialAxis::EdgeView;
use strict;

use constant EDGE => 0;
use constant OFFSET => 1;
use constant RESOLUTION => 2;
use constant INTERPOLATE => 3;
use constant INTERSECT => 4;
use constant TWINTURN => 5;
use constant EVTWIN => 6;
use constant EVNEXT => 7;
use constant EVPREV => 8;

# generate accessors for most of the underlying edge data fields
eval('sub '.lc($_).' {return $_[0]->[0]->[&Slic3r::MedialAxis::Edge::'.$_.'];}')
    for qw(SOURCE_INDEX POINT THETA PHI CURVED PRIMARY INTERNAL FOOT);

sub new {
    my $class = shift;
    # edge, offset, resolution, interpolate, intersect, twinturn
    my $self = [@_];
    bless $self, $class;
    return $self;
}

sub edge        { return $_[0]->[EDGE]; }
sub offset      { return $_[0]->[OFFSET]; }
sub resolution  { return $_[0]->[RESOLUTION]; }
sub intersect   { return $_[0]->[INTERSECT]; }
sub interpolate { return $_[0]->[INTERPOLATE]; }
sub radius      { return $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::POINT]->[&Slic3r::MedialAxis::Edge::RADIUS]; }
sub visited     { my $self = shift; return $self->edge->visited(@_); }
sub twinturn    { $_[0]->[TWINTURN] = $_[1] if @_ > 1; return $_[0]->[TWINTURN]; }

# These probably shouldn't return Edge objects in the fall-through cases.
# Maybe either allow null references at ends of EdgeView sequences, or upgrade
# Edge there to EdgeView.

sub next {
    $_[0]->[EVNEXT] = $_[1] if @_ > 1;
    return $_[0]->[EVNEXT]
           ? $_[0]->[EVNEXT]
           : $_[0]->[INTERSECT] && $_[0]->[TWINTURN] 
             ? 
               #$_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::TWIN]
                $_[0]->[EVNEXT] = Slic3r::MedialAxis::EdgeView->new(
                $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::TWIN],
                $_[0]->offset,
                $_[0]->resolution,
                undef,
                0,
                0,
                undef,
                undef,
                $_[0]
                )
             : 
               #$_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::NEXT]
                $_[0]->[EVNEXT] = Slic3r::MedialAxis::EdgeView->new(
                $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::NEXT],
                $_[0]->offset,
                $_[0]->resolution,
                undef,
                0,
                0,
                undef,
                undef,
                $_[0]
                )
             ;
}
sub prev {
    $_[0]->[EVPREV] = $_[1] if @_ > 1;
    return $_[0]->[EVPREV]
           ? $_[0]->[EVPREV]
           : $_[0]->[INTERSECT] && $_[0]->[TWINTURN] 
             ? $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::TWIN]
             : $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::PREV];
}
sub twin {
    $_[0]->[EVTWIN] = $_[1] if @_ > 1;
    return $_[0]->[EVTWIN]
           ? $_[0]->[EVTWIN]
           : $_[0]->[EDGE]->[&Slic3r::MedialAxis::Edge::TWIN];
}

# point0, point_i and point1 return points on, or offset from, 
# the original polygon.
# point0 is the location of the edge's foot on the polygon
# point1 is the location of the next edge's foot
# The segment between those two points lies on one of the original polygon segments,
# or is offset and parallel to it.

sub point0 {
    my ($self, $off) = @_;

    $off //= $self->offset;
    
    my $thetaphi = $self->theta + $self->phi;     # precalculate this
    while ($thetaphi >  $pi) {$thetaphi-=2*$pi;}  # somewhere maybe
    while ($thetaphi < -$pi) {$thetaphi+=2*$pi;}
    
    if (   $self->offset >= $self->radius
        && $self->offset >= $self->next->radius) {        
        return [$self->vertex0->[0], $self->vertex0->[1]];
    }

    # Normally positive offset is from the polygon inward.
    # For now, if it's negative we'll offset from the medial axis edge outward.
    # But this convention might change with the evolution of the Edge and 
    # EdgeView models.
    my $radius =  defined($off) && $off > 0 ? $self->radius - $off : -$off;

    my $pointx = $radius * cos($thetaphi) + $self->point->[0];
    my $pointy = $radius * sin($thetaphi) + $self->point->[1];
    
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
    my $self = shift;
    my $p;
    
    if ($self->next->isa('Slic3r::MedialAxis::EdgeView')) {
        $p = $self->next->point0;
    } else {
        # at the ends of a sequence of EdgeViews that represents a fragment
        # of the medial axis or offset polygon, the next and prev references
        # might still point to Edge objects instead of EdgeView objects.
        # Should fix that where you make those EdgeView sequences, but
        # for now, fixing that here for the "next" edge case.
        $self->[EVNEXT] = 
            Slic3r::MedialAxis::EdgeView->new(
            $self->next,
            $self->offset,
            $self->resolution,
            undef,
            0,
            0,
            undef,
            undef,
            $self
            )
        ;
        $p = $self->[EVNEXT]->point0;
    }
    
    if ($self->edge->next == $self->edge->twin) {
        # account for phi not being the same for consecutive edges
        # in corners, where the medial axis touches the polygon
        $p = _reflect($p, $self->vertex0, $self->vertex1);
    }
    return $p;
    }

#point_i is an interpolated point between point0 and point1.
# $target is a target radius
# The idea is to find the point between point0 and point1 where the
# interpolated radius matches target. So this is only meant to work when
# $target is within the radius range of the edge.
# By default, this will return the interpolated point where the offset (from
# the polygon) segment intersects this medial axis edge.
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
        $p1 = $self->point0;
        $p2 = $self->point1;
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
sub vertex1 { $_[0]->next->vertex0 }
sub vertex_i {
    my ($self, $factor) = @_;
    my ($p1, $p2) = ($self->vertex0, $self->vertex1);
    return [$p1->[0] + $factor * ($p2->[0] - $p1->[0]),
            $p1->[1] + $factor * ($p2->[1] - $p1->[1])
           ];
}

# start_point, end_point and mid_points for an edge take into account
# radius, offset, interpolate and twinturn options
# use these to get the proper "view" of an EdgeView

sub start_point {
    my $self = shift;
    die "EDGE" if !$self->isa('Slic3r::MedialAxis::EdgeView');    
    my $ret;
    if (defined($self->interpolate) &&  $self->interpolate < $self->radius - $self->offset) {
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
    die "EDGE" if !$self->isa('Slic3r::MedialAxis::EdgeView');    
    my $ret;
    if (defined($self->interpolate) &&  $self->interpolate < $self->next->radius - $self->offset) {
        $ret = $self->point_i($self->interpolate);
    } elsif ($self->radius > $self->offset && $self->next->radius < $self->offset) {
        # the medial axis edge-intersecting point
        if ($self->twinturn) { $ret= $self->point_i; }
        else { $ret = $self->vertex1; }
    } elsif ($self->radius < $self->offset && $self->next->radius < $self->offset) {
        $ret = $self->vertex1;
    } else {
        $ret = $self->point1;
        die "EDGE" if !$self->next->isa('Slic3r::MedialAxis::EdgeView');    
    }
    return $ret;
}

sub mid_points {
    my $self = shift;
    die "EDGE" if !$self->isa('Slic3r::MedialAxis::EdgeView');    

    if (!$self->twinturn &&
          (  ($self->radius > $self->offset && $self->next->radius < $self->offset)
          || ($self->radius < $self->offset && $self->next->radius > $self->offset)
          )
        ) {
        # the medial axis edge-intersecting point
        return ($self->point_i);
    } elsif(0 && $self->curved) {
        # TODO: curve samples that are compatible with Clipper offset polygons
        #       with whatever miter limit is being used - or just samples at some resolution
    } else {
        return ();
    }
}

sub points {
    my $self = shift;

    #$self->edge_svg; # for debug
    
    return grep $_, (
             $self->start_point, 
             $self->mid_points, 
             $self->end_point
             );
}

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

sub _rotate_2d {
    my ($p1, $theta, $origin) = @_;
    return [
            (($p1->[0] - $origin->[0]) * CORE::cos($theta) - ($p1->[1] - $origin->[1]) * CORE::sin($theta)) + $origin->[0],
            (($p1->[1] - $origin->[1]) * CORE::cos($theta) + ($p1->[0] - $origin->[0]) * CORE::sin($theta)) + $origin->[1]
           ];
  }

sub _reflect {
    my ($p1, $p2, $p3) = @_;
    my $dy = $p3->[1] - $p2->[1];
    my $dx = $p3->[0] - $p2->[0];
    if ($dy == 0 && $dx == 0) {
        #warn "Can't reflect about a point";
        return $p1;
        }
    my $theta = atan2($dy, $dx);
    $p1 = _rotate_2d($p1, -$theta, $p2);
    $p1->[1] -= $p2->[1];
    $p1->[1] *= -1.0;
    $p1->[1] += $p2->[1];
    return _rotate_2d($p1, $theta, $p2);
  }

1;
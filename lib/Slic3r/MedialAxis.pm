package Slic3r::MedialAxis;
use strict;
use warnings;

use Math::Geometry::Delaunay 0.10 qw(TRI_CONFORMING);
use Slic3r::Geometry qw(PI angle3points distance_between_points deg2rad same_point polyline_remove_short_segments point_along_segment);

use List::Util qw(first);
use Scalar::Util qw(blessed); # until we make an ma object and can use isa() to tell between that and polygon/line

require Exporter;
our @ISA = qw(Exporter);
our @EXPORT_OK = qw(medial_axis thin_paths_filter splice_into_perimeter);

our $miterLimit = 2; # cooridnate with what's used for Clipper offsets

# Medial Axis

# Shewchuk's Triangle C library provides the constrained Delaunay triangulation
# and Voronoi diagram for a "planar straight line graph", and supports concavities
# and holes.

# Math::Geometry::Delaunay's mic_adjust() moves the vertices of the
# Voronoi diagram to better approximate the medial axis, and it adds
# radius and tangent point data to the nodes.

# Here we want to take an ExPolygon as input, get the Voronoi diagram,
# straighten it out with mic_adjust(), and then do some additional pruning
# and conditioning of the resulting topological structure to get something like
# polygons and and polylines, but with each node/vertex retaining additional
# attributes, like radius and MIC tangent points.

sub medial_axis {
    my ($expolygon, $width, $thresholds) = @_;

    $expolygon = $expolygon->clone;

    foreach my $polygon (@$expolygon) {
        # mostly to eliminate duplicate points
        polyline_remove_short_segments($polygon, &Slic3r::SCALED_RESOLUTION);
        # minimally subdivide polygon segments so that the
        # triangulation captures important shape features, like corners 
        $polygon->splitdivide($width / 2) if $width;
        # the rest can be a coarser subdivision
        #$polygon->subdivide(5 * $width) if $width;
    }
    
    # get the Delaunay triangulation and Voronoi diagram topology
    
    my $tri = Math::Geometry::Delaunay->new();
    $tri->addPolygon($expolygon->contour);
    $tri->addHole($_) for $expolygon->holes;
    my ($topo, $vtopo) = $tri->triangulate(TRI_CONFORMING,'ev');

    # Move vertices in Voronoi diagram ($vtopo) to make them 
    # work much better as a medial axis approximation.

    Math::Geometry::Delaunay::mic_adjust($topo, $vtopo);

    # Remove references to ray edges from all nodes.

    foreach my $node (@{$vtopo->{nodes}}) {
        # vector [0,0] means it's not a ray, so keep it
        @{$node->{edges}} = grep !defined($_->{vector})
                                    || (  $_->{vector}->[0] == 0
                                       && $_->{vector}->[1] == 0 ),
                                     @{$node->{edges}};
    }
    
    # Insert interpolated nodes that have the given threshold radii.
    # Because we want to use the medial axis to detect and fill narrow
    # passages, we want it to have vertices exactly where the 
    # transition to "narrow" happens, to cut at that point.
    # Also, to facilitate splicing into Clipper offset polygons, we
    # can make sure there's a point very close to where those polygons
    # turn around in sharp corners. (Depends on miter limit and varies with
    # local geometry, but with some coordination we should get close).
    
    # Consider doing this interpolation just where needed in other places
    # like in a trimming sub, where you still have enough node, edge, tangent
    # info to do it there. An general interpolate_nodes() sub would be good.
    # but until then, this works well.

    foreach my $threshold_radius (@$thresholds) {
        next if !$threshold_radius;

        my @adjust_queue = (0 .. $#{$vtopo->{nodes}});

        while (scalar @adjust_queue) {
            my $node_index = shift @adjust_queue;
            my $node = $vtopo->{nodes}->[$node_index];

            # This adjustment improves the result when the threshold
            # is meant to get an interpolated point right where a Clipper
            # offset has a point in a miterLimit-squared-off corner.
            # (If thresholds here have any other purpose in the future, this 
            # adjustment may not be what we want.)
            
            # second arg is new and used with new MIC object which is not in 
            # place at this point.
            # with undef, we'll have the function work the old way.
            # Ideally we get rid of all this interpolation
            # here and only do it in filters later, when the new MIC object is
            # what we're working with.
            my $threshold_radius_adj = adjusted_radius_for_miter_limit($node, undef, $threshold_radius/2, $miterLimit);

            if ($node->{radius} < $threshold_radius_adj) {

                for (my $i = $#{$node->{edges}}; $i > -1; $i--) {

                    my ($other_node_index, $this_node_index) = $node == $node->{edges}->[$i]->{nodes}->[0] ? (1, 0) : (0, 1);
                    my $other_node = $node->{edges}->[$i]->{nodes}->[$other_node_index];

                    if ($other_node->{radius} > $threshold_radius_adj) {

                        # All nodes directly connected to this one should 
                        # reevaluate their situation after this one has been 
                        # processed, so add them to the end of the queue again.
                        for (my $j = $#{$node->{edges}}; $j > -1; $j--) {
                            my $other_node_index = $node == $node->{edges}->[$j]->{nodes}->[0] ? 1 : 0;
                            # convert to the index in $vtopo->{nodes} list
                            $other_node_index = $node->{edges}->[$j]->{nodes}->[$other_node_index]->{index};
                            push @adjust_queue, $other_node_index;
                        }

                        # clone
                        my $this_node = {
                                          index  => scalar(@{$vtopo->{nodes}}),
                                          point  => [@{$node->{point}}],
                                          radius => $node->{radius},
                                          tangents  => [[map {[@$_]} @{$node->{tangents}->[$i]}]],
                                          edges  => [],
                                          elements => [],
                                          segments => [],
                                          marker => undef,
                                          attributes => [],
                                          };

                        # attach current edge to new node, detach from old
                        my $edge = $node->{edges}->[$i];
                        splice @{$edge->{nodes}}, $this_node_index, 1, $this_node;
                        @{$this_node->{edges}} = splice @{$node->{edges}}, $i, 1;

                        # Tangents list order needs to match edge list order,
                        # so remove and re-add to end of list, where new edge will be added.
                        my $tmptan = splice @{$node->{tangents}}, $i, 1;

                        # Add an edge that links the the new node to the old one.
                        push @{$vtopo->{edges}}, {
                                                  nodes    => [$node,$this_node],
                                                  index    => scalar(@{$vtopo->{edges}}),
                                                  vector   => [0,0],
                                                  elements => [],
                                                  };
                        push @{$node->{edges}}, $vtopo->{edges}->[-1];
                        push @{$node->{tangents}}, $tmptan;
                        push @{$this_node->{edges}}, $vtopo->{edges}->[-1];
                        push @{$vtopo->{nodes}}, $this_node;
                        $this_node->{isinterp} = 1;

                        # interpolate vertex to where radius == threshold_radius_adj
                        my $factor = (($threshold_radius_adj) - $this_node->{radius}) / ($other_node->{radius} - $this_node->{radius});
                        $this_node->{point} = [$this_node->{point}->[0] + $factor * ($other_node->{point}->[0] - $this_node->{point}->[0]),
                                               $this_node->{point}->[1] + $factor * ($other_node->{point}->[1] - $this_node->{point}->[1])];
                        $this_node->{radius} = $threshold_radius_adj;

                        # interpolate tangent point pair
                        my $other_edge_node_edge_index = +(grep $other_node->{edges}->[$_] == $this_node->{edges}->[0], (0..$#{$other_node->{edges}}))[0];
                        my $other_tangents = $other_node->{tangents}->[$other_edge_node_edge_index];

                        $this_node->{tangents}->[0]->[0] = [$this_node->{tangents}->[0]->[0]->[0] + $factor * ($other_tangents->[1]->[0] - $this_node->{tangents}->[0]->[0]->[0]),
                                                            $this_node->{tangents}->[0]->[0]->[1] + $factor * ($other_tangents->[1]->[1] - $this_node->{tangents}->[0]->[0]->[1])];
                        $this_node->{tangents}->[0]->[1] = [$this_node->{tangents}->[0]->[1]->[0] + $factor * ($other_tangents->[0]->[0] - $this_node->{tangents}->[0]->[1]->[0]),
                                                            $this_node->{tangents}->[0]->[1]->[1] + $factor * ($other_tangents->[0]->[1] - $this_node->{tangents}->[0]->[1]->[1])];
                        $this_node->{tangents}->[1]      = [$this_node->{tangents}->[0]->[1], 
                                                            $this_node->{tangents}->[0]->[0]];

                        $this_node->{color} = '#FFFF00'; # for SVG debug output
                    }
                }
            }
        }
    }

    # If the first, minimum threshold radius was greater than zero,
    # remove any nodes with radius smaller than that.
    if ($thresholds->[0]) {
        foreach my $node (@{$vtopo->{nodes}}) {
            if ($node->{radius} < $thresholds->[0]) {
                for (my $i = $#{$node->{edges}}; $i > -1; $i--) {
                    my $other = $node == $node->{edges}->[$i]->{nodes}->[0]
                                ? $node->{edges}->[$i]->{nodes}->[1]
                                : $node->{edges}->[$i]->{nodes}->[0];
                    @{$other->{edges}} = grep $_ != $node->{edges}->[$i] ,@{$other->{edges}};
                }
                $node->{edges} = [];
            }
        }
    }

    my @vnodes = grep @{$_->{edges}}, @{$vtopo->{nodes}};

    # Walk the cross referenced nodes and edges to build up polyline-like node lists.

    my @polyedges = ();
    my %end_edges;

    # Start at tips, and when you reach branch nodes
    # add them to the @start_nodes list. This way you
    # have better control of orientation ("inward"),
    # and you can keep track of branch node depth from tips.
    my @start_nodes = grep @{$_->{edges}} == 1, @vnodes;
    
    # Linked loops could have branches and no tips.
    # In that case, start from branch nodes.
    if (@start_nodes == 0) {
        @start_nodes = grep @{$_->{edges}} > 2, @vnodes;
    }

    # Otherwise, it's a single loop - any node can be the start node.
    if (@start_nodes == 0) {
        push @start_nodes, $vnodes[0];
        $end_edges{$vnodes[0]->{edges}->[-1]} = 1;
    }
    
    foreach my $start_node (@start_nodes) {

        my @starts = @{$start_node->{edges}};

        if (@{$start_node->{edges}} == 3
            && (grep $end_edges{$_}, @{$start_node->{edges}}) == 1) {

            # At a branch node with two unprocessed edges
            # always turn to the left, relative the
            # one processed edge coming into the node.
            # Tends to make rings that may not end up as polygons still
            # have component paths oriented in the same direction.

            my $seen_ind  = +(grep $end_edges{$start_node->{edges}->[$_]}, (0,1,2))[0];
            my $next_ind  = ($seen_ind + 1) % scalar(@{$start_node->{edges}});
            my $other_ind = ($seen_ind + 2) % scalar(@{$start_node->{edges}});
            my $p1 = +(grep $_ != $start_node, @{$start_node->{edges}->[$seen_ind]->{nodes}})[0];
            my $p3 = +(grep $_ != $start_node, @{$start_node->{edges}->[$next_ind]->{nodes}})[0];
            my $p4 = +(grep $_ != $start_node, @{$start_node->{edges}->[$other_ind]->{nodes}})[0];
            # Choose the edge that makes a larger angle with the processed edge.
            # Works because angle3points() converts negative angles to positive.
            if (  angle3points($start_node->{point},$p1->{point},$p3->{point})
                < angle3points($start_node->{point},$p1->{point},$p4->{point})
               ) {
                $next_ind = $other_ind;
            }
            @starts = ($start_node->{edges}->[$next_ind]);
        }
        
        foreach my $start_edge (@starts) {

            # don't go back over path already traveled
            next if $end_edges{$start_edge};

            my $this_node = $start_node;
            push @polyedges, [];
            push @{$polyedges[-1]}, $this_node;
            my $this_edge = $start_edge;
            
            #step along nodes: next node is the node on current edge that isn't this one
            while ($this_node = +(grep $_ != $this_node, @{$this_edge->{nodes}})[0]) {
                # stop at point too close to polygon (those nodes had all edges removed)
                if (@{$this_node->{edges}} == 0) {
                    $end_edges{$this_edge} = 1;
                    last;
                }
                # otherwise, always add the point (duplicate start and end lets us detect polygons later)
                push @{$polyedges[-1]}, $this_node;
                # stop at a branch node or dead end, and remember the edge so we don't backtrack
                if (@{$this_node->{edges}} > 2 || @{$this_node->{edges}} == 1) {
                    $end_edges{$this_edge} = 1;
                    if (@{$this_node->{edges}} > 2) {
                        # what can we do with a branch node
                        # depth attribute?
                        my $depth = 1 + ($start_node->{depth} // 0); #/
                        if (!defined $this_node->{depth}
                            || $this_node->{depth} > $depth) {
                            $this_node->{depth} = $depth;
                        }
                        # Only add branch nodes to the processing list as we
                        # encounter them for the first time, so all paths from
                        # tip nodes get processed before any branch nodes.
                        if ((grep $end_edges{$_}, @{$this_node->{edges}}) == 1) {
                            push @start_nodes, $this_node;
                        }
                    }
                    last;
                }
                my $last_edge = $this_edge;
                # step to next edge
                $this_edge = +(grep $_ != $this_edge, @{$this_node->{edges}})[0];
                if (!$this_edge) {$end_edges{$last_edge} = 1;}
                # stop if we've looped around to start
                if ($this_edge == $start_edge) {
                    $end_edges{$last_edge} = 1;
                    last;
                }
            }
        }
    }

    # Now make the node order tend toward outward, toward the tips,
    # because this makes it easier (for me) to think about the left and 
    # right sides of branches when trying to generate just one side or 
    # the other of a thin polygon feature.
    @$_ = reverse @$_ for @polyedges;

    # make any loops ccw
    foreach my $polyedge (grep @$_ > 2, @polyedges) {
        if ($polyedge->[0] == $polyedge->[-1]) {
            if (!Math::Clipper::orientation([map $_->{point}, @$polyedge])) {
                @$polyedge = reverse @$polyedge;
            }
        }
    }

    # transitional :
    # Convert to the data structure we want to use here.
    # Should do this right after mic_adjust(), or even
    # change Math::Geometry::Delaunay's data structure
    # to something similar.
    
    
    my %dupnodes;
    foreach my $polyedge (@polyedges) {
        my $prevnode;
        foreach my $node (@$polyedge) {
            my @edgeandindex = map [$_, $node != $_->{nodes}->[0] ? 1 : 0 ], @{$node->{edges}};
            if (ref $node eq 'HASH') {
                if (defined $dupnodes{$node}) { $node = $dupnodes{$node}; }
                else {
                    $dupnodes{$node} = Slic3r::MedialAxis::MIC->new(
                        point => Slic3r::Point->new($node->{point}),
                        radius => $node->{radius},
                        edges => $node->{edges},
                        tangents => $node->{tangents},
                        );
                    $node = $dupnodes{$node};
                    my %duptan;
                    foreach my $tangent_pair (@{$node->[3]}) {
                        for (0,1) {
                            if (ref $tangent_pair->[$_] eq 'ARRAY') {
                                if (defined $duptan{$tangent_pair->[$_]}) {
                                    $tangent_pair->[$_] = $duptan{$tangent_pair->[$_]};
                                } else {
                                    $duptan{$tangent_pair->[$_]} = Slic3r::MedialAxis::Tangent->new(x=>$tangent_pair->[$_]->[0],y=>$tangent_pair->[$_]->[1],offset=>0,parent=>$node);
                                    $tangent_pair->[$_] = $duptan{$tangent_pair->[$_]};
                                }
                            }
                        }
                    }
                    foreach my $edge_entry (@edgeandindex) {
                        $edge_entry->[0]->{nodes}->[$edge_entry->[1]] = $node;

                    }
                }
            }
        my $prevnode=$node;
        }
    }

    my %dupedge;
    foreach my $polyedge (@polyedges) {
        foreach my $node (@$polyedge) {
            foreach my $edge (@{$node->[2]}) {
                if (ref $edge eq 'HASH') {
                    if (defined $dupedge{$edge}) {
                        $edge = $dupedge{$edge};
                    } else {
                        $dupedge{$edge} = Slic3r::MedialAxis::Edge->new(@{$edge->{nodes}});
                        $edge = $dupedge{$edge};
                    }
                }
            }
        }
    }


    foreach my $polyedge (@polyedges) {
        for (my $i=1;$i<@$polyedge;$i++){
            if (!$polyedge->[$i-1]->common_edge($polyedge->[$i])) {
                print "no common\n\n";
                exit;
            } 
        }
    }



    if (0) {
    my $t = $polyedges[0]->[0];
    print "node: ",$t,"\n";
    print "radius: ",$t->radius,"\n";
    print "point: ",$t->point," , [",$t->point->[0],", ",$t->point->[1],"]\n";
    print "edges: ",join(',',@{$t->edges}),"\n    [\n",join("]\n    [",map {$_."\n[". join(',',$_->nodes) . "]  == \n["  . $_->[0].','.$_->[1] . "]\n and ". $_->[0]->[0].' == ' . $_->points->[0]} @{$t->edges}),"]\n";
    print "tangents: ",$t->tangents->[0]->[0], " , ",$t->tangents->[0]->[1],"\n";
    print "tangents array: ",join(', ',@{$t->tangents->[0]->[0]}), " and ",join(', ',@{$t->tangents->[0]->[1]}),"\n";
    print "first edge points: ",$t->edges->[0]->points->[0], " , ",$t->edges->[0]->points->[1],"\n";

    print "\n\nsecond\n\n";

    $t = $polyedges[0]->[1];
    print "node: ",$t,"\n";
    print "radius: ",$t->radius,"\n";
    print "point: ",$t->point," , [",$t->point->[0],", ",$t->point->[1],"]\n";
    print "edges: ",join(',',@{$t->edges}),"\n    [\n",join("]\n    [",map {$_."\n[". join(',',$_->nodes) . "]  == \n["  . $_->[0].','.$_->[1] . "]\n and ". $_->[0]->[0].' == ' . $_->points->[0]} @{$t->edges}),"]\n";
    print "tangents: ",$t->tangents->[0]->[0], " , ",$t->tangents->[0]->[1],"\n";
    print "tangents array: ",join(', ',@{$t->tangents->[0]->[0]}), " and ",join(', ',@{$t->tangents->[0]->[1]}),"\n";
    print "first edge points: ",$t->edges->[0]->points->[0], " , ",$t->edges->[0]->points->[1],"\n";

    print "edge match? \n",join(',',@{$polyedges[0]->[0]->edges}),"\n",join(',',@{$polyedges[0]->[1]->edges}),"\n";

    exit;
    }

    combine_polyedges(\@polyedges);

    foreach my $polyedge (@polyedges) {
        for (my $i=1;$i<@$polyedge;$i++){
            if (!$polyedge->[$i-1]->common_edge($polyedge->[$i])) {
                print "ITS CPE\n\n";
                exit;
            } 
        }
    }

    return @polyedges;
}




# Trim medial axis edge chains to a given maximum and minimum MIC radius.
# Avoid going into corners that are too wide.

sub thin_paths_filter {

    my ($medial_paths,
        $start_radius, $min_radius,    # max, min radius to keep
        $offset_radius,                # added to the previous two
        $tool_radius,                  # 
        $theta,                        # MIC tangents-center angle filter threshold
        ) = @_;
    $min_radius //= 0; #/
    $theta //= deg2rad(100); #/
    $tool_radius //= 0; #/
    $offset_radius //= 0; #/

    my @outpaths = ();

    foreach my $path (@$medial_paths) {
        my $previous_test_radius = $start_radius + $offset_radius;
        my $isloop = $path->[0] == $path->[-1];
        # TODO: Need to better analyze just when to insert
        # a new array ref for a new path. Right now we 
        # insert some that don't get used, and weed out 
        # the empties at the end.
        push @outpaths, [];
        for (my $i = 0; $i < @$path; $i++) {
            my $minuswhat = ($isloop && $i == 0 && @$path > 3) ? 2 : 1;
            $minuswhat = 1;

            # Each point needs it's own test radius that accounts for the effect
            # of miter limit on the location of the two corner points where
            # a perimeter stops and turns around at the entry to a thin section.

            my $edge = $path->[$i]->common_edge($path->[$i == $#$path ? $i - 1 : $i + 1]);
            my $test_radius = $offset_radius + 
                              adjusted_radius_for_miter_limit($path->[$i], $edge, $start_radius/2, $miterLimit);

            # Create a set of paths from the original path, where the endpoints
            # are roughly where the medial axis radius crosses the adjusted
            # start_radius threshold. Mark end nodes at the transitions from too
            # big to okay radius with the tosplice indicator, for later splicing
            # into perimiter polygons.

            if ($path->[$i]->radius <= $test_radius && $path->[$i]->radius >= $min_radius + $offset_radius) {
                if ($i == 0 || ($path->[$i - $minuswhat]->radius > $previous_test_radius || $path->[$i - $minuswhat]->radius < $min_radius + $offset_radius)) {
                    push @outpaths, [];
                    if ($i != 0 || $isloop) {
                        # factor calc can probably be simpler
                        my $denom = $path->[$i]->radius/$test_radius - $path->[$i - $minuswhat]->radius/$previous_test_radius;
                        my $factor = $denom == 0 ? 1 :
                                     ((1 - $path->[$i-1]->radius/$previous_test_radius) / $denom);
                        if ($path->[$i - $minuswhat]->radius < ($min_radius + $offset_radius)) {
                            $denom = $path->[$i]->radius - $path->[$i - $minuswhat]->radius;
                            if ($denom == 0) {$factor=1;}
                            else {$factor = (($min_radius + $offset_radius) - $path->[$i - $minuswhat]->radius)/$denom;}
                        }
                        #print "fact:",$factor,"\n";
                        $factor = abs($factor);
                        my $interp_node = $path->[$i - $minuswhat]->interpolate($path->[$i], $factor);
                        remove_edge($path->[$i - $minuswhat], $interp_node);
                        $interp_node->tosplice(1) if $path->[$i - $minuswhat]->radius > $previous_test_radius;
                        push @{$outpaths[-1]}, $interp_node;
                    }
                }
                if (@outpaths==0) {push @outpaths, [];} 
                push @{$outpaths[-1]}, $path->[$i];

            } elsif (($i != 0 || $isloop) && ($path->[$i - $minuswhat]->radius <= $previous_test_radius && $path->[$i - $minuswhat]->radius >= $min_radius + $offset_radius)) {
                # factor calc can probably be simpler
                my $denom = $path->[$i]->radius/$test_radius - $path->[$i - $minuswhat]->radius/$previous_test_radius;
                my $factor = $denom == 0 ? 1 :
                             ((1 - $path->[$i - $minuswhat]->radius/$previous_test_radius) / $denom);
                if ($path->[$i]->radius < ($min_radius + $offset_radius)) {
                    $denom = $path->[$i]->radius - $path->[$i - $minuswhat]->radius;
                    if ($denom == 0) {$factor=1;}
                    else {$factor = (($min_radius + $offset_radius) - $path->[$i - $minuswhat]->radius)/$denom;}
                }
                #print "fact:",$factor,"\n";
                $factor = abs($factor);
                my $interp_node = $path->[$i - $minuswhat]->interpolate($path->[$i], $factor);
                remove_edge($interp_node, $path->[$i]);
                $interp_node->tosplice(1) if $path->[$i]->radius > $test_radius;
                push @{$outpaths[-1]}, $interp_node;
            }
            $previous_test_radius = $test_radius;
        }
    }

    #print "path lengths: ",join(', ',map scalar(@$_), @outpaths),"\n";

    @outpaths = grep @$_, @outpaths;

    # An angle threshold to cull short paths in the medial axis that
    # go into non-narrow corners. (We trust the normal offset-generated
    # paths to handle those areas.)
    # If the angle formed by the MIC center and two tangents is
    # less than the threshold (of say, 90 or 80 degrees), don't go into the corner.
    # Small threshold angles correspond to wide angles we don't want to go into.

    foreach my $path (@outpaths) {
        next if @{$path} < 2;
        my $edge = $path->[0]->common_edge($path->[1]);
        # we may need more conditions on these while()s to avoid
        # overzealous trimming
        while (  @{$path} > 1
               && abs(angle_reduce_pi($path->[1]->central_angle($edge))) <= $theta 
               
              ) {
            remove_edge($path->[0], $path->[1]) if @$path > 1;
            shift @{$path};
            $edge = $path->[0]->common_edge($path->[1]) if @$path > 1;
        }
        next if @{$path} < 2;
        $edge = $path->[-1]->common_edge($path->[-2]) if @$path > 1;
        while ( @{$path} > 1
               && abs(angle_reduce_pi($path->[-2]->central_angle($edge))) <= $theta 
              ) {
            remove_edge($path->[-2], $path->[-1]) if @$path > 1;
            pop @{$path};
            $edge = $path->[-1]->common_edge($path->[-2]) if @$path > 1;
        }
    }

    #print "path lengthspost angle filter: ",join(', ',map scalar(@$_), @outpaths),"\n";

    @outpaths = grep @$_ > 1, @outpaths;

    # Polygons have the same node in the first and last array entries.
    # If that's all that's left, drop it.
    @outpaths = grep @$_ != 2 || $_->[0] != $_->[1], @outpaths;

    # Combine paths with the same end nodes.
    combine_polyedges(\@outpaths);
    # Combine based on ends being really close.
    combine_fragments(\@outpaths, $tool_radius);
    
    # we try to do left-right orientation of offset paths in split() generally
    # considering left or right in relation to leaving the wide-enough zone of
    # the part and entering the too-thin zone. If a fragment has a splice point
    # at just one end, we want it to be at the beginning, which corresponds to the
    # fragment leaving the wide-enough zone. If it's at the end, the left-right 
    # sense will be reversed.
    foreach my $path (@outpaths) {
        @$path = reverse @$path if $path->[-1]->tosplice && !$path->[0]->tosplice;
    }

    return @outpaths;
}


# Generate offset polylines or a polygon from a collection of medial axis 
# edge chains, possibly favoring either the left or right side,
# and adjusting for tool width in narrow corners or passages by omitting one
# side if only one will fit, or centering where not even one will fit.

sub split {
    my ($medial_paths, $split_radius, $offset_radius, $tool_radius, $left_right) = @_;

    # $left_right: which side to favor, or which side to keep when < 2 * $tool_radius
    # Even number for left (clockwise), odd for right (ccw). 
    # Negative to do both, when radius is > 2 * $tool_radius
    $left_right //= 1; #/# right by default - that tends to give ccw loops
    my $both_sides = $left_right < 0;
    $left_right = abs($left_right) % 2;

    my @shiftnodepaths; # the nodes on the prefered side
    my @splitnodepaths; # the other side nodes, if requested

    foreach my $path (@$medial_paths) {
        my @splitnodessets = ([]);
        my @shiftnodes;
        for (my $i = $#$path; $i > -1; $i--) {

            my $newpoint;

            if ($path->[$i]->radius > $split_radius + $offset_radius) {



                my $other_node = $i == $#$path ? $path->[$i-1] : $path->[$i+1];

                if (!$other_node->common_edge($path->[$i])) {
                    # shouldn't happen
                    # find out where the nodes are getting linked the wrong way
                    # or not getting linked
                    print "trouble\n";
                    print "i: $i of ",$#$path,"\n";
                    print " other: ",$other_node," this: ", $path->[$i],"\n";
                    print "poly",join(',',map '['.$_->point->[0].','.$_->point->[1].','.($_->[4]?'yes':'no').']', @$path),"\n";
                    print "each edge",join(',',map '['.join (',',@{$_->edges}).']', @$path),"\n";
                    $DB::svg->appendPolylines({style=>'stroke-width:20000;stroke:blue;fill:none;'},[map $_->point, @$path]);
                    next;
                }

                my $tangent_pair = $path->[$i]->tangent_pair_toward($other_node);
                my $which_tangent = ($i == $#$path) ? (1 - $left_right) : (0 + $left_right);
                my $other_tangent = $which_tangent == 1 ? 0 : 1;

                $newpoint = $tangent_pair->[$which_tangent]->clone;
                $newpoint->offset($tool_radius + $offset_radius);
                # this was related to variable offset: (($path->[$i]->radius - $offset_radius)/2)

                if ($both_sides
                    && (!$tool_radius
                        || ($path->[$i]->radius >=  (2 * $tool_radius + $offset_radius) 
                            && defined $tangent_pair->[$other_tangent])
                       )
                   ) {
                    my $split_point = $tangent_pair->[$other_tangent]->clone;
                    $split_point->offset($tool_radius + $offset_radius);
                    push @{$splitnodessets[-1]}, $split_point;
                                                 
                } elsif (@{$splitnodessets[-1]}) {
                    push @splitnodessets, [];
                }
            }
            push @shiftnodes, $newpoint // $path->[$i]; #/
        }
        push @splitnodepaths, grep @$_, @splitnodessets;
        $path = [(reverse @shiftnodes)]; # we iterated backwards on the path node list
    }

    push @$medial_paths, grep @$_ > 1, @splitnodepaths;
    
    # TODO:

    # If there was any offset, corners are crossed if both sides were done.
    # Try to avoid that by skipping or adjusting nodes when the offset is larger
    # than a node's radius, when doing both sides.

    # Combine paths with common end nodes, or ends with common parent tangents.
    # needs to be fixed to work with both original MIC nodes and tangent nodes.
    #combine_polyedges($medial_paths, 1);

    # combine paths based on really close endpoints
    combine_fragments($medial_paths, $tool_radius);#&Slic3r::Geometry::epsilon);

    # modifying input array
    # but also returning it?
    # make up your mind
    return @$medial_paths;
}


sub splice_into_perimeter {
    my ($loops, $frags, $match_dist, $left_right) = @_;

    if (@$loops == 0) {
        #print "NO LOOPS to splice_into_perimeter(), returning frags\n";
        return ([], [map Slic3r::Polyline->new([map $_->point, @$_]), grep @$_ > 0, @$frags], []);
    }

    combine_fragments($frags, $match_dist);

    my @frags_out = map Slic3r::Polyline->new([map $_->point, @$_]), grep @$_ > 0, grep !$_->[0]->tosplice && !$_->[-1]->tosplice, @$frags;
    my @frags = ((grep $_->[0]->tosplice, @$frags),
                 (map [reverse @$_], grep $_->[-1]->tosplice && !$_->[0]->tosplice, @$frags),
                );

    if (@frags == 0 && @frags_out == 0) {
        #print "NO FRAGS to splice_into_perimeter(), returning original loops\n";
        return ($loops, [], []);
    }

    my @loops = grep @$_ > 0, @$loops;

    # Eliminate short frags.
    # Only do this if you've already merged close frag ends, since sometimes
    # it's the short frag that makes the connection to the perimeter splice point.
    my @too_short_frags;

    for (my $i = $#frags; $i > -1; $i--) {
        my $frag_length = 0;
        my $j=1;
        while ($frag_length < $match_dist && $j < @{$frags[$i]}) {
            $frag_length += Slic3r::Geometry::distance_between_points($frags[$i]->[$j]->point, $frags[$i]->[$j - 1]->point);
            $j++;
        }
        if ($frag_length < $match_dist/2) {
            push @too_short_frags, splice(@frags, $i, 1);
        }
        #print "eliminated short frag [ $frag_length < $match_dist/2 ]\n" if $frag_length < $match_dist;
    }

    #$DB::svg->appendPolylines({style=>'opacity:0.5;stroke-width:50000;stroke-dasharray:70000 70000;stroke:pink;fill:none;'},map [map $_->{point}, @{$_}], @$frags);
    #$DB::svg->appendPolylines({style=>'opacity:0.5;stroke-width:50000;stroke-dasharray:70000 70000;stroke:pink;fill:none;'}, @loops);

    my @nodes;
    my @edges;
    my %spliced_loops;
    for (my $i = $#frags; $i > -1; $i--) {
        next if @{$frags[$i]} == 0;

        # get indeces of closest points on loops to frag splice end points, sorted by distance
        my @inds = map Slic3r::Geometry::nearest_point_index($frags[$i]->[0]->point,$_), @loops;
        @inds = sort {$a->[2] <=> $b->[2]} 
                    map [$_, $inds[$_], distance_between_points(
                                        $loops[$_]->[$inds[$_]],
                                        $frags[$i]->[0]->point)
                        ], (0 .. $#inds);
        my @indsb;
        if ($frags[$i]->[-1]->tosplice) {
            @indsb = map Slic3r::Geometry::nearest_point_index($frags[$i]->[-1]->point,$_), @loops;
            }
        @indsb = sort {$a->[2] <=> $b->[2]}
                    map [$_, $indsb[$_], distance_between_points(
                                          $loops[$_]->[$indsb[$_]],
                                          $frags[$i]->[-1]->point)
                        ], (0 .. $#indsb);

        
        @inds = grep $_->[2] < $match_dist, @inds;
        @indsb = grep $_->[2] < $match_dist, @indsb;
        if (@inds == 0) {
            #print "nothing close enough to front splice point\n";
            #@inds = ();
        }
        if (@indsb == 0 && $frags[$i]->[-1]->tosplice) {
            #print "nothing close enough to back splice point")\n";
            #@indsb = ();
        }
        if (@inds == 0 && @indsb == 0) {
            print "nothing close enough to either splice point\n";
            push @frags_out, Slic3r::Polyline->new([map $_->point, @{$frags[$i]}]);
            #$DB::svg->appendPoints({r=>1000000,style=>'opacity:0.55;fill:yellow;'}, map $_->{point}, grep $_->{tosplice}, @{$frags[$i]});
            next;
        }
        if (@inds > 0 && @indsb> 0 && $inds[0]->[0] == $indsb[0]->[0] && $inds[0]->[1] == $indsb[0]->[1]) {
            # frag with two splice ends has both matching up with  the same point on a loop
            print "frag matches at same point\n";
            # look for a different close point on a different polygon
            # in case this should be a short link between islands
            
            #$DB::svg->appendPolylines({style=>'opacity:0.5;stroke-width:50000;stroke-dasharray:70000 70000;stroke:pink;fill:none;'}, [map $_->{point}, @{$frags[$i]}]);

            #while (@indsb > 0 && $inds[0]->[0] == $indsb[0]->[0]) { shift @indsb; }
            #if (@indsb > 0  && $indsb[0]->[2] > $match_dist) {
                $frags[$i]->[-1]->tosplice(0);
                @indsb = ();
            #}
            #print "   found alternate on different polygon\n" if @indsb > 0;
        }

        $spliced_loops{$loops[$inds[0]->[0]]}++  if @inds  > 0;
        $spliced_loops{$loops[$indsb[0]->[0]]}++ if @indsb > 0;

        # make enough of a node and edge graph structure to splice together
        # loop subsections and fragments later

        push @edges, { nodes => [], 
                       isfrag => 1,
                       frag => $frags[$i] 
                     };

        #$DB::svg->appendPolylines({style=>'opacity:0.5;stroke-width:50000;stroke-dasharray:70000 70000;stroke:red;fill:none;'},[map $_->{point}, @{$frags[$i]}]);
        #$DB::svg->appendPoints({style=>'opacity:0.55;fill:blue;',r=>500000},map $_->{point}, grep $_->{tosplice}, @{$frags[$i]});
        
        if (@inds > 0) {       
            push @nodes, {loop=>$loops[$inds[0]->[0]],
                          index=>$inds[0]->[1],
                          edges=>[$edges[-1]],
                          frag=>$frags[$i],
                          frag_index=>0,
                          };
            $edges[-1]->{nodes}->[0] = $nodes[-1];
        }
        if (@indsb > 0) {
            push @nodes, {loop=>$loops[$indsb[0]->[0]],
                          index=>$indsb[0]->[1],
                          edges=>[$edges[-1]],
                          frag=>$frags[$i],
                          frag_index=>$#{$frags[$i]},
                          };
            $edges[-1]->{nodes}->[1] = $nodes[-1];
        }
    }
 
    for my $i (0 .. $#loops) {
        my @loop_node_sort = sort {$a->{index} <=> $b->{index}} grep $_->{loop} == $loops[$i], @nodes;
        for (my $j = 0; $j < @loop_node_sort; $j++) {
            push @edges, {nodes=>[$loop_node_sort[$j-1],$loop_node_sort[$j]]};
            $loop_node_sort[ $j ]->{edges}->[1] = $edges[-1];
            $loop_node_sort[$j-1]->{edges}->[2] = $edges[-1];
        }
    }


    # accounting
    # Want to make sure all original loop points get returned in spliced paths
    # or unmodified loops
    if (1) {
        my $origloopsum=0;
        map {$origloopsum += scalar(@$_)} @$loops;
        my $loopedgesum=0;
        foreach my $edge (grep !$_->{isfrag}, @edges) {            
            if ($edge->{nodes}->[0]->{index} >= $edge->{nodes}->[1]->{index}) {
                $loopedgesum += @{$edge->{nodes}->[0]->{loop}} - $edge->{nodes}->[0]->{index};
                $loopedgesum += $edge->{nodes}->[1]->{index};
            } else { $loopedgesum += $edge->{nodes}->[1]->{index} - $edge->{nodes}->[0]->{index}; }
        }
        my $untouchedloopsum=0;
        map {$untouchedloopsum += scalar(@$_)} grep !$spliced_loops{$_}, @loops;

        if ($origloopsum != ($loopedgesum + $untouchedloopsum)) {
            print  "origloopsum != (loopedgesum + untouchedloopsum) : $origloopsum != ($loopedgesum + $untouchedloopsum)\n";
        }

        my $fragedgecount = scalar(grep $_->{isfrag}, @edges);
        my $origfragcount = scalar(@$frags);
        my $filteredfragcount = scalar(@frags);
        my $skipfragcount = @frags_out;
        my $tooshortcount = @too_short_frags;
        if ($skipfragcount + $fragedgecount != $filteredfragcount + $tooshortcount) {
            print "frag counts wrong:  $skipfragcount + $fragedgecount != $filteredfragcount + $tooshortcount (from original $origfragcount)\n"; 
        }
    }
    
    # Once you've got this straightend out, flip all the uses
    # below, instead of flipping the flag, and work up
    # the visual doc of this and how it relates to anything
    # upstream.
    $left_right = !$left_right;

    # Start point candidates for the graph walk.
    # Everything but frag edges that have splice points at both ends - we
    # expect to get on and over those from loop edges.
    my @start_nodes = map $_->{nodes}->[$left_right ? 1 : 0], 
                      grep { !($_->{isfrag}
                               && defined $_->{nodes}->[0] 
                               && defined $_->{nodes}->[1]
                              )
                           } @edges;

    # Walk the cross referenced nodes and edges to build up polyline-like node lists.
    my @polyedges = ();

    foreach my $start_node (@start_nodes) {
        my $start_edge = $start_node->{edges}->[ $left_right ? 1 : 2 ];
        next if $start_edge->{stop};
        my $this_node = $start_node;
        push @polyedges, [];
        push @{$polyedges[-1]}, [$this_node, $start_edge->{isfrag}];
        my $this_edge = $start_edge;
        # Step along nodes: The next node is the node on the current edge 
        # that isn't this one, unless we happen to be on a loop with only
        # one splice point.
        
        while ($this_node = +(grep {($_ != $this_node)
                                # Special case: A loop with one splice point,
                                # which becomes an edge with the same node at each end.
                                || (!$this_edge->{isfrag} && $this_node->{edges}->[1] == $this_node->{edges}->[2])
                              } @{$this_edge->{nodes}})[0]) {
            if ($this_edge->{stop}) {
                last;
                }
            $this_edge->{stop} = 1;
            # always add the point (duplicate start and end lets us detect polygons later)
            push @{$polyedges[-1]}, [$this_node, $this_edge->{isfrag}];
            $this_edge->{spliced} = 1;
            # step to next edge
            if ($this_edge->{isfrag}) {
                # We only get here if we started on a frag
                # or got onto a frag with splice points on both ends.
                $this_edge = $this_node->{edges}->[$left_right ? 1 : 2];
                $this_node->{edges}->[$left_right ? 2 : 1]->{stop} = 1;
            } else {
                # We've just completed a loop edge, and will now step
                # onto the frag edge at this node.
                my $frag_end_nodes_count = grep defined $_, @{$this_node->{edges}->[0]->{nodes}};
                if ($frag_end_nodes_count == 1) {
                    # If the frag edge at this node has just one splice point
                    # end the path with this node, flagged as a frag.
                    $this_node->{edges}->[0]->{stop} = 1;
                    push @{$polyedges[-1]}, [$this_node, 1];
                    last;
                } elsif ($frag_end_nodes_count == 2) {
                    # If the frag has splice points on both ends
                    # make it the next edge to follow.
                    $this_edge = $this_node->{edges}->[0];
                }
            }
            # stop if we've looped around to start
            if ($this_edge == $start_edge) {
                last;
            }
        }
    }
  
    my @spliced_paths;
    my @untouched = grep !$spliced_loops{$_}, @loops;
    my @unspliced;

    # Edges that were not spliced to frags : Make Polylines and trim ends.
    foreach my $edge (grep !$_->{spliced} && !$_->{isfrag}, @edges) {
        my $loop = $edge->{nodes}->[0]->{loop};
        my ($start, $end) = ($edge->{nodes}->[0]->{index}, $edge->{nodes}->[1]->{index});
        my @seq;
        if ($start <= $end) { @seq = $start .. $end; }
        else { @seq = -(@$loop - $start) .. $end; }
        if (@seq > 1) {
            my $p = Slic3r::Polyline->new(@$loop[@seq]);
            $p->clip_end($match_dist);
            $p->reverse;
            $p->clip_end($match_dist);
            $p->reverse;
            push @unspliced, $p if @$p > 1, 
        }
    }
    
    # Turn polyedges splice info lists into Polylines and Polygons.
    foreach my $pe (@polyedges) {
        my @pts;
        for (my $i = 1; $i < @$pe; $i++) {
            if (! $pe->[$i]->[1]) {
                # loop sections
                my ($start, $end) = ($pe->[$i - 1]->[0]->{index}, $pe->[$i]->[0]->{index});
                ($start, $end) = ($end, $start) if $left_right;
                my @seq;
                if ($start < $end) { @seq = $start .. $end; } 
                else { @seq = ($start .. $#{$pe->[$i]->[0]->{loop}} , 0 .. $end); }
                @seq = reverse @seq if $left_right;
                push @pts, @{$pe->[$i]->[0]->{loop}}[@seq];
            }
            else {
                # frag sections
                my @seq = 0 .. $#{$pe->[$i]->[0]->{frag}};
                if ($pe->[$i - 1]->[0]->{frag_index} != 0) {
                    @seq = reverse @seq;
                }
                push @pts, map $_->point, @{$pe->[$i]->[0]->{frag}}[@seq];
            }
        }
        if (@pts > 1) {
            if ($pe->[0]->[0] == $pe->[-1]->[0] # same start and end node
                    # and doesn't look like a single-splice loop
                && !(@$pe > 2 && $pe->[0]->[0] == $pe->[1]->[0] 
                              && $pe->[0]->[0] == $pe->[2]->[0]) 
               ) {
                pop @pts;
                push @spliced_paths, Slic3r::Polygon->new(@pts);
            }
            else {
                my $p = Slic3r::Polyline->new(@pts);
                $p->clip_end($match_dist) if !$pe->[-1]->[1];
                if (!$pe->[0]->[1]) {
                    $p->reverse;
                    $p->clip_end($match_dist);
                    $p->reverse;
                    }
                push @spliced_paths, $p if @$p > 1;
            }
        }
    }

    return ([@untouched], [@frags_out], [@spliced_paths, @unspliced]);
}


# adjusted_radius_for_miter_limit()
#
# At the transition into a thin passage or corner, a Clipper offset polygon
# typically has a two-point cusp where it turns around, where the passage is too thin.
# We want medial axis fragments that start right at one of those points for a clean
# splice. The offset polygons typically turn around when the passage gets down to
# two extrusion widths - but you can't just look for a medial axis radius of
# one extrusion width to find the splice point for the polygon corner. The
# corner splice point will correspond to a slightly smaller radius which can be
# derived from the miter limit used to generate the Clipper offset, and the
# formed by the walls heading into the corner or thin passage. Here we use the
# angle formed by the two MIC radii of a medial axis node.
# http://sheldrake.net/3D_printing/miterAdjRadius/

sub acos { atan2( sqrt(1 - $_[0] * $_[0]), $_[0] ) }

sub adjusted_radius_for_miter_limit {
    my ($node, $edge, $ref_radius, $miter_limit) = @_;

    my $radius;
    
    my $split_theta;
    if (ref $node eq 'HASH') { # transitional as we work out final data structure
        $split_theta = abs((angle3points($node->{point}, @{$node->{tangents}->[0]})));
    } else {
        #$split_theta = abs((angle3points($node->point, @{$node->tangents->[0]})));
        $split_theta = abs(($node->central_angle($edge)));
    }

    $split_theta = PI * 2 - $split_theta if $split_theta > PI;

    my $miter_threshold_theta = 2 * acos(1 / $miter_limit);

    if ($split_theta <= $miter_threshold_theta) {
        #$radius = $ref_radius * 2;
        $radius = $ref_radius;
    } else {
        my $miter_dist = $ref_radius * $miter_limit;
        # The inner portion of the test radius, accounting for miter limit and 
        # the path overlapping itself in a corner,

        $radius = (2 * $ref_radius * (1 - $miter_limit * cos($split_theta/2))) / (1 - cos($split_theta));

        # plus the outer half of the radius, which is just half extrusion width.

        $radius += $ref_radius;
    }

    return $radius;
}

sub remove_edge {
    my ($node1, $node2) = @_;

    return if $node1 == $node2;
    # Remove edge (and associated tangent pairs) between two nodes.
    # Don't remove the last tangent pair if all edges are removed.
    # There may be some use for an isolated node with tangent points.
    foreach my $this_ind (0 .. $#{$node1->edges}) {
        my $other_ind = +(grep $node1->edges->[$this_ind] == $node2->edges->[$_], (0 .. $#{$node2->edges}))[0];
        if (defined $other_ind) {
            splice @{$node1->edges}, $this_ind, 1;
            splice @{$node1->tangents}, $this_ind, 1 if @{$node1->edges} > 0;
            splice @{$node2->edges}, $other_ind, 1;
            splice @{$node2->tangents}, $other_ind, 1 if @{$node2->edges} > 0;
            last;
        }
    }
}

sub link_nodes {
    my ($node1, $node2) = @_;

    return if $node1->common_edge($node2);

    my $link_edge = Slic3r::MedialAxis::Edge->new($node1, $node2);
    push @{$node1->edges}, $link_edge;
    push @{$node2->edges}, $link_edge;
    
    # now get tangent pairs in sync with the new edge
    if (defined $node1->tangents && defined $node1->tangents->[0]) {
        if (@{$node1->edges} == 1
            && !Math::Geometry::Delaunay::counterclockwise
                ($node1->point, $node2->point, $node1->tangents->[0]->[0])) {
            @{$node1->tangents->[0]} = reverse @{$node1->tangents->[0]};
        } elsif (@{$node1->edges} - 1 == @{$node1->tangents}) {
            push @{$node1->tangents}, [reverse @{$node1->tangents->[0]}];
        }
    }

    if (defined $node2->tangents && defined $node2->tangents->[0]) {
        if (@{$node2->edges} == 1
            && !Math::Geometry::Delaunay::counterclockwise
                ($node1->point, $node2->point, $node2->tangents->[0]->[0])) {
            @{$node2->tangents->[0]} = reverse @{$node2->tangents->[0]};
        } elsif (@{$node2->edges} - 1 == @{$node2->tangents}) {
            push @{$node2->tangents}, [reverse @{$node2->tangents->[0]}];
        }
    }
    
    $node1->tosplice(0);
    $node2->tosplice(0);
}

# combine based on end distance
sub combine_fragments {
    my ($frags, $match_dist) = @_;
    
    for (my $i = $#$frags; $i > -1; $i--) {
        my $this = $frags->[$i];
        for (my $j = 0; $j < $i; $j++) {
            my $other = $frags->[$j];
            if      (   abs($this->[0]->point->[0] - $other->[0]->point->[0]) < $match_dist
                     && abs($this->[0]->point->[1] - $other->[0]->point->[1]) < $match_dist
                     && !$this->[0]->tosplice
                     && !$other->[0]->tosplice
                    ) {
                #print "STARTS MATCH\n";
                my @new_this =  reverse @{splice @$frags, $i, 1};
                link_nodes($new_this[-1], $other->[0]) if $new_this[-1]->isa('Slic3r::MedialAxis::MIC');
                unshift @{$other}, @new_this;
                last;
            } elsif (   abs($this->[-1]->point->[0] - $other->[-1]->point->[0]) < $match_dist
                     && abs($this->[-1]->point->[1] - $other->[-1]->point->[1]) < $match_dist
                     && !$this->[-1]->tosplice
                     && !$other->[-1]->tosplice
                    ) {
                #print "ENDS MATCH\n";
                my @new_this =  reverse @{splice @$frags, $i, 1};
                link_nodes($other->[-1], $new_this[0]) if $new_this[-1]->isa('Slic3r::MedialAxis::MIC');
                push @{$other}, @new_this;
                last;
            } elsif (   abs($this->[-1]->point->[0] - $other->[0]->point->[0]) < $match_dist
                     && abs($this->[-1]->point->[1] - $other->[0]->point->[1]) < $match_dist
                     && !$this->[-1]->tosplice
                     && !$other->[0]->tosplice
                    ) {
                #print "FORWARD MATCH\n";
                my @new_this = @{splice @$frags, $i, 1};
                link_nodes($new_this[-1], $other->[0]) if $new_this[-1]->isa('Slic3r::MedialAxis::MIC');
                unshift @{$other}, @new_this;
                last;
            } elsif (   abs($this->[0]->point->[0] - $other->[-1]->point->[0]) < $match_dist
                     && abs($this->[0]->point->[1] - $other->[-1]->point->[1]) < $match_dist
                     && !$this->[0]->tosplice
                     && !$other->[-1]->tosplice
                    ) {
                #print "BACKWARD MATCH\n";
                my @new_this = @{splice @$frags, $i, 1};
                link_nodes($other->[-1], $new_this[0]) if $new_this[-1]->isa('Slic3r::MedialAxis::MIC');
                push @{$other}, @new_this;
                last;
            }
        }
    }
}


# Combine based on identical node refs at ends.
# Meant to work on MIC nodes and Tangent nodes (Tangents linked parents are identical).
# TODO: When linking two paths at a junction of three paths, trim back the
#       the third path that isn't getting linked.

sub combine_polyedges {
    my ($polyedges, $by_parent) = @_;

    # Sort by length, where array length is rough proxy for edge length sum.
    # Intent is to encourage attaching short paths to longer paths at junctions.
    @$polyedges = sort {@{$a} <=> @{$b}} @$polyedges;

    # Link polyedges with common end points.
    for (my $i = $#$polyedges; $i > -1; $i--) {
        # polygons
        next if ($polyedges->[$i]->[0] == $polyedges->[$i]->[-1]);
        # polylines
        my $this  = $polyedges->[$i];
        for (my $j = 0; $j < $i ; $j++) {
            my $other = $polyedges->[$j];
            next if ($other->[0] == $other->[-1]);
            # all the cases of ends matching up
            if ($this->[$#$this] == $other->[0] || ($by_parent && $this->[$#$this]->parent == $other->[0]->parent)) {
                shift @{$other};
                link_nodes($polyedges->[$i]->[-1], $other->[0]) if !$this->[$#$this]->common_edge($other->[0]);
                @{$other} = (@{splice(@$polyedges, $i, 1)}, @{$other});
                last;
            } elsif ($this->[0] == $other->[$#$other] || ($by_parent && $this->[0]->parent == $other->[$#$other]->parent)) {
                shift @{$this};
                link_nodes($other->[-1], $polyedges->[$i]->[0]) if !$this->[0]->common_edge($other->[$#$other]);
                @{$other} = (@{$other}, @{splice(@$polyedges, $i, 1)});
                last;
            } elsif ($this->[0] == $other->[0] || ($by_parent && $this->[0]->parent == $other->[0]->parent)) {
                shift @{$this};
                link_nodes($other->[0], $polyedges->[$i]->[0]) if !$this->[0]->common_edge($other->[0]);
                @{$other} = ((reverse @{$other}), @{splice(@$polyedges, $i, 1)});
                last;
            } elsif ($this->[$#$this] == $other->[$#$other] || ($by_parent && $this->[$#$this]->parent == $other->[$#$other]->parent)) {
                pop @{$other};
                link_nodes($polyedges->[$i]->[-1], $other->[-1]) if !$this->[$#$this]->common_edge($other->[$#$other]);
                @{$other} = (@{splice(@$polyedges, $i ,1)}, (reverse @{$other}));
                last;
            }
        }
    }
}

sub angle_reduce_pi {
    my $a = shift;
    while ($a >   &PI) { $a -= &PI * 2; }
    while ($a <= -&PI) { $a += &PI * 2; }
    return $a;
}

package Slic3r::MedialAxis::MIC;

use Slic3r::Geometry qw(X Y);

use constant MA_POINT     => 0;
use constant MA_RADIUS    => 1;
use constant MA_EDGES     => 2;
use constant MA_TANGENTS  => 3;
use constant MA_TO_SPLICE => 4;

sub new {
    my $class = shift;
    my %args = @_;
    
    my $self = [
        map delete $args{$_}, qw(point radius edges tangents attribute),
    ];
    
    bless $self, $class;
    $self;
}

sub clone {
    my $self = shift;
    my $clone =  (ref $self)->new();
    @{$clone} = ($self->[MA_POINT]->clone, $self->[MA_RADIUS], [], [map [map $_->clone, @$_], @{$self->[MA_TANGENTS]}], 0);
    return $clone;
}

sub point {
    return $_[0]->[MA_POINT];
}

sub radius {
    return $_[0]->[MA_RADIUS];
}

sub tosplice {
    $_[0]->[MA_TO_SPLICE] = ($_[1] ? 1 : 0) if defined $_[1];
    return $_[0]->[MA_TO_SPLICE];
}

sub edges {
    return $_[0]->[MA_EDGES];
}

sub tangents {
    return $_[0]->[MA_TANGENTS];
}

sub tangent_pair_toward {
    my ($self, $other) = @_;
    my $edge = $self->common_edge($other);
    return $edge->tangent_pair_from_node($self);
}

sub edges_ahead { 
    my ($self, $edge) = @_;

    my @other_edges = grep $_ != $edge, @{$self->[MA_EDGES]};

    return @other_edges if @other_edges < 2;
    
    # If there are two other edges, always return in the same order:
    # the left and then the right, relative to the provided edge coming
    # into the node.

    my $self_index  = +(grep $self->[MA_EDGES]->[$_] == $edge, (0, 1, 2))[0];
    my $left_index  = ($self_index + 1) % scalar(@{$self->[MA_EDGES]});
    my $right_index = ($self_index + 2) % scalar(@{$self->[MA_EDGES]});
    my $p1 = +(grep $_ != $self, $self->[MA_EDGES]->[$self_index]->nodes)[0];
    my $p3 = +(grep $_ != $self, $self->[MA_EDGES]->[$left_index]->nodes)[0];
    my $p4 = +(grep $_ != $self, $self->[MA_EDGES]->[$right_index]->nodes)[0];

    # Choose the edge that makes a larger angle with the processed edge.
    # Works because angle3points() converts negative angles to positive.

    if (  angle3points($self->point,$p1->point,$p3->point)
        < angle3points($self->point,$p1->point,$p4->point)
       ) {
        ($left_index, $right_index) = ($right_index, $left_index);
    }

    return [$self->[MA_EDGES]->[$left_index], $self->[MA_EDGES]->[$right_index]];
}

sub nodes_ahead { 
    my ($self, $edge) = @_;
    return [grep $_ != $self, map $_->nodes, $self->edges_ahead($edge)];
}

sub central_angle {
    my ($self, $edge) = @_;

    # angle formed by two tangents and center point
    # Tangent points corresponding to provided edge should be ordered
    # left, right, heading out on the edge from the MIC center.
    # This depends on Math::Geometry::Delaunay getting that right,
    # and then tangents and edges being kept in sync through processes
    # that remove or add edges.

    my $tangents_index = +(grep $self->[MA_EDGES]->[$_] == $edge, (0 .. $#{$self->[MA_EDGES]}))[0];
    my $angle = atan2($self->[MA_TANGENTS]->[$tangents_index]->[0]->[X] - $self->[MA_POINT]->[X], $self->[MA_TANGENTS]->[$tangents_index]->[0]->[Y] - $self->[MA_POINT]->[Y])
              - atan2($self->[MA_TANGENTS]->[$tangents_index]->[1]->[X] - $self->[MA_POINT]->[X], $self->[MA_TANGENTS]->[$tangents_index]->[1]->[Y] - $self->[MA_POINT]->[Y]);
    return abs($angle);
}

sub is_fork { return @{$_[0]->[MA_EDGES]} == 3; }

sub common_edge {
    my ($self, $other) = @_;
    my $other_index;
    foreach my $this_index (0 .. $#{$self->[MA_EDGES]}) {
        $other_index = +(grep $self->[MA_EDGES]->[$this_index] == $other->[MA_EDGES]->[$_], (0 .. $#{$other->[MA_EDGES]}))[0];
        last if defined $other_index;
    }

    if (!defined $other_index) {
        return undef;
    } else {
        return $other->edges->[$other_index];
    }
}

sub interpolate {
    my ($this_node, $other_node, $factor) = @_;

    my $new_node = $this_node->clone;

    # debug
    my $intersvg;
    if ($DB::svgfilename) {
        my $fn = $DB::svgfilename;
        my $u = $this_node+0;
        $fn =~ s/\.svg/_${u}.svg/;
        #$intersvg = SVGAppend->new($fn, {style=>'background-color:#000000;'});
        $intersvg = $DB::svg;
    }
    if ($DB::dbinterp) { # debug flag to set in various places in Region
        $intersvg->appendRaw('<g>');
        $intersvg->appendPoints({r=>50000,style=>'fill:white;'},$this_node->point,$other_node->point);
    }

    # interpolate point

    @{$new_node->[MA_POINT]} = ($this_node->point->[0] + $factor * ($other_node->point->[0] - $this_node->point->[0]),
                                $this_node->point->[1] + $factor * ($other_node->point->[1] - $this_node->point->[1]));
    if ($DB::dbinterp) {
        $intersvg->appendPoints({r=>50000,style=>'fill:gray;'},$new_node->[MA_POINT]);
        $intersvg->appendRaw('<text style="font-size:50000;fill:white;" x="'.($new_node->[MA_POINT]->[0] - 150000).'" y="'.($new_node->[MA_POINT]->[1] - 25000).'">'.sprintf("%.2f",$factor)."</text>\n");
    }
    
    # interpolate radius

    $new_node->[MA_RADIUS] = $this_node->radius + $factor * ($other_node->radius - $this_node->radius);

    # interpolate tangent point pair, if the nodes are linked by an edge

    my $edge = $this_node->common_edge($other_node);

    if ($edge) {

        my @this_tangent_pair = @{$edge->tangent_pair_from_node($this_node)};
        my @other_tangent_pair = reverse @{$edge->tangent_pair_from_node($other_node)};

        # tangent x, y coords
        $new_node->[MA_TANGENTS]->[0]->[1]->[0] = $this_tangent_pair[0]->[0] + $factor * ($other_tangent_pair[0]->[0] - $this_tangent_pair[0]->[0]);
        $new_node->[MA_TANGENTS]->[0]->[1]->[1] = $this_tangent_pair[0]->[1] + $factor * ($other_tangent_pair[0]->[1] - $this_tangent_pair[0]->[1]);
        $new_node->[MA_TANGENTS]->[0]->[0]->[0] = $this_tangent_pair[1]->[0] + $factor * ($other_tangent_pair[1]->[0] - $this_tangent_pair[1]->[0]);
        $new_node->[MA_TANGENTS]->[0]->[0]->[1] = $this_tangent_pair[1]->[1] + $factor * ($other_tangent_pair[1]->[1] - $this_tangent_pair[1]->[1]);
        @{$new_node->[MA_TANGENTS]->[1]} = reverse @{$new_node->[MA_TANGENTS]->[0]};

        # tangent offset
        $new_node->[MA_TANGENTS]->[0]->[1]->[2] = $this_tangent_pair[0]->[2] + $factor * ($other_tangent_pair[0]->[2] - $this_tangent_pair[0]->[2]);
        $new_node->[MA_TANGENTS]->[0]->[0]->[2] = $this_tangent_pair[1]->[2] + $factor * ($other_tangent_pair[1]->[2] - $this_tangent_pair[1]->[2]);

        # assign parent
        $new_node->[MA_TANGENTS]->[0]->[0]->[3] = $new_node;
        $new_node->[MA_TANGENTS]->[0]->[1]->[3] = $new_node;
        $new_node->[MA_TANGENTS]->[1]->[0]->[3] = $new_node;
        $new_node->[MA_TANGENTS]->[1]->[1]->[3] = $new_node;

        if ($DB::dbinterp) {
            $intersvg->appendPoints({r=>50000,style=>'fill:red;'},$this_tangent_pair[0]->point,$other_tangent_pair[0]->point);
            $intersvg->appendPoints({r=>50000,style=>'fill:green;'},$this_tangent_pair[1]->point,$other_tangent_pair[1]->point);
            $intersvg->appendPoints({r=>50000,style=>'fill:pink;'},$new_node->[MA_TANGENTS]->[0]->[0]->point);
            $intersvg->appendPoints({r=>50000,style=>'fill:lightgreen;'},$new_node->[MA_TANGENTS]->[0]->[1]->point);
            $intersvg->appendRaw('</g>');
        }

        # set up new edge links
        Slic3r::MedialAxis::remove_edge($this_node, $other_node);
        my $new_edge1 = Slic3r::MedialAxis::Edge->new($this_node, $new_node);
        my $new_edge2 = Slic3r::MedialAxis::Edge->new($new_node, $other_node);
        push @{$this_node->[MA_EDGES]}, $new_edge1;
        push @{$this_node->[MA_TANGENTS]}, \@this_tangent_pair;
        push @{$other_node->[MA_EDGES]}, $new_edge2;
        push @{$other_node->[MA_TANGENTS]}, \@other_tangent_pair;
        $new_node->[MA_EDGES] = [$new_edge1, $new_edge2];
    }

    return $new_node;
}

package Slic3r::MedialAxis::Edge;

use Slic3r::Geometry qw(A B X Y);

sub new {
    my $class = shift;
    my $self;
    $self = [ @_ ];
    bless $self, $class;
    return $self;
}

sub nodes {
    return @{$_[0]};
}

sub points {
    #          MA_POINT
    return [map $_->[0], @{$_[0]}];
}

sub next_node {
    my ($self, $node) = @_;
    return $node != $self->[0] ? $self->[0] : $self->[1];
}

sub tangent_pair_from_node {
    my ($self, $node) = @_;
    my $tangents_index = +(grep {$node->edges->[$_] == $self} (0 .. $#{$node->edges}))[0];
    return $node->tangents->[$tangents_index];
}

package Slic3r::MedialAxis::Tangent;

use constant MAT_X      => 0;
use constant MAT_Y      => 1;
use constant MAT_OFFSET => 2;
use constant MAT_PARENT => 3;

sub new {
    my $class = shift;
    my %args = @_;
    
    my $self = [
            map delete $args{$_}, qw(x y offset parent),
        ];
        
    bless $self, $class;
    $self;
}

sub clone {
    my ($self) = @_;
    my $clone = (ref $self)->new(x=>$self->[MAT_X],y=>$self->[MAT_Y],offset=>$self->[MAT_OFFSET],parent=>$self->[MAT_PARENT]);
    return $clone;
    }

sub point {
    my ($self, $offset) = @_;
    if ($_[0]->[MAT_OFFSET]) {
        return $self->offset_point;
    } else {
        return [$self->[0], $self->[1]];
    }
}

sub radius { return $_[0]->[MAT_PARENT]->radius - $_[0]->[MAT_OFFSET]; }

sub parent { return $_[0]->[MAT_PARENT]; }

sub tosplice { return $_[0]->[MAT_PARENT]->tosplice; }

sub offset {
    my ($self, $offset) = @_;
    $self->[MAT_OFFSET] = $offset;
}

sub offset_point { 
    my ($self, $offset) = @_;

    $offset //= $self->[MAT_OFFSET]; #/

    my $a = CORE::atan2($self->[MAT_PARENT]->point->[1] - $self->[MAT_Y], 
                        $self->[MAT_PARENT]->point->[0] - $self->[MAT_X]);

    return Slic3r::Point->new($self->[MAT_X] + $offset * CORE::cos($a),
                              $self->[MAT_Y] + $offset * CORE::sin($a));
}

1;
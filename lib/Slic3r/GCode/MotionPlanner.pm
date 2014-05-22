package Slic3r::GCode::MotionPlanner;
use Moo;

has 'islands'           => (is => 'ro', required => 1);  # arrayref of ExPolygons
has 'internal'          => (is => 'ro', default => sub { 1 });
has '_space'            => (is => 'ro', default => sub { Slic3r::ConfSpace->new });
has '_inner'            => (is => 'ro', default => sub { [] });  # arrayref of ExPolygons
has '_outer'            => (is => 'ro', default => sub { [] });  # arrayref of ExPolygons
has '_contour'          => (is => 'ro', default => sub { [] });  # arrayref of ExPolygons

use List::Util qw(first max);
use Slic3r::Geometry qw(A B scale unscale epsilon scaled_epsilon);
use Slic3r::Geometry::Clipper qw(offset offset_ex diff diff_ex diff_pl intersection_pl union_ex);

# clearance (in mm) from the perimeters
has '_inner_margin' => (is => 'ro', default => sub { scale .3 });
has '_outer_margin' => (is => 'ro', default => sub { scale 2 });

# this factor weigths the crossing of a perimeter 
# vs. the alternative path. a value of 5 means that
# a perimeter will be crossed if the alternative path
# is >= 5x the length of the straight line we could
# follow if we decided to cross the perimeter.
# a nearly-infinite value for this will only permit
# perimeter crossing when there's no alternative path.
use constant CROSSING_PENALTY => 200;

use constant POINT_DISTANCE => 1;  # unscaled

# setup our configuration space
sub BUILD {
    my $self = shift;
    
    my $point_distance = scale POINT_DISTANCE;

 #   $DB::single=1;

    # create outside space
    my $grown_islands=offset( [map @$_, @{$self->islands}], +($self->_outer_margin - scale .1));
    
    my $bb=Slic3r::Geometry::BoundingBox->new_from_points([ map @$_, @$grown_islands]);
    my $ofs=scale 10;
    
    my $outside=diff_ex(
        [Slic3r::Polygon->new([$bb->x_min-$ofs,$bb->y_min-$ofs], [$bb->x_min-$ofs,$bb->y_max+$ofs], 
                              [$bb->x_max+$ofs,$bb->y_max+$ofs], [$bb->x_max+$ofs,$bb->y_min-$ofs])],
        $grown_islands
        );

    my @inner_spaces;


    for my $i (0 .. $#{$self->islands}) {
        my $expolygon = $self->islands->[$i];

        # find external margin
        my $outer = offset([ @$expolygon ], $self->_outer_margin);
        my @outer_points = map @{$_->equally_spaced_points($point_distance)}, @$outer;

        $self->_space->insert_point($_) for @outer_points;

        my $inner = offset_ex([ @$expolygon ], -$self->_inner_margin);
        my $inner_mask = offset_ex([ @$expolygon ], -($self->_inner_margin - scale .1));
        push @{ $self->_inner }, @$inner;
        my @inner_points = map @{$_->equally_spaced_points($point_distance)}, map @$_, @$inner;

        $self->_space->insert_point($_) for @inner_points;

        my $contour = offset_ex(
            diff(
                $outer,
                [ map @$_, @$inner ],
            ),
            scale -0.1);
        push @{ $self->_contour }, @$contour;
    }
    # build grid
    
    for (my $x = $bb->x_min-$ofs ; $x <= $bb->x_max+$ofs; $x+=scale 2) {
        for (my $y = $bb->y_min-$ofs ; $y <= $bb->y_max+$ofs; $y+=scale 2) {
            $self->_space->insert_point(Slic3r::Point->new($x,$y+unscale($x)));
        }
    }

    my @lines;
    for my $p ( @{$self->_space->points} ) {
        for my $p2 (@{$self->_space->points_in_range($p, scale 5)}) {
            push @lines, Slic3r::Polyline->new($p,$p2);
        }
    }
    
    $DB::single=1;

    sub min ($$) { $_[$_[0] > $_[1]] }

    for(my $c=0 ; $c<=$#lines; $c+=1000) {
        my @slice=@lines[$c .. min($#lines, $c+999)];
        my $open=diff_pl(\@slice, [ map @$_, @{$self->_contour} ]);
        for my $l (@$open) {
            $self->_space->insert_edge($l->first_point, $l->last_point, $l->length);
        }
        my $blocked=intersection_pl(\@slice, [ map @$_, @{$self->_contour} ]);
        for my $l (@$blocked) {
            $self->_space->insert_edge($l->first_point, $l->last_point, $l->length * CROSSING_PENALTY);
        }       
    }
    if(0) {
    my $open=diff_pl(\@lines, [ map @$_, @{$self->_contour} ]);
    my $blocked=intersection_pl(\@lines, [ map @$_, @{$self->_contour} ]);
    for my $l (@$open) {
        $self->_space->insert_edge($l->first_point, $l->last_point, $l->length);
    }
    for my $l (@$blocked) {
        $self->_space->insert_edge($l->first_point, $l->last_point, $l->length * CROSSING_PENALTY);
    }
    }
    if(0) {
    # process individual islands
    for my $i (0 .. $#{$self->islands}) {
        my $expolygon = $self->islands->[$i];
            
        # find external margin
        my $outer = offset([ @$expolygon ], $self->_outer_margin);
        my @outer_points = map @{$_->equally_spaced_points($point_distance)}, @$outer;
        
        # find pairs of visible outer points and add them to the graph
        for my $i (0 .. $#outer_points) {
            for my $j (($i+1) .. $#outer_points) {
                my ($a, $b) = ($outer_points[$i], $outer_points[$j]);
                my $line = Slic3r::Line->new($a, $b);
                # outer points are visible when their line has empty intersection with islands
                if (first { $_->contains_line($line) } @$outside) {
                    $self->_space->insert_edge($line->a, $line->b, $line->length);
                }
            }
        }
        
        if ($self->internal) {
            # find internal margin
            my $inner = offset_ex([ @$expolygon ], -$self->_inner_margin);
            my $inner_mask = offset_ex([ @$expolygon ], -($self->_inner_margin - scale .1));
            push @{ $self->_inner }, @$inner;
            my @inner_points = map @{$_->equally_spaced_points($point_distance)}, map @$_, @$inner;
            
            # find pairs of visible inner points and add them to the graph
            for my $i (0 .. $#inner_points) {
                for my $j (($i+1) .. $#inner_points) {
                    my ($a, $b) = ($inner_points[$i], $inner_points[$j]);
                    my $line = Slic3r::Line->new($a, $b);
                    # turn $inner into an ExPolygonCollection and use $inner->contains_line()
                    if (first { $_->contains_line($line) } @$inner_mask) {
                        $self->_space->insert_edge($line->a, $line->b, $line->length);
                    }
                }
            }
#            $DB::single=1;
            # generate the stripe around slice contours
            my $contour = offset_ex(
                diff(
                    $outer,
                    [ map @$_, @$inner ],
                ),
                scale .1);
            push @{ $self->_contour }, @$contour;
            # find pairs of visible points in this area and add them to the graph
            for my $i (0 .. $#inner_points) {
                for my $j (0 .. $#outer_points) {
                    my ($a, $b) = ($inner_points[$i], $outer_points[$j]);
                    my $line = Slic3r::Line->new($a, $b);
                    # turn $contour into an ExPolygonCollection and use $contour->contains_line()
                    if (first { $_->contains_line($line) } @$contour) {
                        $self->_space->insert_edge($line->a, $line->b, $line->length * CROSSING_PENALTY);
                    }
                }
            }
        }
    }
    }



#    $DB::single=1;
    if (0) {
        require "Slic3r/SVG.pm";
        Slic3r::SVG::output("space.svg",
                            no_arrows       => 1,
                            line_width      => .03,
                            cyan_expolygons => $self->islands,
                            red_expolygons  => $outside,
                            green_expolygons => $self->_inner,
                            blue_expolygons => $self->_contour,
                            lines           => $self->_space->lines,
                            points          => $self->_space->points,
            );
        printf "%d islands\n", scalar @{$self->islands};
        
        eval "use Devel::Size";
        print  "MEMORY USAGE:\n";
        printf "  %-19s = %.1fMb\n", $_, Devel::Size::total_size($self->$_)/1024/1024
            for qw(_space islands);
        printf "  %-19s = %.1fMb\n", 'self', Devel::Size::total_size($self)/1024/1024;
        
        #exit if $self->internal;
    }
}

sub shortest_path {
    my $self = shift;
    my ($from, $to) = @_;
    
    return Slic3r::Polyline->new($from, $to)
        if !@{$self->_space->points};
    
    # create a temporary configuration space
    my $space = $self->_space;
    
    $self->_add_point_to_space($from, $space);
    $self->_add_point_to_space($to, $space);
    
    # compute shortest path
    my $path = $space->dijkstra($from, $to);

    $DB::single=1;
    
    if (0) {
        require "Slic3r/SVG.pm";
        Slic3r::SVG::output("path.svg",
                            no_arrows       => 1,
                            line_width      => .1,
                            green_expolygons      => $self->islands,
                            lines           => $space->lines,
                            red_points      => [$from, $to],
                            red_polylines   => [$path],
        );
#        exit;
    }
 
    if (!$path->is_valid) {
        Slic3r::debugf "Failed to compute shortest path.\n";
        return Slic3r::Polyline->new($from, $to);
    }
     
    return $path;
}

# returns the index of the new node
sub _add_point_to_space {
    my ($self, $point, $space) = @_;
        
    # check whether we are inside an island or outside
    my $inside = defined first { $_->contains_point($point) } @{ $self->_inner };
    my $contour = defined first { $_->contains_point($point) } @{ $self->_contour };
    my $outside = !$inside && !$contour;

    # find candidates by checking visibility from $from to them
    my $nodes=$space->points_in_range($point, scale 5);
    foreach my $idx (0..$#{$nodes}) {
        my $line = Slic3r::Line->new($point, $nodes->[$idx]);
        if ($inside && defined first { $_->contains_line($line) } @{$self->_inner}) {
            $space->insert_edge($point, $nodes->[$idx], $line->length);
        } 
        if ($contour && defined first { $_->contains_line($line) } @{$self->_contour}) {
            $space->insert_edge($point, $nodes->[$idx], $line->length * CROSSING_PENALTY);
        }
        if ($outside && defined first { $_->contains_line($line) } @{$self->_outer}) {
            $space->insert_edge($point, $nodes->[$idx], $line->length);
        }          
    }
    
    # if we found no visibility, find closest existing point and add it with penalty
    if ($space->find_point($point)<0) {
        my $nearest=$space->nearest_point($point);
        my $line = Slic3r::Line->new($point, $nearest);
        $space->insert_edge($point, $nearest, $line->length * CROSSING_PENALTY);
    }
}

1;

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
has '_outer_margin' => (is => 'ro', default => sub { scale 1 });

# this factor weigths the crossing of a perimeter 
# vs. the alternative path. a value of 5 means that
# a perimeter will be crossed if the alternative path
# is >= 5x the length of the straight line we could
# follow if we decided to cross the perimeter.
# a nearly-infinite value for this will only permit
# perimeter crossing when there's no alternative path.
use constant CROSSING_PENALTY => 10;

use constant POINT_DISTANCE => 1;  # unscaled

# setup our configuration space
sub BUILD {
    my $self = shift;
    
    my $point_distance = scale POINT_DISTANCE;

    $DB::single=1;
    
    if(!@{$self->islands}) {
        $DB::single=1;
        return;
    }
    # create outside space
    my $grown_islands=offset( [map @$_, @{$self->islands}], +$self->_outer_margin);
    
    my $bb=Slic3r::Geometry::BoundingBox->new_from_points([ map @$_, @$grown_islands]);
    my $ofs=scale 10;
    
    my $bb_poly=Slic3r::Polygon->new([$bb->x_min-$ofs,$bb->y_min-$ofs], [$bb->x_min-$ofs,$bb->y_max+$ofs], 
                                     [$bb->x_max+$ofs,$bb->y_max+$ofs], [$bb->x_max+$ofs,$bb->y_min-$ofs]);
    my $outside=diff_ex(
        [ $bb_poly ],
        $grown_islands
        );

    $self->_space->add_Polygon($bb_poly,1);
    
    for my $i (0 .. $#{$self->islands}) {
        my $expolygon = $self->islands->[$i];

        # find external margin
        my $outer = offset_ex(\@$expolygon, +$self->_outer_margin);
        my $inner = offset_ex(\@$expolygon, -$self->_inner_margin);

        my $contour = diff_ex(
            [ map @$_, @$outer ],
            [ map @$_, @$inner ],
            );
        for my $poly (map @$_, @$contour) {
            my @pts=@{$poly->equally_spaced_points($point_distance)};
            shift @pts;
            $self->_space->add_Polygon(Slic3r::Polygon->new(@pts),1,CROSSING_PENALTY);
        }
        push @{ $self->_inner }, @$inner;
        push @{ $self->_contour }, @$contour;
    }
    $self->_space->triangulate();
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
    }
}

sub shortest_path {
    my $self = shift;
    my ($from, $to) = @_;
    
    # create a temporary configuration space
    my $space = $self->_space;
    
    #$self->_add_point_to_space($from, $space);
    #$self->_add_point_to_space($to, $space);
    
    # compute shortest path
    my $path = $space->dijkstra($from, $to);

#    $DB::single=1;
    $space->SVG_dump_path("path-$self.svg", $from, $to);

    if (0) {
        require "Slic3r/SVG.pm";
        Slic3r::SVG::output("path.svg",
                            no_arrows       => 1,
                            line_width      => .1,
                            green_expolygons   => $self->islands,
                            lines           => $space->lines,
                            red_points      => [$from, $to],
                            red_polylines   => [$path],
        );
    }


    if (!$path->is_valid) {
        Slic3r::debugf "Failed to compute shortest path.\n";
        return Slic3r::Polyline->new($from, $to);
    }
     
    return $path;
}


1;

package Slic3r::GCode::MotionPlanner;
use Moo;

has 'islands'           => (is => 'ro', required => 1);  # arrayref of ExPolygons
has 'internal'          => (is => 'ro', default => sub { 1 });
has 'id'                => (is => 'ro', default => sub { '' });  # for debugging
has '_space'            => (is => 'ro', default => sub { Slic3r::ConfSpace->new });
has '_inner'            => (is => 'ro', default => sub { [] });  # arrayref of ExPolygons
has '_outer'            => (is => 'ro', default => sub { [] });  # arrayref of ExPolygons
has '_contour'          => (is => 'ro', default => sub { [] });  # arrayref of ExPolygons
has '_id_path'          => (is => 'rw', default => sub { 1 });   # path index for debugging

use List::Util qw(first max);
use Slic3r::Geometry qw(A B scale unscale epsilon scaled_epsilon);
use Slic3r::Geometry::Clipper qw(offset offset_ex diff diff_ex diff_pl intersection_pl union_ex union);

# clearance (in mm) from the perimeters
has '_inner_margin' => (is => 'ro', default => sub { scale .7 });
has '_outer_margin' => (is => 'ro', default => sub { scale 1 });

# this factor weigths the crossing of a perimeter 
# vs. the alternative path. a value of 5 means that
# a perimeter will be crossed if the alternative path
# is >= 5x the length of the straight line we could
# follow if we decided to cross the perimeter.
# a nearly-infinite value for this will only permit
# perimeter crossing when there's no alternative path.
use constant CROSSING_PENALTY => 30;

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
    my $bb=Slic3r::Geometry::BoundingBox->new_from_points([ map @$_, map @$_, @{$self->islands} ]);
    $bb->grow(scale 20, scale 20);
    $self->_space->add_Polygon($bb->polygon, 1);  
    # find restricted area
    my $contour = diff(
        offset([map @$_, @{$self->islands}], +$self->_outer_margin),
        offset([map @$_, @{$self->islands}], -$self->_inner_margin)
        );
    my $spaced_contour=[map Slic3r::Polygon->new(splice($_->equally_spaced_points($point_distance), 1)), @$contour];
    
    $self->_space->add_Polygon($_, CROSSING_PENALTY) for @$spaced_contour;
    $self->_space->triangulate();
}

sub shortest_path {
    my $self = shift;
    my ($from, $to) = @_;
    
    # create a temporary configuration space
    my $space = $self->_space;
    
    # compute shortest path
    my $path = $space->path($from, $to);

#    $DB::single=1;
    my $fid=$self->id . "-" . $self->_id_path;
    $space->SVG_dump_path("path-$fid.svg", $from, $to, $path);    
    $self->_id_path($self->_id_path + 1);
    if (0) {
        require "Slic3r/SVG.pm";
        Slic3r::SVG::output("path-p-$fid.svg",
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

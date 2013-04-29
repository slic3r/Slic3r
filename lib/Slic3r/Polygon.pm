package Slic3r::Polygon;
use strict;
use warnings;

# a polygon is a closed polyline.
use parent 'Slic3r::Polyline';

use Slic3r::Geometry qw(polygon_lines polygon_remove_parallel_continuous_edges
    polygon_remove_acute_vertices polygon_segment_having_point point_in_polygon
    X1 X2 Y1 Y2);
use Slic3r::Geometry::Clipper qw(JT_MITER);

sub new_from_bounding_box {
    my $class = shift;
    my ($bounding_box) = @_;
    
    return $class->new([
        [ $bounding_box->[X1], $bounding_box->[Y1] ],
        [ $bounding_box->[X2], $bounding_box->[Y1] ],
        [ $bounding_box->[X2], $bounding_box->[Y2] ],
        [ $bounding_box->[X1], $bounding_box->[Y2] ],
    ]);
}

sub lines {
    my $self = shift;
    return polygon_lines($self);
}

sub wkt {
    my $self = shift;
    return sprintf "POLYGON((%s))", join ',', map "$_->[0] $_->[1]", @$self;
}

sub is_counter_clockwise {
    my $self = shift;
    return Slic3r::Geometry::Clipper::is_counter_clockwise($self);
}

sub make_counter_clockwise {
    my $self = shift;
    if (!$self->is_counter_clockwise) {
        $self->reverse;
        return 1;
    }
    return 0;
}

sub make_clockwise {
    my $self = shift;
    if ($self->is_counter_clockwise) {
        $self->reverse;
        return 1;
    }
    return 0;
}

sub merge_continuous_lines {
    my $self = shift;
    
    polygon_remove_parallel_continuous_edges($self);
    bless $_, 'Slic3r::Point' for @$self;
}

sub remove_acute_vertices {
    my $self = shift;
    polygon_remove_acute_vertices($self);
    bless $_, 'Slic3r::Point' for @$self;
}

sub point_on_segment {
    my $self = shift;
    my ($point) = @_;
    return polygon_segment_having_point($self, $point);
}

sub encloses_point {
    my $self = shift;
    my ($point) = @_;
    return point_in_polygon($point, $self);
}

sub area {
    my $self = shift;
    return Slic3r::Geometry::Clipper::area($self);
}

sub grow {
    my $self = shift;
    return $self->split_at_first_point->grow(@_);
}

sub simplify {
    my $self = shift;
    return Slic3r::Geometry::Clipper::simplify_polygon( $self->SUPER::simplify(@_) );
}

sub clip_with_polylines {
    my $self = shift;
    my $polylines = shift;
    my $epsilon = shift;
    my $miter_limit = shift;
    $epsilon //= &Slic3r::Geometry::scaled_epsilon; #/
    $miter_limit //= 1; #/
    my @bb = Slic3r::Geometry::bounding_box([@$self, (map @$_, @$polylines)]);
    # my $bb = (Slic3r::Polygon->new(
        # [ $bb[0], $bb[1] ],
        # [ $bb[2], $bb[1] ],
        # [ $bb[2], $bb[3] ],
        # [ $bb[0], $bb[3] ],
        # )->offset($epsilon * ($miter_limit + 1)))[0];

    my $bb = Slic3r::Polygon->new(
        (Slic3r::Geometry::Clipper::offset(
            [
                [
                    [ $bb[0], $bb[1] ],
                    [ $bb[2], $bb[1] ],
                    [ $bb[2], $bb[3] ],
                    [ $bb[0], $bb[3] ],
                ]
            ],
        $epsilon * ($miter_limit + 1)))[0]
    );

    my $clip_expolygonbb = Slic3r::Geometry::Clipper::diff_ex(
        [$bb],
        [map $_->grow($epsilon, undef, &Math::Clipper::JT_MITER, $miter_limit), @$polylines]
        );
    my $polygon_as_polyline = Slic3r::Polyline->new([@$self,$self->[0]]);
    my @frags = map $polygon_as_polyline->clip_with_expolygon($_), @$clip_expolygonbb;

    # no intersection: return a Polygon copy of the original Polygon    
    return $self->clone if @frags == 1;

    # relink two fragments that came from the original start and end of the polygon
    for (my $i = 0; $i < @frags; $i++) {
        for (my $j = 0; $j < @frags; $j++) {
            next if $i == $j;
            if (Slic3r::Geometry::same_point($frags[$i]->[-1], $frags[$j]->[0])) {
                pop @{$frags[$i]};
                push @{$frags[$i]}, @{splice(@frags, $j, 1)};
                last;
            }
        }
    }

    return @frags;
}

# this method subdivides the polygon segments to that no one of them
# is longer than the length provided
sub subdivide {
    my $self = shift;
    my ($max_length) = @_;
    
    for (my $i = 0; $i <= $#$self; $i++) {
        my $len = Slic3r::Geometry::line_length([ $self->[$i-1], $self->[$i] ]);
        my $num_points = int($len / $max_length) - 1;
        $num_points++ if $len % $max_length;
        
        # $num_points is the number of points to add between $i-1 and $i
        next if $num_points == -1;
        my $spacing = $len / ($num_points + 1);
        my @new_points = map Slic3r::Point->new($_),
            map Slic3r::Geometry::point_along_segment($self->[$i-1], $self->[$i], $spacing * $_),
            1..$num_points;
        
        splice @$self, $i, 0, @new_points;
        $i += @new_points;
    }
}

# returns false if the polygon is too tight to be printed
sub is_printable {
    my $self = shift;
    my ($width) = @_;
    
    # try to get an inwards offset
    # for a distance equal to half of the extrusion width;
    # if no offset is possible, then polyline is not printable.
    # we use flow_width here because this has to be consistent 
    # with the thin wall detection in Layer->make_surfaces, 
    # otherwise we could lose surfaces as that logic wouldn't
    # detect them and we would be discarding them.
    my $p = $self->clone;
    $p->make_counter_clockwise;
    return Slic3r::Geometry::Clipper::offset([$p], -$width / 2) ? 1 : 0;
}

sub is_valid {
    my $self = shift;
    return @$self >= 3;
}

sub split_at_index {
    my $self = shift;
    my ($index) = @_;
    
    return Slic3r::Polyline->new(
        @$self[$index .. $#$self], 
        @$self[0 .. $index],
    );
}

sub split_at {
    my $self = shift;
    my ($point) = @_;
    
    # find index of point
    my $i = -1;
    for (my $n = 0; $n <= $#$self; $n++) {
        if (Slic3r::Geometry::same_point($point, $self->[$n])) {
            $i = $n;
            last;
        }
    }
    die "Point not found" if $i == -1;
    
    return $self->split_at_index($i);
}

sub split_at_first_point {
    my $self = shift;
    return $self->split_at_index(0);
}

1;
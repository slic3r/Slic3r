use Test::More;
use strict;
use warnings;

plan tests => 1;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
}

use Slic3r;
use Slic3r::Geometry qw(scale);

my $square = [
   [10,10],[110,10],[110,110],[10,110],
];

my $hole = [
   [30,30],[30,80],[80,80],[80,30],
];

my $points = [
   [0,0],
   [120,120],
];

my $slice=Slic3r::ExPolygon->new_scale($square, $hole);

my $mp=Slic3r::GCode::MotionPlanner->new(
   islands => [ $slice ],
   internal => 1
);

my $path=$mp->shortest_path(Slic3r::Point->new_scale(@{$points->[0]}), Slic3r::Point->new_scale(@{$points->[1]}));

is $path->length, 10, 'Path found';

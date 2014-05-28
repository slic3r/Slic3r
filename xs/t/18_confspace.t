#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;

use Test::More tests => 2;

{
    my $cs=Slic3r::ConfSpace->new();
    isa_ok $cs, 'Slic3r::ConfSpace', 'new';
    
    if(0) {
    my $point1 = Slic3r::Point->new(10, 15);
    my $point2 = Slic3r::Point->new(10, 25);

    my $idx1=$cs->insert_point($point1);
    is $cs->insert_point($point1), $idx1, 'no duplicates';
    my $idx2=$cs->insert_point($point2);
    is scalar @{$cs->points}, 2, 'two points created';

    my $n=$cs->nearest_point(Slic3r::Point->new(10, 21));
    is $n->distance_to($point2), 0, 'Correct nearest point';
    }
    my $square = [
        [10,10],[110,10],[110,110],[10,110],
        ];

    my $hole = [
        [30,30],[30,80],[80,80],[80,30],
        ];
    
    my $points = [
        [15,15],
        [90,90],
        ];
    
   
    my $expolygon = Slic3r::ExPolygon->new($square,$hole);
    
    $cs->add_Polygon($_,1) for @$expolygon;
    $cs->triangulate();
    my $lines=$cs->lines;
    $DB::single=1;
    my $path=$cs->dijkstra(Slic3r::Point->new(@{$points->[0]}), Slic3r::Point->new(@{$points->[1]}));
    is scalar @$path, 5, 'path found';
    $cs->SVG_dump_path('confspace.svg', Slic3r::Point->new(@{$points->[0]}), Slic3r::Point->new(@{$points->[1]}));
}

__END__

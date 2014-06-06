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
        [10e6,10e6],[110e6,10e6],[110e6,110e6],[10e6,110e6],
        ];

    my $hole = [
        [30e6,30e6],[30e6,80e6],[80e6,80e6],[80e6,30e6],
        ];
    
    my $points = [
        [15e6,15e6],
        [90e6,90e6],
        ];
    
   
    my $expolygon = Slic3r::ExPolygon->new($square,$hole);
    
    $cs->add_Polygon($_,10) for @$expolygon;
    $cs->triangulate();
    $DB::single=1;
    my $path=$cs->path(Slic3r::Point->new(@{$points->[0]}), Slic3r::Point->new(@{$points->[1]}));
    is scalar @$path, 3, 'path found';
    $cs->SVG_dump_path('confspace.svg', Slic3r::Point->new(@{$points->[0]}), Slic3r::Point->new(@{$points->[1]}), $path);
}

__END__

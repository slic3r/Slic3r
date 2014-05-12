#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More tests => 6;

{
    my $cs=Slic3r::ConfSpace->new();
    isa_ok $cs, 'Slic3r::ConfSpace', 'new';

    my $point1 = Slic3r::Point->new(10, 15);
    my $point2 = Slic3r::Point->new(10, 25);
    
    my $idx1=$cs->insert_point($point1);
    is $cs->insert_point($point1), $idx1, 'no duplicates';
    my $idx2=$cs->insert_point($point2);
    is scalar @{$cs->points}, 2, 'two points created';

    my $n=$cs->nearest_point(Slic3r::Point->new(10, 21));
    is $n->distance_to($point2), 0, 'Correct nearest point';

    $cs->insert_edge($point1, $point2, 10);
    $cs->insert_edge($point2, $point1, 10);
    my $lines=$cs->lines;
    is scalar @{$lines}, 1, 'single edge created';

    my $points = [
        [100, 100],
        [100, 200],
        [200, 200],
        [200, 100],
        [150, 150],
        ];
    my $edges = [
        [0, 1],
        [1, 2],
        [2, 3],
        [3, 0],
        [3, 4]
        ];
    foreach my $e (@$edges) {
        my $p1=Slic3r::Point->new(@{$points->[$e->[0]]});
        my $p2=Slic3r::Point->new(@{$points->[$e->[1]]});
        $cs->insert_edge($p1,$p2,$p1->distance_to($p2));
    }
    $DB::single=1;
    my $path=$cs->dijkstra(Slic3r::Point->new(@{$points->[1]}), Slic3r::Point->new(@{$points->[4]}));
    is scalar @$path, 4, 'path found';
}

__END__

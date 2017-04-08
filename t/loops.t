use Test::More;
use strict;
use warnings;

plan tests => 4;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
    use local::lib "$FindBin::Bin/../local-lib";
}

use Slic3r;
use Slic3r::Geometry qw(Z epsilon scale);
use Slic3r::Test;

{
    # We want to check what happens when three concentric loops happen
    # to be at the same height, the two external ones being ccw and the other being
    # a hole, thus cw. So we create three cubes, centered around origin, the internal
    # one having reversed normals.
    my $mesh1 = Slic3r::Test::mesh('20mm_cube');
    
    # center around origin
    my $bb = $mesh1->bounding_box;
    $mesh1->translate(
        -($bb->x_min + $bb->size->x/2),
        -($bb->y_min + $bb->size->y/2),  #//
        -($bb->z_min + $bb->size->z/2),
    );
    
    my $mesh2 = $mesh1->clone;
    $mesh2->scale(1.2);
    
    my $mesh3 = $mesh2->clone;
    $mesh3->scale(1.2);
    
    $mesh1->reverse_normals;
    ok $mesh1->volume < 0, 'reverse_normals';
    
    my $all_meshes = Slic3r::TriangleMesh->new;
    $all_meshes->merge($_) for $mesh1, $mesh2, $mesh3;
    
    my $loops = $all_meshes->slice_at(Z, 0);
    is scalar(@$loops), 1, 'one expolygon returned';
    is scalar(@{$loops->[0]->holes}), 1, 'expolygon has one hole';
    ok abs(-$loops->[0]->holes->[0]->area - scale($bb->size->x)*scale($bb->size->y)) < epsilon, #))
        'hole has expected size';
}

__END__

#!/usr/bin/perl

use strict;
use warnings;

use Test::More;

#plan skip_all => 'temporarily disabled';

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../../lib";
    use local::lib "$FindBin::Bin/../../local-lib";
}

use Slic3r::XS;
use Slic3r::Geometry qw(X Y Z);

my $mat1 = Slic3r::TransformationMatrix->new;

ok(check_elements($mat1,1,0,0,0,0,1,0,0,0,0,1,0), 'eye, init');

$mat1->set_elements(1,2,3,4,5,6,7,8,9,10,11,12);
my $mat2 = Slic3r::TransformationMatrix->new;
$mat2->set_elements(1,4,7,10,2,5,8,11,3,6,9,12);

ok(check_elements($mat1,1,2,3,4,5,6,7,8,9,10,11,12), 'set elements, 1');
ok(check_elements($mat2,1,4,7,10,2,5,8,11,3,6,9,12), 'set elements, 2');

my $mat_3 = Slic3r::TransformationMatrix->new;
$mat_3->set_multiply($mat1,$mat2);
ok(check_elements($mat_3,14,32,50,72,38,92,146,208,62,152,242,344), 'multiply: M1 * M2');

$mat_3->set_multiply($mat2,$mat1);
ok(check_elements($mat_3,84,96,108,130,99,114,129,155,114,132,150,180), 'multiply: M2 * M1');


ok(check_elements($mat2->multiplyLeft($mat1),14,32,50,72,38,92,146,208,62,152,242,344), 'multiplyLeft');
ok(check_elements($mat1->multiplyRight($mat2),14,32,50,72,38,92,146,208,62,152,242,344), 'multiplyRight');

my $mat_rnd = Slic3r::TransformationMatrix->new;
$mat_rnd->set_elements(0.9004,-0.2369,-0.4847,12.9383,-0.9311,0.531,-0.5026,7.7931,-0.1225,0.5904,0.2576,-7.316);

ok(abs($mat_rnd->determinante() - 0.5539) < 0.0001, 'determinante');

my $inv_rnd = $mat_rnd->inverse();
ok(check_elements($inv_rnd,0.78273,-0.4065,0.67967,-1.9868,0.54422,0.31157,1.6319,2.4697,-0.87509,-0.90741,0.46498,21.7955), 'inverse');

my $vec1 = Slic3r::Pointf3->new(1,2,3);
my $vec2 = Slic3r::Pointf3->new(-4,3,-2);

$mat1->set_scale_uni(3);
ok(check_point(multiply_point($mat1,$vec1),3,6,9),'scale uniform');

$mat1->set_scale_xyz(2,3,4);
ok(check_point(multiply_point($mat1,$vec1),2,6,12),'scale xyz');

$mat1->set_mirror_xyz(X);
my $vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,-1,2,3),'mirror axis X');

$mat1->set_mirror_xyz(Y);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,1,-2,3),'mirror axis Y');

$mat1->set_mirror_xyz(Z);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,1,2,-3),'mirror axis Z');

$mat1->set_mirror_vec($vec2);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,-0.1034,2.8276,2.4483),'mirror arbituary axis');



done_testing();

sub check_elements {
    my $cmp_trafo = Slic3r::TransformationMatrix->new;
    $cmp_trafo->set_elements($_[1],$_[2],$_[3],$_[4],$_[5],$_[6],$_[7],$_[8],$_[9],$_[10],$_[11],$_[12]);
    return $_[0]->equal($cmp_trafo);
}

sub multiply_point {
    my $trafo = $_[0];
    my $x = 0;
    my $y = 0;
    my $z = 0;
    if ($_[1]->isa('Slic3r::Pointf3')) {
        $x = $_[1]->x();
        $y = $_[1]->y();
        $z = $_[1]->z();
    } else {
        $x = $_[1];
        $y = $_[2];
        $z = $_[3];
    }
    
    my $ret = Slic3r::Pointf3->new;
    $ret->set_x($trafo->m11()*$x + $trafo->m12()*$y + $trafo->m13()*$z + $trafo->m14());
    $ret->set_y($trafo->m21()*$x + $trafo->m22()*$y + $trafo->m23()*$z + $trafo->m24());
    $ret->set_z($trafo->m31()*$x + $trafo->m32()*$y + $trafo->m33()*$z + $trafo->m34());
    return $ret
}

sub check_point {
    my $eps = 0.001;
    my $equal = 1;
    $equal = $equal & (abs($_[0]->x() - $_[1]) < $eps);
    $equal = $equal & (abs($_[0]->y() - $_[2]) < $eps);
    $equal = $equal & (abs($_[0]->z() - $_[3]) < $eps);
    return $equal;
}

__END__

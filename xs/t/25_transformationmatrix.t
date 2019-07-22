#!/usr/bin/perl

use strict;
use warnings;

use Test::More;

use Slic3r::XS;

use constant X => 0;
use constant Y => 1;
use constant Z => 2;

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
ok(check_elements($inv_rnd,0.78273016,-0.40649736,0.67967289,-1.98683622,0.54421957,0.31157368,1.63191055,2.46965668,-0.87508846,-0.90741083,0.46498424,21.79552507), 'inverse');

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
ok(abs($mat1->determinante() + 1) < 0.0001,'mirror arbituary axis - determinante');

$mat1->set_translation_xyz(4,2,5);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,5,4,8),'translate xyz');

$mat1->set_translation_vec($vec2);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,-3,5,1),'translate vector');

$mat1->set_rotation_ang_xyz(deg2rad(90),X);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,1,-3,2),'rotation X');

$mat1->set_rotation_ang_xyz(deg2rad(90),Y);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,3,2,-1),'rotation Y');

$mat1->set_rotation_ang_xyz(deg2rad(90),Z);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,-2,1,3),'rotation Z');

$mat1->set_rotation_ang_arb_axis(deg2rad(80),$vec2);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,3.0069,1.8341,-1.2627),'rotation arbituary axis');
ok(abs($mat1->determinante() - 1) < 0.0001,'rotation arbituary axis - determinante');

$mat1->set_rotation_quat(-0.4775,0.3581,-0.2387,0.7660);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,3.0069,1.8341,-1.2627),'rotation quaternion');
ok(abs($mat1->determinante() - 1) < 0.0001,'rotation quaternion - determinante');

$mat1->set_rotation_vec_vec($vec1,$vec2);
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,-2.7792,2.0844,-1.3896),'rotation vector to vector 1');
ok(abs($mat1->determinante() - 1) < 0.0001,'rotation vector to vector 1 - determinante');

$mat1->set_rotation_vec_vec($vec1,$vec1->negative());
$vecout = multiply_point($mat1,$vec1);
ok(check_point($vecout,-1,-2,-3),'rotation vector to vector 2 - colinear, oppsite directions');

$mat1->set_rotation_vec_vec($vec1,$vec1);
ok(check_elements($mat1,1,0,0,0,0,1,0,0,0,0,1,0), 'rotation vector to vector 3 - colinear, same directions');


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
    $ret->set_x($trafo->m00()*$x + $trafo->m01()*$y + $trafo->m02()*$z + $trafo->m03());
    $ret->set_y($trafo->m10()*$x + $trafo->m11()*$y + $trafo->m12()*$z + $trafo->m13());
    $ret->set_z($trafo->m20()*$x + $trafo->m21()*$y + $trafo->m22()*$z + $trafo->m23());
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

sub deg2rad {
    return ($_[0] * 3.141592653589793238 / 180);
}

__END__

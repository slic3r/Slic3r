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

is(1, 1, 'Dummy test');

done_testing();

sub check_elements {
    my $cmp_trafo = Slic3r::TransformationMatrix->new;
    $cmp_trafo->set_elements($_[1],$_[2],$_[3],$_[4],$_[5],$_[6],$_[7],$_[8],$_[9],$_[10],$_[11],$_[12]);
    return $_[0]->equal($cmp_trafo);
}

sub multiply_point {
    my $trafo = $_[0];
    my $x = $_[1];
    my $y = $_[1];
    my $z = $_[1];
    my $ret = Slic3r::Pointf3->new;
    $ret->set_x($trafo->m11()*$x + $trafo->m12()*$y + $trafo->m13()*$z + $trafo->m14());
    $ret->set_y($trafo->m21()*$x + $trafo->m22()*$y + $trafo->m23()*$z + $trafo->m24());
    $ret->set_z($trafo->m31()*$x + $trafo->m32()*$y + $trafo->m33()*$z + $trafo->m34());
    return $ret
}

sub check_point {
    my $eps = 0.0001;
    my $equal = 1;
    $equal = $equal & (abs($_[0]->x() - $_[1]) < $eps);
    $equal = $equal & (abs($_[0]->y() - $_[2]) < $eps);
    $equal = $equal & (abs($_[0]->z() - $_[3]) < $eps);
    return $equal;
}

__END__

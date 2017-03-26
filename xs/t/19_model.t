#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More tests => 5;

{
    my $model = Slic3r::Model->new;
    my $object = $model->_add_object;
    isa_ok $object, 'Slic3r::Model::Object::Ref';
    isa_ok $object->origin_translation, 'Slic3r::Pointf3::Ref';
    $object->origin_translation->translate(10,0,0);
    is_deeply \@{$object->origin_translation}, [10,0,0], 'origin_translation is modified by ref';
    
    my $lhr = [ [ 5, 10, 0.1 ] ];
    $object->set_layer_height_ranges($lhr);
    is_deeply $object->layer_height_ranges, $lhr, 'layer_height_ranges roundtrip';
}

{
	my $model = eval { Slic3r::Model->read_from_file("a.xyz") };
	ok !$model && $@, "Error while reading STL file";
}

__END__

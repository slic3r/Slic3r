#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More tests => 3;

{
    my $gcodegen = Slic3r::GCode->new;
    $gcodegen->set_origin(Slic3r::Pointf->new(10,0));
    is_deeply $gcodegen->origin->pp, [10,0], 'set_origin';
    $gcodegen->origin->translate(5,5);
    is_deeply $gcodegen->origin->pp, [15,5], 'origin returns reference to point';
}

{
    my $config = Slic3r::Config->new_from_defaults;
    $config->set('use_firmware_retraction', 1)
    $config->set('gcode_flavor', 'reprap')

    my $gcodegen = Slic3r::GCode->new;
    my $output = $gcodegen->writer->retract_for_toolchange;
    is $output, "G10 S1; retract for toolchange"
{
    my $config = Slic3r::Config->new_from_defaults;
    $config->set('use_firmware_retraction', 1)
    $config->set('gcode_flavor', 'reprap')

    my $gcodegen = Slic3r::GCode->new;
    my $output = $gcodegen->writer->retract_for_toolchange;
    is $output, "G10 S1; retract for toolchange", 'Produces long retract for fw marlin retract';
}

{
    my $config = Slic3r::Config->new_from_defaults;
    $config->set('use_firmware_retraction', 1)
    $config->set('gcode_flavor', 'repetier')

    my $gcodegen = Slic3r::GCode->new;
    my $output = $gcodegen->writer->retract_for_toolchange;
    is $output, "G10 S1; retract for toolchange", 'Produces long retract for fw repetier retract';

}
{
    my $config = Slic3r::Config->new_from_defaults;
    $config->set('gcode_flavor', 'smoothie')
    $config->set('use_firmware_retraction', 1)

    my $gcodegen = Slic3r::GCode->new;
    my $output = $gcodegen->writer->retract_for_toolchange;
    is $output, "G10; retract for toolchange", 'Produces regular retract for non-reprap';

}

}
__END__

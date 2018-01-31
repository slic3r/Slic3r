#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More tests => 5;

{
    my $gcodegen = Slic3r::GCode->new;
    $gcodegen->set_origin(Slic3r::Pointf->new(10,0));
    is_deeply $gcodegen->origin->pp, [10,0], 'set_origin';
    $gcodegen->origin->translate(5,5);
    is_deeply $gcodegen->origin->pp, [15,5], 'origin returns reference to point';
}

{
    my $config = Slic3r::Config::Static::new_FullPrintConfig;
    my $gcodegen = Slic3r::GCode->new;
    $config->set('use_firmware_retraction', 1);
    $config->set('gcode_flavor', "reprap");
    $config->set('gcode_comments', 1);
    $gcodegen->apply_print_config($config);
    $gcodegen->writer->set_extruders([0]);
    $gcodegen->writer->set_extruder(0);
    my @output = split(/\n/, $gcodegen->retract(1));
    is $output[0], "G10 S1 ; retract for toolchange extruder 0", 'Produces long retract for fw marlin retract';
}

{
    my $config = Slic3r::Config::Static::new_FullPrintConfig;
    $config->set('use_firmware_retraction', 1);
    $config->set('gcode_flavor', "repetier");
    $config->set('gcode_comments', 1);

    my $gcodegen = Slic3r::GCode->new;
    $gcodegen->apply_print_config($config);
    $gcodegen->writer->set_extruders([0]);
    $gcodegen->writer->set_extruder(0);
    my @output = split(/\n/, $gcodegen->retract(1));
    is $output[0], "G10 S1 ; retract for toolchange extruder 0", 'Produces long retract for fw repetier retract';

}
{
    my $config = Slic3r::Config::Static::new_FullPrintConfig;
    $config->set('gcode_flavor', "smoothie");
    $config->set('use_firmware_retraction', 1);
    $config->set('gcode_comments', 1);

    my $gcodegen = Slic3r::GCode->new;
    $gcodegen->apply_print_config($config);

    $gcodegen->writer->set_extruders([0]);
    $gcodegen->writer->set_extruder(0);
    my @output = split(/\n/, $gcodegen->retract(1));
    ok($output[0] eq "G10 ; retract for toolchange extruder 0", 'Produces regular retract for flavors that are not Marlin or Repetier');

}

__END__

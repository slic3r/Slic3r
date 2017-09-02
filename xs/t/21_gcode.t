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
    $gcodegen->apply_print_config($config);
    printf($gcodegen->retract . " <- \n");
    my $output = $gcodegen->retract(1);
    is $output, "G10 S1; retract for toolchange", 'Produces long retract for fw marlin retract';
}

{
    my $config = Slic3r::Config::Static::new_FullPrintConfig;
    $config->set('use_firmware_retraction', 1);
    $config->set('gcode_flavor', "repetier");

    my $gcodegen = Slic3r::GCode->new;
    $gcodegen->apply_print_config($config);
    my $output = $gcodegen->retract(1);
    is $output, "G10 S1; retract for toolchange", 'Produces long retract for fw repetier retract';

}
{
    my $config = Slic3r::Config::Static::new_FullPrintConfig;
    $config->set('gcode_flavor', "smoothie");
    $config->set('use_firmware_retraction', 1);

    my $gcodegen = Slic3r::GCode->new;
    $gcodegen->apply_print_config($config);

    my $output = $gcodegen->retract(1);
    ok($output eq "G10; retract for toolchange", 'Produces regular retract for non-reprap');

}

__END__

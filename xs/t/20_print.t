#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More tests => 6;

{
    {
        my $print = Slic3r::Print->new;
        isa_ok $print, 'Slic3r::Print';
        isa_ok $print->config, 'Slic3r::Config::Static::Ref';
        isa_ok $print->default_object_config, 'Slic3r::Config::Static::Ref';
        isa_ok $print->default_region_config, 'Slic3r::Config::Static::Ref';
        isa_ok $print->placeholder_parser, 'Slic3r::GCode::PlaceholderParser::Ref';
    }

    {
        my $print = Slic3r::Print->new;
        my $config = Slic3r::Config->new;
        $config->set('skirts', 0);
        $print->apply_config($config);
        $config->set('skirts', 1);
        $print->set_step_started(Slic3r::Print::State::STEP_SKIRT);
        $print->set_step_done(Slic3r::Print::State::STEP_SKIRT);
        my $invalid = $print->apply_config($config);
        ok $invalid, 'applying skirt config invalidates skirt step';
    }

}

__END__

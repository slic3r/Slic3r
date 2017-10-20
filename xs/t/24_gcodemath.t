#!/usr/bin/perl

use strict;
use warnings;

use Slic3r::XS;
use Test::More tests => 3;
{
    {
        my $test_string = "{if{3 == 4}} string";

        my $result = Slic3r::ConditionalGCode::apply_math($test_string);
        is $result, "", 'If statement with nested bracket removes on false resolution.';
    }

    {
        my $test_string = "{if{3 == 3}} string";

        my $result = Slic3r::ConditionalGCode::apply_math($test_string);
        is $result, "string", 'If statement with nested bracket removes itself only on resulting true.';
    }
    {
        my $test_string = "M104 S{4*5}; Sets temp to {4*5}";

        my $result = Slic3r::ConditionalGCode::apply_math($test_string);
        is $result, "M104 S20; Sets temp to 20", 'Bracket replacement works with math ops';
    }
}

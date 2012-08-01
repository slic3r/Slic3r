#!/usr/bin/perl -i
#
# Post-processing script to use mac os 10.8 notification center

use strict;
use warnings;

my $info;

while (<>) {
    if (/^;\sfilament\s+used\s+=\s(.*cm3\))/) {
        $info = "Filament required: $1";
    }
    print;
}

system("./utils/terminal-notifier.app/Contents/MacOS/terminal-notifier -title 'Slicing done!' -message '$info' -group 'SLIC3R'");

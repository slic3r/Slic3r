#!/usr/bin/perl -i
#
# Remove all toolchange commands (and T# arguments) from gcode.

use strict;
use warnings;

# read stdin and any/all files passed as parameters one line at a time
while (<>) {
    if (not /^T[0-9]/) {
        s/\s*(T[0-9])//;
        print;
    } else {
    }
}

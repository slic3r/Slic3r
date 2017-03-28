#!/usr/bin/env perl

use strict;
use warnings;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
    use local::lib "$FindBin::Bin/../local-lib";
}

use Slic3r;

die "Usage: estimate-gcode-time.pl FILE\n"
    if @ARGV != 1;

my $estimator = Slic3r::GCode::TimeEstimator->new;
$estimator->parse_file($ARGV[0]);
printf "Time: %d minutes and %d seconds\n", int($estimator->time / 60), $estimator->time % 60;

__END__

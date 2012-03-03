#!/usr/bin/perl -i

use strict;
use Getopt::Long qw(:config bundling pass_through);
use XXX;

# default values
my $min_fan_speed = 35 * 255 / 100;
my $layer_height = 0.2;
my $max_fan_speed = 255;
my $bridge_fan_speed = 255;

# read specific options from command line
GetOptions(
	'min-fan-speed=i' => \$min_fan_speed,
	'layer-height=f' => \$layer_height,
	'max-fan-speed=i' => \$max_fan_speed,
	'bridge-fan-speed=i' => \$bridge_fan_speed,
);

# read other options from command line
my %options;
while ($ARGV[0] =~ '^--') {
    my ($arg, $value) = (shift, shift);
    $arg =~ s/^--//;
    $options{$arg} = $value;
}

# runtime variables
my $fan_speed = $min_fan_speed * 255 / 100;
my $past_start_gcode = 0;
my $past_first_layer = 0;

# now we start processing
while (<>) {
	# if we find a G90, we're probably looking at the start of the print proper, after user's start codes
	if ($past_start_gcode == 0 && /G90\b/) {
		$past_start_gcode = 1;
	}
	elsif ($past_start_gcode == 1) {
		# set fan to idle after the first layer
		if ($past_first_layer == 0 && /Z([\d\.]+)/) {
			if ($1 > $layer_height) {
				$past_first_layer = 1;
				printf "M106 S%d\n", $fan_speed;
			}
		}
		# when we get a non-maximum fan speed, save it
		if (/M106\s+S(\d+)/) {
			$fan_speed = $1
				if $1 >= ($min_fan_speed * 255 / 100) && $1 < ($max_fan_speed * 255 / 100) && $1 != int($bridge_fan_speed * 255 / 100);
		}
		# replace all M107 with the previous fan speed
		else {
			s/\bM107\b/sprintf("M106 S%d",$fan_speed)/ei;
		}
	}
	# write (possibly altered) line to output file
	print;
}

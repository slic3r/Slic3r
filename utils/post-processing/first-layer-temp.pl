#!/usr/bin/perl

use strict;
use Getopt::Long qw(:config bundling pass_through);

# default values
my $layer_height = 0.2;
my $first_layer_temp = 190;
my $rest_layers_temp = 170;

# read options from command line
GetOptions(
	'first_layer_temp=f' => \$first_layer_temp,
	'temperature=f' => \$rest_layers_temp,
	'layer_height=f' => \$layer_height);

while ($ARGV[0] =~ /^--/) {
	shift; shift;
}

if ($rest_layers_temp == 0) {
	die "first-layer-temp script is useless without a standard print temperature configured! Please set your temperature to something other than zero"
}

# set up state machine
my $past_start_gcode = 0;

# and ensure we see the first layer
my $current_layer = 0;

# now we start processing
for (<>) {
	# if we find a G90, we're probably looking at the start of the print proper, after user's start codes
	if ($past_start_gcode == 0 && /G90\b/) {
		$past_start_gcode = 1;
	}
	elsif ($past_start_gcode == 1) {
		# if we find a Z value
		if (/Z([\d\.]+)/) {
			# work out which layer we're at
			my $layer = $1;
			my $layer_count = int($1 / $layer_height);
			# if we just got here
			if ($current_layer != $layer_count) {
				$current_layer = $layer_count;
				# and it's the first layer
				if ($layer_count == 1) {
					# insert our first layer temp
					print "M104 S$first_layer_temp\n";
				}
				# or if it's the next layer
				elsif (($layer_count == 2) && ($rest_layers_temp > 0)) {
					# insert temperature for rest of print
					print "M104 S$rest_layers_temp\n";
					$past_start_gcode = 2;
				}
			}
		}
	}
	# write input data unmodified unless it's slic3r's temperature stuff which we remove
	print unless /^M109 S\d+ ; wait for temperature to be reached$/ || /^M104 S\d+ ; set temperature/;
}

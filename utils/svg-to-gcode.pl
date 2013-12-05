#!/usr/bin/perl
# This script converts a SVG file to GCode

use strict;
use warnings;

BEGIN {
    use FindBin;
    use lib "$FindBin::Bin/../lib";
}

use Getopt::Long qw(:config no_auto_abbrev);
use Slic3r;
use Slic3r::PathsToGCode;
$|++;

my %opt = ();
{
    my %options = (
        'help'                  => sub { usage() },
        'scale'                 => \$opt{scale},
    );
    GetOptions(%options) or usage(1);
    $ARGV[0] or usage(1);
}

{
    my $input_file = $ARGV[0];
    my $output_file = $input_file;
    $output_file =~ s/\.svg$/.gcode/i;
    
    my $converter = Slic3r::PathsToGCode->new(
        scale => $opt{scale} // 1,
    );
    $converter->process_svg($input_file, $output_file);
    printf "G-code written to $output_file\n";
}


sub usage {
    my ($exit_code) = @_;
    
    print <<"EOF";
Usage: svg-to-gcode.pl [ OPTIONS ] file.svg

    --help              Output this usage screen and exit
    --scale SCALE       Scale factor (default: 1)
    
EOF
    exit ($exit_code || 0);
}


__END__

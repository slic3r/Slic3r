#!/usr/bin/perl -i

use strict;
use Data::Dumper;

my %options;

while ($ARGV[0] =~ '^--') {
	my ($arg, $value) = (shift, shift);
	$arg =~ s/^--//;
	$options{$arg} = $value;
}

# die Dumper \%options;

# read stdin and any/all files passed as parameters one line at a time
while (<>) {
	s/\[([\w\-_]+)\]/$options{$1} or $1/egis;
	print;
}

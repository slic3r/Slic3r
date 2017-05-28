#!/usr/bin/perl -i
# Conditional gcode post-processor
# Written by Joseph Lenox
# Executes statements beginning with ;_if
# the condition must be evaluatable Perl. 
# If it's true, then print contents of the line after the second ; until end of line.
# Else print nothing.
use strict;
use warnings;
# regex: ^;_if (.* = .*);
while (<>) {
  if ($_ =~ m/^;[ ]*_if/) {
    my @tokens = split(/;/, $_);
    # modify $_ here before printing
    my $condition = $tokens[1] =~ s/_if//r;
    if (eval($condition)) {
      $_ = join('', @tokens[2..$#tokens]);
    } else {
      $_ = "";
    }
  }
  print;
}

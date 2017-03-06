#!/bin/bash

DIR=$(dirname "$0")
$DIR/perl-local -I$DIR/local-lib/lib/perl5 $DIR/slic3r.pl $@

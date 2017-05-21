#!/bin/bash

DIR=$(dirname "$0")
export LD_LIBRARY_PATH=./bin
exec "$DIR/perl-local" -I"$DIR/local-lib/lib/perl5" "$DIR/slic3r.pl" $@

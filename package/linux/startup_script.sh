#!/bin/bash

DIR=$(dirname "$0")
export LD_LIBRARY_PATH=./shlib/x86_64-linux-thread-multi/
exec "$DIR/perl-local" -I"$DIR/local-lib/lib/perl5" "$DIR/slic3r.pl" $@

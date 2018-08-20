#!/bin/bash

BIN=$(readlink "$0")
DIR=$(dirname "$BIN")
export LD_LIBRARY_PATH="$DIR/bin"
exec "$DIR/perl-local" -I"$DIR/local-lib/lib/perl5" "$DIR/slic3r.pl" $@

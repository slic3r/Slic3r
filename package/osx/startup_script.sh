#!/bin/bash

DIR=$(dirname "$0")
exec "$DIR/perl-local" -I"$DIR/local-lib/lib/perl5" "$DIR/slic3r.pl" $@

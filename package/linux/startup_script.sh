#!/bin/bash

BIN=$(readlink "$0")
DIR=$(dirname "$BIN")
export LD_LIBRARY_PATH="$DIR/bin"
exec "$DIR/Slic3r"

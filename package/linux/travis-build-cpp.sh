#!/bin/bash
set -euo pipefail

export WXVERSION=pkg
export CC=gcc-7
export CXX=g++-7
export DISPLAY=:99.0

if [ -f "$(which cmake3)" ]; then 
    export CMAKE=$(which cmake3)
else
    export CMAKE=$(which cmake)
fi

mkdir -p $CACHE

if [[ "$WXVERSION"  != "pkg" ]]; then
    export WXDIR=$HOME/wx${WXVERSION}
    if [ ! -e $CACHE/wx${WXVERSION}.tar.bz2 ]; then
        echo "Downloading http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2 => $CACHE/wx${WXVERSION}.tar.bz2"
        curl -L "http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2" -o $CACHE/wx${WXVERSION}.tar.bz2
    fi
    tar -C$HOME -xjf $CACHE/wx${WXVERSION}.tar.bz2
fi

mkdir build && cd build
${CMAKE} -DSLIC3R_STATIC=ON -DCMAKE_BUILD_TYPE=Release ../src
${CMAKE} --build .
./slic3r_test -s

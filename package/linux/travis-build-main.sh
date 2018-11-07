#!/bin/bash

# This is too strict for source $HOME/perl5/perlbrew/etc/bashrc:
### set -euo pipefail

mkdir -p $CACHE

WXVERSION=302
if [ ! -e $CACHE/slic3r-perlbrew-5.24.tar.bz2 ]; then
    echo "Downloading http://www.siusgs.com/slic3r/buildserver/slic3r-perl.524.travis.tar.bz2 => $CACHE/slic3r-perlbrew-5.24.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/slic3r-perl.524.gcc49.travis.tar.bz2" -o $CACHE/slic3r-perlbrew-5.24.tar.bz2; 
fi

if [ ! -e $CACHE/boost-compiled.tar.bz2 ]; then
    echo "Downloading http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2 => $CACHE/boost-compiled.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2" -o $CACHE/boost-compiled.tar.bz2
fi

if [ ! -e $CACHE/local-lib-wx${WXVERSION}.tar.bz2 ]; then
    echo "Downloading http://www.siusgs.com/slic3r/buildserver/slic3r-dependencies.gcc49.travis-wx${WXVERSION}.tar.bz2 => $CACHE/local-lib-wx${WXVERSION}.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/slic3r-dependencies.travis-wx${WXVERSION}.tar.bz2" -o $CACHE/local-lib-wx${WXVERSION}.tar.bz2
fi

if [ ! -e $CACHE/wx${WXVERSION}.tar.bz2 ]; then
    echo "Downloading http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2 => $CACHE/wx${WXVERSION}.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2" -o $CACHE/wx${WXVERSION}.tar.bz2
fi

tar -C$TRAVIS_BUILD_DIR -xjf $CACHE/local-lib-wx${WXVERSION}.tar.bz2
tar -C$HOME/perl5/perlbrew/perls -xjf $CACHE/slic3r-perlbrew-5.24.tar.bz2
tar -C$HOME -xjf $CACHE/boost-compiled.tar.bz2
tar -C$HOME -xjf $CACHE/wx${WXVERSION}.tar.bz2

export PERLBREW_SKIP_INIT=0
source $HOME/perl5/perlbrew/etc/bashrc
perlbrew switch slic3r-perl

# CXX=g++-4.9 ?
# CC=g++-4.9 ?
SLIC3R_STATIC=1 CC=gcc-7 CXX=g++-7 BOOST_DIR=$HOME/boost_1_63_0 perl ./Build.PL

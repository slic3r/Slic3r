#!/bin/bash

# This is too strict for source $HOME/perl5/perlbrew/etc/bashrc:
### set -euo pipefail

if [ ! -d $HOME/perl5/perlbrew/perls ]; then
    echo "Downloading slic3r-perl.524.travis.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/slic3r-perl.524.gcc49.travis.tar.bz2" -o /tmp/slic3r-perlbrew-5.24.tar.bz2; 
    tar -C$HOME/perl5/perlbrew/perls -xjf $CACHE/slic3r-perlbrew-5.24.tar.bz2
fi

if [ ! -d $HOME/boost_1_63_0 ]; then
    echo "Downloading boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2" -o /tmp/boost-compiled.tar.bz2
    tar -C$HOME -xjf /tmp/boost-compiled.tar.bz2
fi

if [ ! -d $TRAVIS_BUILD_DIR/local-lib ]; then
    echo "Downloading slic3r-dependencies.gcc49.travis-wx302.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/slic3r-dependencies.travis-wx302.tar.bz2" -o /tmp/local-lib-wx302.tar.bz2
    tar -C$TRAVIS_BUILD_DIR -xjf /tmp/local-lib-wx302.tar.bz2
fi

if [ ! -d $HOME/wx302 ]; then
    echo "Downloading buildserver/wx302-libs.tar.bz2"
    curl -L "http://www.siusgs.com/slic3r/buildserver/wx302-libs.tar.bz2" -o /tmp/wx302.tar.bz2
    tar -C$HOME -xjf /tmp/wx302.tar.bz2
fi

source $HOME/perl5/perlbrew/etc/bashrc
perlbrew switch slic3r-perl

SLIC3R_STATIC=1 CC=g++-4.9 CXX=g++-4.9 BOOST_DIR=$HOME/boost_1_63_0 perl ./Build.PL

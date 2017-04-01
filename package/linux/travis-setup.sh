#!/bin/bash
# Script to configure travis environment prior to build
if [ ! -e $HOME/cache/slic3r-perlbrew-5.24.tar.bz2 ]; then
    curl -L "https://bintray.com/lordofhyphens/Slic3r/download_file?file_path=slic3r-perl.524.travis.tar.bz2" -o $HOME/cache/slic3r-perlbrew-5.24.tar.bz2; 
fi

if [ ! -e $HOME/cache/boost-compiled.tar.bz2 ]; then
    curl -L "http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2" -o $HOME/cache/boost-compiled.tar.bz2
fi

if [ ! -e $HOME/cache/local-lib.tar.bz2 ]; then
    curl -L "http://www.siusgs.com/slic3r/buildserver/slic3r-dependencies.travis.tar.bz2" -o $HOME/cache/local-lib.tar.bz2
fi

tar -C$TRAVIS_BUILD_DIR -xjf $HOME/cache/local-lib.tar.bz2
tar -C$HOME/perl5/perlbrew/perls -xjf $HOME/cache/slic3r-perlbrew-5.24.tar.bz2
tar -C$HOME -xjf $HOME/cache/boost-compiled.tar.bz2

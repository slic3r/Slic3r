#!/bin/bash
# Script to configure travis environment prior to build
WXVERSION=302
CACHE=$HOME/cache
mkdir -p $CACHE
if [ $TRAVIS_OS_NAME == "linux" ]; then
    export WXDIR=$HOME/wx${WXVERSION}
    if [ ! -e $CACHE/boost-compiled.tar.bz2 ]; then
        echo "Downloading http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2 => $CACHE/boost-compiled.tar.bz2"
        curl -L "http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2" -o $CACHE/boost-compiled.tar.bz2
    fi

    if [ ! -e $CACHE/wx${WXVERSION}.tar.bz2 ]; then
        echo "Downloading http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2 => $CACHE/wx${WXVERSION}.tar.bz2"
        curl -L "http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2" -o $CACHE/wx${WXVERSION}.tar.bz2
    fi
    
    tar -C$HOME -xjf $CACHE/boost-compiled.tar.bz2
    tar -C$HOME -xjf $CACHE/wx${WXVERSION}.tar.bz2

    # Set some env variables specific to Travis Linux
    export CXX=g++-7
    export CC=gcc-7
elif [ $TRAVIS_OS_NAME == "osx" ]; then
    WXVERSION=311
    export WXDIR=$HOME/wx${WXVERSION}
    if [ ! -e $CACHE/wx${WXVERSION}.tar.bz2 ]; then
        curl -L "https://github.com/wxWidgets/wxWidgets/releases/download/v3.1.1/wxWidgets-3.1.1.tar.bz2" -o $HOME/wx311-src.tar.bz2
        tar -C$HOME -xjr $HOME/wx311-src.tar.bz2
        mkdir $HOME/wxbuild-${WXVERSION}
        cd $HOME/wxbuild-${WXVERSION} && cmake $HOME/wxWidgets-3.1.1  -DwxBUILD_SHARED=off -DCMAKE_INSTALL_PREFIX=${WXDIR}
        cmake --build . --target install
        tar -C$HOME -cjf $CACHE/wx${WXVERSION}.tar.bz2 $(basename ${WXDIR})
    else 
        tar -C$HOME -xjf $CACHE/wx${WXVERSION}.tar.bz2
    fi
fi

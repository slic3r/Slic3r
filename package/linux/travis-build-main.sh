#!/bin/bash

# -u is too strict for source $HOME/perl5/perlbrew/etc/bashrc:
### set -euo pipefail
set -eo pipefail

USE_CC=gcc
USE_CXX=g++

while getopts ":c:pbw" opt; do
    case ${opt} in
        c )
            USE_CC=gcc-${OPTARG}
            USE_CXX=g++-${OPTARG}
        ;;
        p ) 
            CACHED_PERL="no"
        ;;
        b ) 
            CACHED_BOOST="no"
        ;;
        w )
            CACHED_WX="no"
        ;;
    esac
done

if [ ! ${CACHED_PERL} == "no" ]; then
    if [ ! -d $HOME/perl5/perlbrew/perls/slic3r-perl ]; then
        echo "Downloading slic3r-perlbrew-5.28.tar.bz2"
        curl -L "http://www.siusgs.com/slic3r/buildserver/slic3r-perl.528.gcc81.travis.tar.bz2" -o /tmp/slic3r-perlbrew-5.28.tar.bz2; 
        tar -C$HOME/perl5/perlbrew/perls -xjf /tmp/slic3r-perlbrew-5.28.tar.bz2
    fi

    source $HOME/perl5/perlbrew/etc/bashrc
    perlbrew switch slic3r-perl
fi

if [ ! ${CACHED_BOOST} == "no" ]; then
    if [ ! -e $HOME/boost_1_63_0/boost/version.hpp ]; then
        echo "Downloading boost_1_69_0.built.gcc-8.1.0-buildserver.tar.bz2"
        curl -L "http://www.siusgs.com/slic3r/buildserver/boost_1_69_0.built.gcc-8.1.0-buildserver.tar.bz2" -o /tmp/boost-compiled.tar.bz2
        tar -C$HOME -xjf /tmp/boost-compiled.tar.bz2
    fi
fi

if [ ! ${CACHED_WX} == "no" ]; then
    if [ ! -e ./local-lib/lib/perl5/x86_64-linux-thread-multi/Wx.pm ]; then
        echo "Downloading slic3r-dependencies.gcc81.travis-wx302.tar.bz2"
        curl -L "http://www.siusgs.com/slic3r/buildserver/slic3r-dependencies.gcc81.travis-wx302.tar.bz2" -o /tmp/local-lib-wx302.tar.bz2
        tar -C$TRAVIS_BUILD_DIR -xjf /tmp/local-lib-wx302.tar.bz2
    fi
fi

cpanm local::lib
eval $(perl -Mlocal::lib=${TRAVIS_BUILD_DIR}/local-lib)
CC=${USE_CC} CXX=${USE_CXX} cpanm ExtUtils::CppGuess --force
if [ ! ${CACHED_BOOST} == "no" ]; then
    CC=${USE_CC} CXX=${USE_CXX} BOOST_DIR=$HOME/boost_1_69_0 perl ./Build.PL
else 
    CC=${USE_CC} CXX=${USE_CXX} perl ./Build.PL
fi
excode=$?
if [ $excode -ne 0 ]; then exit $excode; fi
perl ./Build.PL --gui
exit $?

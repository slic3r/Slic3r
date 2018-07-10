#!/bin/bash
# Script to configure travis environment prior to build
CACHE=$HOME/cache
mkdir -p $CACHE
if [[ "$TRAVIS_OS_NAME" == "linux" ]]; then
    if [[ "$WXVERSION"  != "pkg" ]]; then
        export WXDIR=$HOME/wx${WXVERSION}
        if [ ! -e $CACHE/wx${WXVERSION}.tar.bz2 ]; then
            echo "Downloading http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2 => $CACHE/wx${WXVERSION}.tar.bz2"
            curl -L "http://www.siusgs.com/slic3r/buildserver/wx${WXVERSION}-libs.tar.bz2" -o $CACHE/wx${WXVERSION}.tar.bz2
        fi
        tar -C$HOME -xjf $CACHE/wx${WXVERSION}.tar.bz2
    fi

    if [ ! -e $CACHE/boost-compiled.tar.bz2 ]; then
        echo "Downloading http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2 => $CACHE/boost-compiled.tar.bz2"
        curl -L "http://www.siusgs.com/slic3r/buildserver/boost_1_63_0.built.gcc-4.9.4-buildserver.tar.bz2" -o $CACHE/boost-compiled.tar.bz2
    fi
        
    tar -C$HOME -xjf $CACHE/boost-compiled.tar.bz2

elif [[ "$TRAVIS_OS_NAME" == "osx" ]]; then
    if [[ "$WXVERSION" != "pkg" ]]; then
        WXVER_EXPANDED=${WXVERSION:0:1}.${WXVERSION:1:1}.${WXVERSION:2:1}
        export WXDIR=$HOME/wx${WXVERSION}
        if [ ! -e $CACHE/wx${WXVERSION}-${TRAVIS_OS_NAME}.tar.bz2 ]; then
            curl -L "https://github.com/wxWidgets/wxWidgets/releases/download/v${WXVER_EXPANDED}/wxWidgets-${WXVER_EXPANDED}.tar.bz2" -o $HOME/wx${WXVERSION}-src.tar.bz2
            tar -C$HOME -xjf $HOME/wx${WXVERSION}-src.tar.bz2
            mkdir $WXDIR
            cd $HOME/$WXDIR && cmake $HOME/wxWidgets-${WXVER_EXPANDED}  -DwxBUILD_SHARED=OFF
            cmake --build . --target -- -j4
            tar -C$HOME -cjf $CACHE/wx${WXVERSION}-${TRAVIS_OS_NAME}.tar.bz2 $(basename ${WXDIR})
        else 
            tar -C$HOME -xjf $CACHE/wx${WXVERSION}-${TRAVIS_OS_NAME}.tar.bz2
        fi
        export PATH=${PATH}:${WXDIR}
        cd $TRAVIS_BUILD_DIR # go back to the build dir
    else
        brew install wxmac # install via homebrew
    fi
fi

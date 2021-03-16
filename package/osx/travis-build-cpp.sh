#!/bin/bash
set -euo pipefail

export WXVERSION=pkg
export DISPLAY=:99.0

mkdir -p $CACHE

( sudo Xvfb :99 -ac -screen 0 1024x768x8; echo ok )&
brew update -v
brew install ccache || brew upgrade ccache
brew install coreutils || brew upgrade coreutils

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
    brew install wxmac || brew upgrade wxmac # install via homebrew
fi

mkdir build && cd build
cmake -DBOOST_ROOT=$HOME/boost_1_63_0 -DSLIC3R_STATIC=ON -DCMAKE_BUILD_TYPE=Release ../src
cmake --build .
./slic3r_test -s
#./gui_test -s

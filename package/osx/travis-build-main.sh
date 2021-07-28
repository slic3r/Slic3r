#!/bin/bash
set -euo pipefail

# # These two commands are only needed on 10.12:
# rm -rf /usr/local/Homebrew/Library/Taps/homebrew/homebrew-cask
# brew uninstall --force postgis cgal sfcgal

# brew update -v

# brew install boost     || brew upgrade boost
# brew install perl      || brew upgrade perl
# brew install cpanminus || brew upgrade cpanminus
# brew install wxmac@3.0 || brew upgrade wxmac@3.0
# brew install coreutils || brew upgrade coreutils
# brew link --overwrite perl cpanminus

export SLIC3R_STATIC=1
export SLIC3R_NO_CPANM_TESTS=1
export BOOST_DIR=/usr/local
perl ./Build.PL

# remove X11 because otherwise OpenGL.pm will link libglut.3.dylib instead of GLUT.framework
sudo rm -rf /opt/X11*

export LIBRARY_PATH=/usr/local/lib

# One Wx test fails on 10.12; seems harmless
if [ $TRAVIS_OSX_IMAGE == 'xcode9.2' ]; then
    cpanm --local-lib local-lib -f Wx
fi

perl ./Build.PL --gui

# Install PAR::Packer now so that it gets cached by Travis
cpanm --local-lib local-lib PAR::Packer

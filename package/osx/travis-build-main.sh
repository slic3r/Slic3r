#!/bin/bash
set -euo pipefail

brew update -v
brew install boost perl cpanminus wxwidgets
brew link --overwrite perl cpanminus

export SLIC3R_STATIC=1
export BOOST_DIR=/usr/local
#perl ./Build.PL
cpanm -v --reinstall --local-lib local-lib Alien::wxWidgets
cpanm -v --reinstall --local-lib local-lib Wx
perl ./Build.PL --gui

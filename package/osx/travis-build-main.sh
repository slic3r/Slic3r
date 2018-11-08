#!/bin/bash
set -euo pipefail

brew update -v
brew install boost perl cpanminus wxwidgets
brew link --overwrite perl cpanminus

export SLIC3R_STATIC=1
export BOOST_DIR=/usr/local
perl ./Build.PL

export LIBRARY_PATH=/usr/local/lib
perl ./Build.PL --gui

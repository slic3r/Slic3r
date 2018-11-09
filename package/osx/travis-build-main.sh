#!/bin/bash
set -euo pipefail

brew update -v
brew install boost     || brew upgrade boost
brew install perl      || brew upgrade perl
brew install cpanminus || brew upgrade cpanminus
brew install wxwidgets || brew upgrade wxwidgets
brew link --overwrite perl cpanminus

export SLIC3R_STATIC=1
export BOOST_DIR=/usr/local
perl ./Build.PL

export LIBRARY_PATH=/usr/local/lib
perl ./Build.PL --gui

# Install PAR::Packer now so that it gets cached by Travis
cpanm --local-lib local-lib PAR::Packer

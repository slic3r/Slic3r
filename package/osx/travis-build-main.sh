#!/bin/bash
set -euo pipefail

brew update -v
brew upgrade boost     || brew install boost
brew upgrade perl      || brew install perl
brew upgrade cpanminus || brew install cpanminus
brew upgrade wxwidgets || brew install wxwidgets
brew link --overwrite perl cpanminus

export SLIC3R_STATIC=1
export BOOST_DIR=/usr/local
perl ./Build.PL

export LIBRARY_PATH=/usr/local/lib
perl ./Build.PL --gui

# Install PAR::Packer now so that it gets cached by Travis
cpanm --local-lib local-lib PAR::Packer

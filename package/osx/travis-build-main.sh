#!/bin/bash
set -euo pipefail

brew update -v
brew install boost perl cpanminus

export SLIC3R_STATIC=1
export BOOST_DIR=/usr/local
perl ./Build.PL

# Install GUI modules and wxWidgets:

curl -O https://gist.githubusercontent.com/alranel/c2de82c05f6006b49c5029fc78bcaa87/raw/778dec2692408334ef2b87818bf977ac6f9fd8ee/patch-wxwidgets-noquicktime.diff

PERL_USE_UNSAFE_INC=1 \
    CXX="clang++ -mmacosx-version-min=10.12" \
    CPPFLAGS="-mmacosx-version-min=10.12 -D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=1" \
    LDFLAGS="-mmacosx-version-min=10.12" \
    cpanm --local-lib local-lib Alien::wxWidgets -v --reinstall \
        --configure-args="--wxWidgets-build=1 --wxWidgets-extraflags=\"--with-macosx-version-min=10.12 --disable-qtkit --disable-mediactrl --disable-webkit --disable-webview\" --wxWidgets-userpatch=$(pwd)/patch-wxwidgets-noquicktime.diff"

PERL_USE_UNSAFE_INC=1 cpanm --local-lib local-lib --reinstall -v https://github.com/alranel/wxPerl-osx10.12/tarball/master

perl ./Build.PL --gui

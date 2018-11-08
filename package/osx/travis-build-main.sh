#!/bin/bash
set -euo pipefail

brew update -v
brew install boost perl cpanminus

export SLIC3R_STATIC=1
export BOOST_DIR=/usr/local
perl ./Build.PL

# Only recompile Wx if it's not already there
if [ ! -e ./local-lib/lib/perl5/darwin-thread-multi-2level/Wx.pm ]; then
    # our patch-wxwidgets.diff assumes Alien::wxWidgets installs wxWidgets 3.0.2
    PERL_USE_UNSAFE_INC=1 \
        CXX="clang++ -mmacosx-version-min=10.12" \
        CPPFLAGS="-mmacosx-version-min=10.12 -D__ASSERT_MACROS_DEFINE_VERSIONS_WITHOUT_UNDERSCORES=1" \
        LDFLAGS="-mmacosx-version-min=10.12" \
        cpanm --local-lib local-lib Alien::wxWidgets --reinstall \
            --configure-args="--wxWidgets-build=1 --wxWidgets-extraflags=\"--with-macosx-version-min=10.12 --disable-qtkit --disable-mediactrl --disable-webkit --disable-webview\" --wxWidgets-userpatch=$(pwd)/package/osx/patch-wxwidgets.diff"

    PERL_USE_UNSAFE_INC=1 cpanm --local-lib local-lib --reinstall -v https://github.com/alranel/wxPerl-osx10.12/tarball/master
fi

perl ./Build.PL --gui

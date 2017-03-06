#!/bin/bash

# Assembles an installation bundle from a built copy of Slic3r.
# Requires PAR::Packer to be installed for the version of
# perl copied.
# Adapted from script written by bubnikv for Prusa3D.
# Run from slic3r repo root directory.
SLIC3R_VERSION=$(grep "VERSION" xs/src/libslic3r/libslic3r.h | awk -F\" '{print $2}')

if [ "$#" -ne 1 ]; then
    echo "Usage: $(basename $0) dmg_name"
    exit 1;
fi

WD=$(dirname $0)
# Determine if this is a tagged (release) commit.
# Change the build id accordingly.
if [ $(git describe &>/dev/null) ]; then
    TAGGED=true
    SLIC3R_BUILD_ID=$(git describe)
else
    TAGGED=false
    SLIC3R_BUILD_ID=${SLIC3R_VERSION}
fi
if [ -z ${GIT_BRANCH+x} ] && [ -z ${APPVEYOR_REPO_BRANCH+x} ]; then
    current_branch=$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
else
    current_branch="unknown"
    if [ ! -z ${GIT_BRANCH+x} ]; then
        echo "Setting to GIT_BRANCH"
        current_branch=$(echo $GIT_BRANCH | cut -d / -f 2)
    fi
    if [ ! -z ${APPVEYOR_REPO_BRANCH+x} ]; then
        echo "Setting to APPVEYOR_REPO_BRANCH"
        current_branch=$APPVEYOR_REPO_BRANCH
    fi
fi

# If we're on a branch, add the branch name to the app name.
if [ "$current_branch" == "master" ]; then
    appname=Slic3r
    dmgfile=slic3r-${SLIC3R_BUILD_ID}-${1}.dmg
else
    appname=Slic3r-$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
    dmgfile=slic3r-${SLIC3R_BUILD_ID}-${1}-$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!').dmg
fi
rm -rf $WD/_tmp
mkdir -p $WD/_tmp


# OSX Application folder shenanigans.
appfolder="$WD/${appname}.app"
macosfolder=$appfolder/Contents/MacOS
resourcefolder=$appfolder/Contents/Resources
plistfile=$appfolder/Contents/Info.plist
PkgInfoContents="APPL????"
source $WD/plist.sh

# Our slic3r dir and location of perl
PERL_BIN=$(which perl)
PP_BIN=$(which pp)
SLIC3R_DIR=$(perl -MCwd=realpath -e "print realpath '${WD}/../../'")

if [[ -d "${appfolder}" ]]; then
    echo "Deleting old working folder."
    rm -rf ${appfolder}
fi

if [[ -e "${dmgfile}" ]]; then
    echo "Deleting old dmg ${dmgfile}."
    rm -rf ${dmgfile}
fi

echo "Creating new app folder at $appfolder."
mkdir -p $appfolder 
mkdir -p $macosfolder
mkdir -p $resourcefolder

echo "Copying resources..." 
cp -r $SLIC3R_DIR/var $macosfolder/
mv $macosfolder/var/Slic3r.icns $resourcefolder

echo "Copying Slic3r..."
cp $SLIC3R_DIR/slic3r.pl $macosfolder/slic3r.pl
cp -RP $SLIC3R_DIR/local-lib $macosfolder/local-lib
cp -RP $SLIC3R_DIR/lib/* $macosfolder/local-lib/lib/perl5/
find $macosfolder/local-lib -name man -type d -delete
find $macosfolder/local-lib -name .packlist -delete
rm -rf $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets/osx_cocoa_3_0_2_uni/include

echo "Relocating dylib paths..."
for bundle in $(find $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/auto/Wx -name '*.bundle') $(find $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets -name '*.dylib' -type f); do
    chmod +w $bundle
    find $SLIC3R_DIR/local-lib -name '*.dylib' -exec bash -c 'install_name_tool -change "{}" "@executable_path/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets/osx_cocoa_3_0_2_uni/lib/$(basename {})" '$bundle \;
done

echo "Copying startup script..."
cp $WD/startup_script.sh $macosfolder/$appname
chmod +x $macosfolder/$appname

echo "Copying perl from $PERL_BIN"
cp $PERL_BIN $macosfolder/perl-local
${PP_BIN} -M attributes -M base -M bytes -M B -M POSIX \
          -M FindBin -M Unicode::Normalize -M Tie::Handle \
          -M Time::Local -M Math::Trig \
          -M lib -M overload \
          -M warnings -M local::lib \
          -M strict -M utf8 -M parent \
          -B -p -e "print 123" -o $WD/_tmp/test.par
unzip $WD/_tmp/test.par -d $WD/_tmp/
cp -r $WD/_tmp/lib/* $macosfolder/local-lib/lib/perl5/
rm -rf $WD/_tmp

make_plist

echo $PkgInfoContents >$appfolder/Contents/PkgInfo

echo "Creating dmg file...."
hdiutil create -fs HFS+ -srcfolder "$appfolder" -volname "$appname" "$dmgfile"

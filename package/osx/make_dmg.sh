#!/bin/bash

# Assembles an installation bundle from a built copy of Slic3r.
# Requires PAR::Packer to be installed for the version of
# perl copied.
# Adapted from script written by bubnikv for Prusa3D.
# Required environment variables:
# SLIC3R_VERSION - x.x.x format

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
    SLIC3R_BUILD_ID=${SLIC3R_VERSION}d
fi

# If we're on a branch, add the branch name to the app name.
if [ "$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')" == "master" ]; then
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
SLIC3R_DIR="${WD}/../../"

if [[ -d "${appfolder}" ]]; then
    echo "Deleting old working folder."
    rm -rf ${appfolder}
fi

if [[ -d "${dmgfile}" ]]; then
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
cp -r $SLIC3R_DIR/local-lib $macosfolder/local-lib
cp -r $SLIC3R_DIR/lib/* $macosfolder/local-lib/lib/perl5/

echo "Copying startup script..."
cp $WD/startup_script.sh $macosfolder/$appname
chmod +x $macosfolder/$appname

echo "Copying perl from $PERL_BIN"
cp $PERL_BIN $macosfolder/perl-local
${PP_BIN} -M POSIX -M FindBin \
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

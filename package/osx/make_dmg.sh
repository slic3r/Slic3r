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
    echo "Deleting old working folder: ${appfolder}"
    rm -rf ${appfolder}
fi

if [[ -e "${dmgfile}" ]]; then
    echo "Deleting old dmg: ${dmgfile}"
    rm -rf ${dmgfile}
fi

echo "Creating new app folder: $appfolder"
mkdir -p $appfolder 
mkdir -p $macosfolder
mkdir -p $resourcefolder

echo "Copying resources..." 
cp -rf $SLIC3R_DIR/var $resourcefolder/
mv $resourcefolder/var/Slic3r.icns $resourcefolder

echo "Copying Slic3r..."
cp $SLIC3R_DIR/slic3r.pl $macosfolder/slic3r.pl
cp -fRP $SLIC3R_DIR/local-lib $macosfolder/local-lib
cp -fRP $SLIC3R_DIR/lib/* $macosfolder/local-lib/lib/perl5/

echo "Relocating dylib paths..."
for bundle in $(find $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/auto/Wx -name '*.bundle') $(find $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets -name '*.dylib' -type f); do
    chmod +w $bundle
    for dylib in $(otool -l $bundle | grep .dylib | grep local-lib | awk '{print $2}'); do
        install_name_tool -change "$dylib" "@executable_path/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets/osx_cocoa_3_0_2_uni/lib/$(basename $dylib)" $bundle
    done
done

echo "Copying startup script..."
cp -f $WD/startup_script.sh $macosfolder/$appname
chmod +x $macosfolder/$appname

echo "Copying perl from $PERL_BIN"
# Edit package/common/coreperl to add/remove core Perl modules added to this package, one per line.
cp -f $PERL_BIN $macosfolder/perl-local
${PP_BIN} \
          -M $(grep -v "^#" ${WD}/../common/coreperl | xargs | awk 'BEGIN { OFS=" -M "}; {$1=$1; print $0}') \
          -B -p -e "print 123" -o $WD/_tmp/test.par
unzip -o $WD/_tmp/test.par -d $WD/_tmp/
cp -rf $WD/_tmp/lib/* $macosfolder/local-lib/lib/perl5/
rm -rf $WD/_tmp

echo "Cleaning bundle"
rm -rf $macosfolder/local-lib/bin
rm -rf $macosfolder/local-lib/man
rm -f $macosfolder/local-lib/lib/perl5/Algorithm/*.pl
rm -rf $macosfolder/local-lib/lib/perl5/unicore
rm -rf $macosfolder/local-lib/lib/perl5/App
rm -rf $macosfolder/local-lib/lib/perl5/Devel/CheckLib.pm
rm -rf $macosfolder/local-lib/lib/perl5/ExtUtils
rm -rf $macosfolder/local-lib/lib/perl5/Module/Build*
rm -rf $macosfolder/local-lib/lib/perl5/TAP
rm -rf $macosfolder/local-lib/lib/perl5/Test*
find -d $macosfolder/local-lib -name '*.pod' -delete
find -d $macosfolder/local-lib -name .packlist -delete
find -d $macosfolder/local-lib -name .meta -exec rm -rf "{}" \;
find -d $macosfolder/local-lib -name '*.h' -delete
find -d $macosfolder/local-lib -name wxPerl.app -exec rm -rf "{}" \;
find -d $macosfolder/local-lib -type d -path '*/Wx/*' \( -name WebView \
    -or -name DocView -or -name STC -or -name IPC \
    -or -name Calendar -or -name DataView \
    -or -name DateTime -or -name Media -or -name PerlTest \
    -or -name Ribbon \) -exec rm -rf "{}" \;
find -d $macosfolder/local-lib -name libwx_osx_cocoau_ribbon-3.0.0.2.0.dylib -delete
find -d $macosfolder/local-lib -name libwx_osx_cocoau_stc-3.0.0.2.0.dylib -delete
find -d $macosfolder/local-lib -name libwx_osx_cocoau_webview-3.0.0.2.0.dylib -delete
rm -rf $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets/osx_cocoa_3_0_2_uni/include
find -d $macosfolder/local-lib -type d -empty -delete

make_plist

echo $PkgInfoContents >$appfolder/Contents/PkgInfo

if [[ -e "${KEYCHAIN_FILE}" ]]; then
    echo "Signing app..."
    chmod -R +w $macosfolder/*
    security list-keychains -s "${KEYCHAIN_FILE}"
    security default-keychain -s "${KEYCHAIN_FILE}"
    security unlock-keychain -p "${KEYCHAIN_PASSWORD}" "${KEYCHAIN_FILE}"
    codesign --sign "${KEYCHAIN_IDENTITY}" --deep "$appfolder"
fi

echo "Creating dmg file...."
hdiutil create -fs HFS+ -srcfolder "$appfolder" -volname "$appname" "$dmgfile"

#!/bin/bash
set -euo pipefail

# Assembles an installation bundle from a built copy of Slic3r.
# Requires PAR::Packer to be installed for the version of
# perl copied.
# Adapted from script written by bubnikv for Prusa3D.
# Run from slic3r repo root directory.

# While we might have a pp executable in our path, it might not be
# using the perl binary we have in path, so make sure they belong
# to the same Perl instance:

source $(dirname $0)/../common/util.sh
if [ $# -lt 1 ]; then
    set_source_dir
else
    set_source_dir $1
fi
set_version
get_commit
set_build_id
set_branch
set_app_name
set_pr_id


if !(perl -Mlocal::lib=local-lib -MPAR::Packer -e1 2> /dev/null); then
    echo "The PAR::Packer module was not found; installing..."
    cpanm --local-lib local-lib PAR::Packer
fi

WD=$(dirname $0)
appname=Slic3r

# Determine if this is a tagged (release) commit.
# Change the build id accordingly.
if [ $(git describe --exact-match &>/dev/null) ]; then
    echo "This is a tagged build"
    SLIC3R_BUILD_ID=$(git describe)
else
    # Get the current branch
    if [ ! -z ${GIT_BRANCH+x} ]; then
        echo "Setting to GIT_BRANCH"
        current_branch=$(echo $GIT_BRANCH | cut -d / -f 2)
    elif [ ! -z ${TRAVIS_BRANCH+x} ]; then
        echo "Setting to TRAVIS_BRANCH"
        current_branch=$TRAVIS_BRANCH
    elif [ ! -z ${APPVEYOR_REPO_BRANCH+x} ]; then
        echo "Setting to APPVEYOR_REPO_BRANCH"
        current_branch=$APPVEYOR_REPO_BRANCH
    else
        current_branch=$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
    fi
    
    if [ "$current_branch" == "master" ]; then
        echo "This is a build of the master branch"
        SLIC3R_VERSION=$(grep "VERSION" xs/src/libslic3r/libslic3r.h | awk -F\" '{print $2}')
        SLIC3R_BUILD_ID=${SLIC3R_VERSION}-$(git rev-parse --short HEAD)
    else
        echo "This is a build of a non-master branch"
        appname=Slic3r-${current_branch}
        SLIC3R_BUILD_ID=${current_branch}-$(git rev-parse --short HEAD)
    fi
fi

if [ "$current_branch" == "master" ]; then
    if [ ! -z ${PR_ID+x} ]; then
        dmgfile=slic3r-${SLIC3R_BUILD_ID}-osx-PR${PR_ID}.dmg
    else
        dmgfile=slic3r-${SLIC3R_BUILD_ID}-osx.dmg
    fi
else
    dmgfile=slic3r-${SLIC3R_BUILD_ID}-osx-${current_branch}.dmg
fi

echo "DMG filename: ${dmgfile}"

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
eval $(perl -Mlocal::lib=local-lib)
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
mv $resourcefolder/var/stl.icns $resourcefolder
mv $resourcefolder/var/gcode.icns $resourcefolder

echo "Copying Slic3r..."
cp $SLIC3R_DIR/slic3r.pl $macosfolder/slic3r.pl
cp -fRP $SLIC3R_DIR/local-lib $macosfolder/local-lib
cp -fRP $SLIC3R_DIR/lib/* $macosfolder/local-lib/lib/perl5/

echo "Relocating Wx dylib paths..."
mkdir $macosfolder/dylibs
function relocate_dylibs {
    local bundle=$1
    chmod +w $bundle
    local dylib
    for dylib in $(otool -l $bundle | grep .dylib | grep -v /usr/lib | awk '{print $2}' | grep -v '^@'); do
        local dylib_dest="$macosfolder/dylibs/$(basename $dylib)"
        if [ ! -e $dylib_dest ]; then
            echo "  relocating $dylib"
            cp $dylib $macosfolder/dylibs/
            relocate_dylibs $dylib_dest
        fi
        install_name_tool -change "$dylib" "@executable_path/dylibs/$(basename $dylib)" $bundle
    done
}
for bundle in $(find $macosfolder/local-lib/ \( -name '*.bundle' -or -name '*.dylib' \) -type f); do
    relocate_dylibs "$bundle"
done

echo "Copying startup script..."
cp -f $WD/startup_script.sh $macosfolder/$appname
chmod +x $macosfolder/$appname

echo "Copying perl from $PERL_BIN"
cp -f $PERL_BIN $macosfolder/perl-local
chmod +w $macosfolder/perl-local
relocate_dylibs $macosfolder/perl-local

echo "Copying core modules"
# Edit package/common/coreperl to add/remove core Perl modules added to this package, one per line.
${PP_BIN} \
          -M $(grep -v "^#" ${WD}/../common/coreperl | xargs | awk 'BEGIN { OFS=" -M "}; {$1=$1; print $0}') \
          -B -p -e "print 123" -o $WD/_tmp/bundle.par
unzip -o $WD/_tmp/bundle.par -d $WD/_tmp/
cp -rf $WD/_tmp/lib/* $macosfolder/local-lib/lib/perl5/

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
find -d $macosfolder/local-lib -name libwx_osx_cocoau_ribbon-3.* -delete
find -d $macosfolder/local-lib -name libwx_osx_cocoau_stc-3.* -delete
find -d $macosfolder/local-lib -name libwx_osx_cocoau_webview-3.* -delete
rm -rf $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets/osx_cocoa_3_0_2_uni/include
find -d $macosfolder/local-lib -type d -empty -delete

# remove wx build tools
rm -rf $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets/osx_cocoa_3_0_2_uni/bin

# Remove all broken symlinks
find -L $macosfolder/local-lib/lib/perl5/darwin-thread-multi-2level/Alien/wxWidgets/osx_cocoa_3_0_2_uni -type l -exec rm {} \;

make_plist

echo $PkgInfoContents >$appfolder/Contents/PkgInfo

KEYCHAIN_FILE_=${KEYCHAIN_FILE:-}
KEYCHAIN_BASE64_=${KEYCHAIN_BASE64:-}
KEYCHAIN_PASSWORD_=${KEYCHAIN_PASSWORD:-travis}
KEYCHAIN_IDENTITY_=${KEYCHAIN_IDENTITY:-Developer ID Application: Alessandro Ranellucci (975MZ9YJL7)}

# In case we were supplied a base64-encoded .p12 file instead of the path
# to an existing keychain, create a temporary one
if [[ -z "$KEYCHAIN_FILE_" && ! -z "$KEYCHAIN_BASE64_" ]]; then
    KEYCHAIN_FILE_=$WD/_tmp/build.keychain
    echo "Creating temporary keychain at ${KEYCHAIN_FILE_}"
    echo "$KEYCHAIN_BASE64_" | base64 --decode > "${KEYCHAIN_FILE_}.p12"
    security delete-keychain "$KEYCHAIN_FILE_" || true
    security create-keychain -p "${KEYCHAIN_PASSWORD_}" "$KEYCHAIN_FILE_"
    security set-keychain-settings -t 3600 -u "$KEYCHAIN_FILE_"
    security import "${KEYCHAIN_FILE_}.p12" -k "$KEYCHAIN_FILE_" -P "${KEYCHAIN_PASSWORD_}" -T /usr/bin/codesign
    security set-key-partition-list -S apple-tool:,apple: -s -k "${KEYCHAIN_PASSWORD_}" "$KEYCHAIN_FILE_"
fi

if [ ! -z $KEYCHAIN_FILE_ ]; then
    echo "Signing app..."
    chmod -R +w $macosfolder/*
    xattr -cr $appfolder
    security list-keychains -s "${KEYCHAIN_FILE_}"
    security default-keychain -s "${KEYCHAIN_FILE_}"
    security unlock-keychain -p "${KEYCHAIN_PASSWORD_}" "${KEYCHAIN_FILE_}"
    codesign --sign "${KEYCHAIN_IDENTITY_}" --options=runtime --strict --deep "$appfolder"
else
    echo "No KEYCHAIN_FILE or KEYCHAIN_BASE64 env variable; skipping codesign"
fi

echo "Creating dmg file...."
hdiutil create -fs HFS+ -srcfolder "$appfolder" -volname "$appname" "$WD/_tmp/$dmgfile"

# Compress the DMG image
hdiutil convert "$WD/_tmp/$dmgfile" -format UDZO -imagekey zlib-level=9 -o "$dmgfile"

if [ ! -z $KEYCHAIN_FILE_ ]; then
    echo "Signing app dmg..."
    chmod +w $dmgfile
    security list-keychains -s "${KEYCHAIN_FILE_}"
    security default-keychain -s "${KEYCHAIN_FILE_}"
    security unlock-keychain -p "${KEYCHAIN_PASSWORD_}" "${KEYCHAIN_FILE_}"
    codesign --sign "${KEYCHAIN_IDENTITY_}" --options=runtime --strict "$dmgfile"
fi

rm -rf $WD/_tmp

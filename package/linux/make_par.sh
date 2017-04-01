#!/bin/bash

# Assembles an installation bundle from a built copy of Slic3r.
# Requires PAR::Packer to be installed for the version of
# perl copied.
# Adapted from script written by bubnikv for Prusa3D.
# Run from slic3r repo root directory.
SLIC3R_VERSION=$(grep "VERSION" xs/src/libslic3r/libslic3r.h | awk -F\" '{print $2}')

if [ "$#" -ne 1 ]; then
    echo "Usage: $(basename $0) arch_name"
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
    dmgfile=slic3r-${SLIC3R_BUILD_ID}-${1}.tar.bz2
else
    appname=Slic3r-$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
    dmgfile=slic3r-${SLIC3R_BUILD_ID}-${1}-$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!').tar.bz2
fi
rm -rf $WD/_tmp
mkdir -p $WD/_tmp

# OSX Application folder shenanigans.
appfolder="$WD/${appname}"
archivefolder=$appfolder
resourcefolder=$appfolder


# Our slic3r dir and location of perl
PERL_BIN=$(which perl)
PP_BIN=$(which pp)
SLIC3R_DIR=$(readlink -f ${archivefolder}/../../../)

if [[ -d "${appfolder}" ]]; then
    echo "Deleting old working folder: ${appfolder}"
    rm -rf ${appfolder}
fi

if [[ -e "${dmgfile}" ]]; then
    echo "Deleting old archive: ${dmgfile}"
    rm -rf ${dmgfile}
fi

echo "Creating new app folder: $appfolder"
mkdir -p $appfolder 

echo "Copying resources..." 
cp -rf $SLIC3R_DIR/var $resourcefolder/
mv $resourcefolder/var/Slic3r.icns $resourcefolder

echo "Copying Slic3r..."
cp $SLIC3R_DIR/slic3r.pl $archivefolder/slic3r.pl
cp -fRP $SLIC3R_DIR/local-lib $archivefolder/local-lib
cp -fRP $SLIC3R_DIR/lib/* $archivefolder/local-lib/lib/perl5/

mkdir $archivefolder/bin
echo "Symlinking libraries to root/bin..."
for bundle in $(find $archivefolder/local-lib/lib/perl5/*/auto/Wx -name '*.so') $(find $archivefolder/local-lib/lib/perl5/*/Alien/wxWidgets -name '*.so' -type f); do
    chmod +w $bundle
    for dylib in $(ldd $bundle | grep .so | grep local-lib | awk '{print $3}'); do
	install $dylib $archivefolder/bin
    done
done

echo "Copying startup script..."
cp -f $WD/startup_script.sh $archivefolder/$appname
chmod +x $archivefolder/$appname

echo "Copying perl from $PERL_BIN"
cp -f $PERL_BIN $archivefolder/perl-local
${PP_BIN} -M attributes -M base -M bytes -M B -M POSIX \
          -M FindBin -M Unicode::Normalize -M Tie::Handle \
          -M Time::Local -M Math::Trig \
          -M lib -M overload \
          -M warnings -M local::lib \
          -M strict -M utf8 -M parent \
          -B -p -e "print 123" -o $WD/_tmp/test.par
unzip -o $WD/_tmp/test.par -d $WD/_tmp/
cp -rf $WD/_tmp/lib/* $archivefolder/local-lib/lib/perl5/
rm -rf $WD/_tmp

echo "Cleaning local-lib"
rm -rf $archivefolder/local-lib/bin
rm -rf $archivefolder/local-lib/man
rm -f $archivefolder/local-lib/lib/perl5/Algorithm/*.pl
rm -rf $archivefolder/local-lib/lib/perl5/unicore
rm -rf $archivefolder/local-lib/lib/perl5/App
rm -rf $archivefolder/local-lib/lib/perl5/Devel/CheckLib.pm
rm -rf $archivefolder/local-lib/lib/perl5/ExtUtils
rm -rf $archivefolder/local-lib/lib/perl5/Module/Build*
rm -rf $(pwd)$archivefolder/local-lib/lib/perl5/TAP
rm -rf $(pwd)/$archivefolder/local-lib/lib/perl5/Test*
find $(pwd)/$archivefolder/local-lib -type d -path '*/Wx/*' \( -name WebView \
    -or -name DocView -or -name STC -or -name IPC \
    -or -name AUI -or -name Calendar -or -name DataView \
    -or -name DateTime -or -name Media -or -name PerlTest \
    -or -name Ribbon \) -exec rm -rf "{}" \;
rm -rf $archivefolder/local-lib/lib/perl5/*/Alien/wxWidgets/*/include
find $archivefolder/local-lib -depth -type d -empty -exec rmdir "{}" \;

tar -C$(pwd)/$(dirname $appfolder) -cvjf $(pwd)/$dmgfile "$appname"

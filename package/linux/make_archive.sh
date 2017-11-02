#!/bin/bash

# Assembles an installation archive from a built copy of Slic3r.
# Requires PAR::Packer to be installed for the version of
# perl copied.
# Adapted from script written by bubnikv for Prusa3D.
# Run from slic3r repo root directory.

if [ "$#" -ne 1 ]; then
    echo "Usage: $(basename $0) arch_name"
    exit 1;
fi
libdirs=$(find ./local-lib -iname *.so -exec dirname {} \; | sort -u | paste -sd ";" -)
WD=./$(dirname $0)
source $(dirname $0)/../common/util.sh
# Determine if this is a tagged (release) commit.
# Change the build id accordingly.

set_version
get_commit
set_build_id
set_branch
set_app_name
set_pr_id
install_par

# If we're on a branch, add the branch name to the app name.
if [ "$current_branch" == "master" ]; then
    if [ ! -z ${PR_ID+x} ]; then
        dmgfile=slic3r-${SLIC3R_BUILD_ID}-${1}-PR${PR_ID}.tar.bz2
    else
        dmgfile=slic3r-${SLIC3R_BUILD_ID}-${1}.tar.bz2
    fi
else
    dmgfile=slic3r-${SLIC3R_BUILD_ID}-${1}-${current_branch}.tar.bz2
fi

rm -rf $WD/_tmp
mkdir -p $WD/_tmp

# Set  the application folder infomation.
appfolder="$WD/${appname}"
archivefolder=$appfolder
resourcefolder=$appfolder

echo "Appfolder: $appfolder, archivefolder: $archivefolder"

# Our slic3r dir and location of perl
PERL_BIN=$(which perl)
PP_BIN=$(which pp)
SLIC3R_DIR="./"

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

echo "Copying Slic3r..."
cp $SLIC3R_DIR/slic3r.pl $archivefolder/slic3r.pl
cp -fRP $SLIC3R_DIR/local-lib $archivefolder/local-lib
cp -fRP $SLIC3R_DIR/lib/* $archivefolder/local-lib/lib/perl5/

mkdir $archivefolder/bin
echo "Installing libraries to $archivefolder/bin ..."
if [ -z ${WXDIR+x} ]; then
    for bundle in $(find $archivefolder/local-lib/lib/perl5 -name '*.so' | grep "Wx") $(find $archivefolder/local-lib/lib/perl5 -name '*.so' -type f | grep "wxWidgets"); do
        echo "$(LD_LIBRARY_PATH=$libdirs ldd $bundle | grep .so | grep local-lib | awk '{print $3}')"
        for dylib in $(LD_LIBRARY_PATH=$libdirs ldd $bundle | grep .so | grep local-lib | awk '{print $3}'); do
            install -v $dylib $archivefolder/bin
        done
    done
else
    echo "Copying libraries from $WXDIR/lib to $archivefolder/bin"
    for dylib in $(find $WXDIR/lib | grep "so"); do
        cp -P -v $dylib $archivefolder/bin
    done
fi

echo "Copying startup script..."
cp -f $WD/startup_script.sh $archivefolder/$appname
chmod +x $archivefolder/$appname

echo "Copying perl from $PERL_BIN"
# Edit package/common/coreperl to add/remove core Perl modules added to this package, one per line.
cp -f $PERL_BIN $archivefolder/perl-local
${PP_BIN} wxextension .0 \
	  -M $(grep -v "^#" ${WD}/../common/coreperl | xargs | awk 'BEGIN { OFS=" -M "}; {$1=$1; print $0}') \
          -B -p -e "print 123" -o $WD/_tmp/test.par
unzip -qq -o $WD/_tmp/test.par -d $WD/_tmp/
cp -rf $WD/_tmp/lib/* $archivefolder/local-lib/lib/perl5/
cp -rf $WD/_tmp/shlib $archivefolder/
rm -rf $WD/_tmp
for i in $(cat $WD/libpaths.txt | grep -v "^#" | awk -F# '{print $1}'); do 
	install -v $i $archivefolder/bin
done

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
    -or -name Calendar -or -name DataView \
    -or -name DateTime -or -name Media -or -name PerlTest \
    -or -name Ribbon \) -exec rm -rf "{}" \;
rm -rf $archivefolder/local-lib/lib/perl5/*/Alien/wxWidgets/*/include
find $archivefolder/local-lib -depth -type d -empty -exec rmdir "{}" \;

tar -C$(pwd)/$(dirname $appfolder) -cjf $(pwd)/$dmgfile "$appname"

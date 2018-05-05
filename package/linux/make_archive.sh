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

mkdir -p $WD

# Set  the application folder infomation.
appfolder="$WD/${appname}"
archivefolder=$appfolder
resourcefolder=$appfolder

echo "Appfolder: $appfolder, archivefolder: $archivefolder"

# Our slic3r dir and location of perl
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
cp $SLIC3R_DIR/slic3r $archivefolder/Slic3r

mkdir $archivefolder/bin
echo "Installing libraries to $archivefolder/bin ..."
if [ -z ${WXDIR+x} ]; then
    for bundle in $archivefolder/Slic3r; do
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

for i in $(cat $WD/libpaths.txt | grep -v "^#" | awk -F# '{print $1}'); do 
	install -v $i $archivefolder/bin
done

tar -C$(pwd)/$(dirname $appfolder) -cjf $(pwd)/$dmgfile "$appname"

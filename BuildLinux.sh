#!/bin/bash

export ROOT=`pwd`
export NCORES=`nproc --all`


while getopts ":dsiuh" opt; do
  case ${opt} in
    u )
        UPDATE_LIB="1"
        ;;
    i )
        BUILD_IMAGE="1"
        ;;
    d )
        BUILD_DEPS="1"
        ;;
    s )
        BUILD_SLIC3R="1"
        ;;
    h ) echo "Usage: ./BuildLinux.sh [-i][-u][-d][-s]"
        echo "   -i: Generate appimage (optional)"
        echo "   -d: build deps (optional)"
        echo "   -s: build slic3r (optional)"
        echo "   -u: only update clock & dependency packets (optional and need sudo)"
        echo "For a first use, you want to 'sudo ./BuildLinux.sh -u'"
        echo "   and then './BuildLinux.sh -dsi'"
        exit 0
        ;;
  esac
done

# mkdir build
if [ ! -d "build" ]
then
    mkdir build
fi

if [[ -n "$UPDATE_LIB" ]]
then
    echo -n "Updating linux ..."
    hwclock -s
    apt update
    apt install libgtk2.0-dev libglew-dev libudev-dev libdbus-1-dev cmake git
    echo "done"
    exit 0
fi

FOUND_GTK2=$(dpkg -l libgtk* | grep gtk2.0-dev)
echo "FOUND_GTK2=$FOUND_GTK2)"
if [[ -z "$FOUND_GTK2" ]]
then
    echo "Error, you must install the dependencies before."
    echo "Use option -u with sudo"
    exit 0
fi

echo "[1/9] Updating submodules..."
{
    # update submodule profiles
    pushd resources/profiles
    git submodule update --init
    popd
}
# > $ROOT/build/Build.log # Capture all command output

echo "[2/9] Changing date in version..."
{
    # change date in version
    sed -i "s/+UNKNOWN/_$(date '+%F')/" version.inc
}
# &> $ROOT/build/Build.log # Capture all command output
echo "done"

# mkdir in deps
if [ ! -d "deps/build" ]
then
    mkdir deps/build
fi

if [[ -n "$BUILD_DEPS" ]]
then
    echo "[3/9] Configuring dependencies..."
    
        # cmake deps
        pushd deps/build
        cmake ..
    
    # &> $ROOT/build/Build.log # Capture all command output
    echo "done"
    
    echo "[4/9] Building dependencies..."
    
        # make deps
        make -j$NCORES
    
    # &> $ROOT/build/Build.log # Capture all command output
    echo "done"
    
    echo "[5/9] Renaming wxscintilla library..."
    
        # rename wxscintilla
        pushd destdir/usr/local/lib
        cp libwxscintilla-3.1.a libwx_gtk2u_scintilla-3.1.a
        popd
    # &> $ROOT/build/Build.log # Capture all command output
    echo "done"
    
    echo "[6/9] Cleaning dependencies..."
    
        # clean deps
        rm -rf dep_*
        popd
    # &> $ROOT/build/Build.log # Capture all command output
    echo "done"
fi

if [[ -n "$BUILD_SLIC3R" ]]
then
    echo "[7/9] Configuring Slic3r..."
    
        # cmake
        pushd build
        cmake .. -DCMAKE_PREFIX_PATH="$PWD/../deps/build/destdir/usr/local" -DSLIC3R_STATIC=1
    # &> $ROOT/build/Build.log # Capture all command output
    echo "done"
    
    echo "[8/9] Building Slic3r..."
    
        # make Slic3r
        make -j$NCORES Slic3r
    
        # make .mo
        make gettext_po_to_mo
        
        popd
    # &> $ROOT/build/Build.log # Capture all command output
    echo "done"
fi

# Give proper permissions to script
chmod 755 $ROOT/build/src/BuildLinuxImage.sh

echo "[9/9] Generating Linux app..."
    pushd build
    if [[ -n "$BUILD_IMAGE" ]]
    then
        $ROOT/build/src/BuildLinuxImage.sh -i
    else
        $ROOT/build/src/BuildLinuxImage.sh
    fi
# &> $ROOT/build/Build.log # Capture all command output
echo "done"

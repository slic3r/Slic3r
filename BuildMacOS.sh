#!/bin/bash

export ROOT=`pwd`
export NCORES=`sysctl -n hw.ncpu`
export CMAKE_INSTALLED=`which cmake`
#export ARCH=$(uname -m)

# Check if CMake is installed
if [[ -z "$CMAKE_INSTALLED" ]]
then
    echo "Can't find CMake. Either is not installed or not in the PATH. Aborting!"
    exit -1
fi

while getopts ":iaxbh" opt; do
  case ${opt} in
    i )
        export BUILD_IMAGE="1"
        ;;
    a )
        export BUILD_ARCH="arm64"
        ;;
    x )
        export BUILD_ARCH="x86_64"
        ;;
    b )
        export BUILD_DEBUG="1"
        ;;
    h ) echo "Usage: ./BuildMacOS.sh [-i]"
        echo "   -i: Generate DMG image (optional)"
        echo "   -a: Build for arm64 (Apple Silicon)"
        echo "   -x: Build for x86_64 (Intel)"
        echo "   -b: Build with debug symbols"
        exit 0
        ;;
  esac
done

echo "Build architecture: ${BUILD_ARCH}"

# mkdir build
if [ ! -d "build" ]
then
    mkdir build
fi

echo -n "[1/9] Updating submodules..."
{
    # update submodule profiles
    pushd resources/profiles
    git submodule update --init
    popd
} #> $ROOT/build/Build.log # Capture all command output
echo "done"

echo -n "[2/9] Changing date in version..."
{
    # change date in version
    sed "s/+UNKNOWN/_$(date '+%F')/" version.inc > version.date.inc
    mv version.date.inc version.inc
} #&> $ROOT/build/Build.log # Capture all command output
echo "done"

# mkdir in deps
if [ ! -d "deps/build" ]
then
    mkdir deps/build
fi

echo -n "[3/9] Configuring dependencies..."
{
    BUILD_ARGS=""
    if [[ -n "$BUILD_ARCH" ]]
    then
        BUILD_ARGS="${BUILD_ARGS}  -DCMAKE_OSX_ARCHITECTURES:STRING=${BUILD_ARCH}"
    fi
    if [[ -n "$BUILD_DEBUG" ]]
    then
        BUILD_ARGS="${BUILD_ARGS} -DCMAKE_BUILD_TYPE=Debug"
    fi
    # cmake deps
    echo "Cmake command: cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET=\"10.13\" ${BUILD_ARCH} "
    pushd deps/build
    cmake .. -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" $BUILD_ARGS 
    echo "ls deps/build:"
    ls -al
    echo "ls deps/build/dep_GLEW-prefix"
    ls -al dep_GLEW-prefix
} #&> $ROOT/build/Build.log # Capture all command output
echo "done"

echo -n "[4/9] Building dependencies..."
{
    # make deps
    make -j$NCORES
} #&> $ROOT/build/Build.log # Capture all command output
echo "done"

echo -n "[5/9] Renaming wxscintilla library..."
{
    # rename wxscintilla
    pushd destdir/usr/local/lib
    cp libwxscintilla-3.1.a libwx_osx_cocoau_scintilla-3.1.a
    echo "ls deps/build/destdir/usr/local/lib"
    ls -al
    popd
} #&> $ROOT/build/Build.log # Capture all command output
echo "done"

echo -n "[6/9] Cleaning dependencies..."
{
    # clean deps
    rm -rf dep_*
    popd
} #&> $ROOT/build/Build.log # Capture all command output
echo "done"

echo -n "[7/9] Configuring Slic3r..."
{
    BUILD_ARGS=""
    if [[ -n "$BUILD_ARCH" ]]
    then
        BUILD_ARGS="${BUILD_ARGS} -DCMAKE_OSX_ARCHITECTURES=${BUILD_ARCH}"
    fi
    if [[ -n "$BUILD_DEBUG" ]]
    then
        BUILD_ARGS="-DCMAKE_BUILD_TYPE=Debug ${BUILD_ARGS}"
    fi
    # cmake
    pushd build
    echo "Cmake command: cmake .. -DCMAKE_PREFIX_PATH=\"$PWD/../deps/build/destdir/usr/local\" -DCMAKE_OSX_DEPLOYMENT_TARGET=\"10.13\" -DSLIC3R_STATIC=1 ${BUILD_ARGS}"
    cmake .. -DCMAKE_PREFIX_PATH="$PWD/../deps/build/destdir/usr/local" -DCMAKE_OSX_DEPLOYMENT_TARGET="10.13" -DSLIC3R_STATIC=1 ${BUILD_ARGS}
} #&> $ROOT/build/Build.log # Capture all command output
echo "done"

echo -n "[8/9] Building Slic3r..."
{
    # make Slic3r
    make -j$NCORES Slic3r

    # make .mo
    make gettext_po_to_mo
} #&> $ROOT/build/Build.log # Capture all command output
echo "done"
echo "ls ROOT"
ls $ROOT
echo "ls ROOT/build"
ls $ROOT/build
echo "ls -al ROOT/build/src"
ls -al $ROOT/build/src
# Give proper permissions to script
chmod 755 $ROOT/build/src/BuildMacOSImage.sh

if [[ -n "$BUILD_IMAGE" ]]
then
	$ROOT/build/src/BuildMacOSImage.sh -i
else
	$ROOT/build/src/BuildMacOSImage.sh
fi
echo "ls -al ROOT/build"
ls -al $ROOT/build

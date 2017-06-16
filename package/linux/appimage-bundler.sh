#!/bin/bash

WD=$(dirname $0)
source $(dirname $0)/../common/util.sh
set_version
set_app_name
LOWERAPP=${appname,,}
wget -q https://github.com/probonopd/AppImages/raw/master/functions.sh -O ./functions.sh
. ./functions.sh

srcfolder="$WD/${appname}"
if [ ! -e $srcfolder ]; then
    echo "Run make_archive first."
    exit 1
fi

cd $srcfolder

# make shell exec and call it Slic3r

mkdir -p /usr/{lib,bin}
mv -R Slic3r local-lib var perl-local slic3r.pl /usr/bin
mv -R bin/* /usr/lib
rm -rf bin

get_apprun

# Copy desktop and icon file to application root dir for Apprun to pick them up.
sed -e "s|SLIC3R_VERSION|$SLIC3R_VERSION|" -e"s|APPLICATION_NAME|$appname|" ../slic3r.desktop.in > ../slic3r.desktop
cp ../slic3r.desktop $LOWERAPP.desktop 
cp ./var/Slic3r_192px_transparent.png ./slic3r.png

# archive directory has everything we need.
delete_blacklisted

get_desktopintegration $LOWERAPP

GLIBC_NEEDED=$(glibc_needed)
VERSION=git$GIT_REV-glibc$GLIBC_NEEDED

cd ..
mkdir -p out
generate_appimage

transfer ../out/*

#!/usr/bin/env bash

########################################################################
# Package the binaries built on Travis-CI as an AppImage
# By Joseph Lenox 2017
# For more information, see http://appimage.org/
# Assumes that the results from make_archive are present.
########################################################################

source $(dirname $0)/../common/util.sh

set_source_dir $2
set_version
get_commit
set_build_id
set_branch
set_app_name
set_pr_id

SCRIPTDIR=$(dirname $0)

WD=${PWD}/$(dirname $0)
srcfolder="$WD/${appname}"
export ARCH=$(arch)

APP=Slic3r
LOWERAPP=${APP,,}


mkdir -p $WD/${APP}.AppDir/usr/
wget -q https://github.com/probonopd/AppImages/raw/master/functions.sh -O ./functions.sh
. ./functions.sh

cd $WD/${APP}.AppDir

mkdir -p $WD/${APP}.AppDir/usr/bin
# Copy primary Slic3r script here and perl-local, as well as var
for i in {var,slic3r.pl,perl-local}; do
    cp -R $srcfolder/$i $WD/${APP}.AppDir/usr/bin/
done

mkdir -p ${WD}/${APP}.AppDir/usr/lib
# copy Slic3r local-lib here
for i in $(ls $srcfolder/bin); do  
    install -v $srcfolder/bin/$i ${WD}/${APP}.AppDir/usr/lib
done

# copy other libraries needed to /usr/lib because this is an AppImage build.
for i in $(cat $WD/libpaths.appimage.txt | grep -v "^#" | awk -F# '{print $1}'); do 
    install -v $i ${WD}/${APP}.AppDir/usr/lib
done

# Workaround to increase compatibility with older systems; see https://github.com/darealshinji/AppImageKit-checkrt for details
mkdir -p ${WD}/${APP}.AppDir/usr/optional/
wget -c https://github.com/darealshinji/AppImageKit-checkrt/releases/download/continuous/exec-x86_64.so -O ./Slic3r.AppDir/usr/optional/exec.so

mkdir -p ${WD}/${APP}.AppDir/usr/optional/libstdc++/
cp /usr/lib/x86_64-linux-gnu/libstdc++.so.6 ${WD}/${APP}.AppDir/usr/optional/libstdc++/
mkdir -p ${WD}/${APP}.AppDir/usr/optional/libgcc/
cp /lib/x86_64-linux-gnu/libgcc_s.so.1 ${WD}/${APP}.AppDir/usr/optional/libgcc/

mkdir -p ${WD}/${APP}.AppDir/usr/optional/swrast_dri/
cp /usr/lib/x86_64-linux-gnu/dri/swrast_dri.so	${WD}/${APP}.AppDir/usr/optional/swrast_dri/

cp -R $srcfolder/local-lib ${WD}/${APP}.AppDir/usr/lib/local-lib

cp ${WD}/appimage-apprun.sh ${WD}/${APP}.AppDir/AppRun

chmod +x AppRun

cp ${WD}/${APP}.AppDir/usr/bin/var/Slic3r_192px.png $WD/${APP}.AppDir/${APP}.png

cat > $WD/${APP}.AppDir/${APP}.desktop <<EOF
[Desktop Entry]
Type=Application
Name=$APP
Icon=$APP
Exec=AppRun
Categories=Graphics;
X-Slic3r-Version=$SLIC3R_VERSION
Comment=Prepare 3D Models for Printing
EOF

cd ..

wget "https://github.com/probonopd/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
chmod a+x appimagetool-x86_64.AppImage

./appimagetool-x86_64.AppImage Slic3r.AppDir

# If we're on a branch, add the branch name to the app name.
if [ "$current_branch" == "master" ]; then
    if [ ! -z ${PR_ID+x} ]; then
        outfile=Slic3r-${SLIC3R_BUILD_ID}-PR${PR_ID}-${1}.AppImage
    else
        outfile=Slic3r-${SLIC3R_BUILD_ID}-${1}.AppImage
    fi
else
    outfile=Slic3r-${SLIC3R_BUILD_ID}-${current_branch}-${1}.AppImage
fi
mv Slic3r-x86_64.AppImage ${WD}/../../${outfile}


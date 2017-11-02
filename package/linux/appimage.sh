#!/usr/bin/env bash

########################################################################
# Package the binaries built on Travis-CI as an AppImage
# By Joseph Lenox 2017
# For more information, see http://appimage.org/
# Assumes that the results from make_archive are present.
########################################################################

source $(dirname $0)/../common/util.sh

set_version
get_commit
set_build_id
set_branch
set_app_name
set_pr_id

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
cp -R $srcfolder/local-lib ${WD}/${APP}.AppDir/usr/lib/local-lib

cat > $WD/${APP}.AppDir/AppRun << 'EOF'
#!/usr/bin/env bash
# some magic to find out the real location of this script dealing with symlinks
DIR=`readlink "$0"` || DIR="$0";
DIR=`dirname "$DIR"`;
cd "$DIR"
DIR=`pwd`
cd - > /dev/null
# disable parameter expansion to forward all arguments unprocessed to the VM
set -f
# run the VM and pass along all arguments as is
LD_LIBRARY_PATH="$DIR/usr/lib" "${DIR}/usr/bin/perl-local" -I"${DIR}/usr/lib/local-lib/lib/perl5" "${DIR}/usr/bin/slic3r.pl" --gui "$@"
EOF

chmod +x AppRun

cp ${WD}/${APP}.AppDir/usr/bin/var/Slic3r_192px_transparent.png $WD/${APP}.AppDir/${APP}.png

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


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

srcfolder="$WD/${appname}"
export ARCH=$(arch)

APP=Slic3r
LOWERAPP=${APP,,}


mkdir -p $APP.AppDir/usr/
wget -q https://github.com/probonopd/AppImages/raw/master/functions.sh -O ./functions.sh
. ./functions.sh

cd $APP.AppDir

mkdir -p usr/bin
# Copy primary Slic3r script here and perl-local, as well as var
for i in {var,slic3r.pl,perl-local}; do
    cp -R $srcfolder/i $APP.AppDir/usr/bin/
done

mkdir -p usr/lib
# copy Slic3r local-lib here
for i in $(ls $srcfolder/bin); do  
    install -v $i $APP.AppDir/usr/lib
done
cp -R $srcfolder/local-lib $APP.AppDir/usr/lib/local-lib

cat > AppRun << 'EOF'
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
LD_LIBRARY_PATH="$DIR/usr/lib" "${DIR}/usr/bin/perl-local" -I"$DIR/usr/lib/local-lib/lib/perl5" "$DIR/usr/bin/slic3r.pl" "$@"
EOF

chmod +x AppRun

cp ../../../var/Slic3r_192px_transparent.png $APP.png

cat > $APP.desktop <<EOF
[Desktop Entry]
Type=Application
Name=$APP
Icon=$APP
Exec=AppRun
Categories=Graphics;
Version=$SLIC3R_VERSION
Comment=Prepare 3D Models for Printing
EOF

cd ..

wget "https://github.com/probonopd/AppImageKit/releases/download/continuous/appimagetool-x86_64.AppImage"
chmod a+x appimagetool-x86_64.AppImage

./appimagetool-x86_64.AppImage Slic3r.AppDir

#!/bin/bash
set -euo pipefail

perlbrew switch slic3r-perl
cpanm local::lib
eval $(perl -Mlocal::lib=$TRAVIS_BUILD_DIR/local-lib)
cd package/linux && make -f build_shell.mk && mv Slic3r* $TRAVIS_BUILD_DIR && cd $TRAVIS_BUILD_DIR

if [ -z ${WXDIR+x} ]; then
package/linux/make_archive.sh linux-x64
else
LD_LIBRARY_PATH=$WXDIR/lib package/linux/make_archive.sh linux-x64
fi

package/linux/appimage.sh x86_64

package/deploy/sftp.sh linux ~/slic3r-upload.rsa `pwd`/*.bz2 `pwd`/Slic3r*.AppImage
package/deploy/sftp-symlink.sh linux ~/slic3r-upload.rsa AppImage Slic3r*.AppImage
package/deploy/sftp-symlink.sh linux ~/slic3r-upload.rsa tar.bz2 *.bz2

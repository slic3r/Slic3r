#!/bin/bash
set -euo pipefail

# Not sure this is the correct deployment:
# cp slic3r ../
# LD_LIBRARY_PATH=$WXDIR/lib package/linux/make_archive.sh linux-x64
# package/linux/appimage.sh x86_64
# package/deploy/sftp.sh linux ~/slic3r-upload.rsa *.bz2 Slic3r*.AppImage
# package/deploy/sftp-symlink.sh linux ~/slic3r-upload.rsa AppImage Slic3r*.AppImage
# package/deploy/sftp-symlink.sh linux ~/slic3r-upload.rsa tar.bz2 *.bz2

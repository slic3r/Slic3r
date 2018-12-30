#!/bin/bash
set -euo pipefail

package/osx/make_dmg.sh
package/deploy/sftp.sh mac ~/slic3r-upload.rsa `pwd`/slic3r*.dmg
READLINK=greadlink package/deploy/sftp-symlink.sh mac ~/slic3r-upload.rsa dmg *.dmg

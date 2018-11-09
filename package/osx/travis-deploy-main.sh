#!/bin/bash
set -euo pipefail

package/osx/make_dmg.sh
package/deploy/sftp.sh mac ~/slic3r-upload.rsa *.bz2 slic3r*.dmg

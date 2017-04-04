#!/bin/bash
# Prerequisites
# Environment Variables:
#   UPLOAD_USER - user to upload to sftp server
# KEY is assumed to be path to a ssh key for UPLOAD_USER

DIR=$1
shift
KEY=$1
shift
FILES=$*

for i in $FILES; do 
     filepath=$(readlink -f "$i")
     echo "echo put $filepath | sftp -i$KEY \"${UPLOAD_USER}@dl.slic3r.org:$DIR/\""
     #echo put $filepath | sftp -i$KEY "${UPLOAD_USER}@dl.slic3r.org:$DIR/"
done

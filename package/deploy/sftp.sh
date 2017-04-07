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
source $(dirname $0)/../common/util.sh
set_pr_id
set_branch
if [ ! -z ${PR_ID+x} ] || [ $current_branch != "master" ]; then
    DIR=${DIR}/branches
fi

if [ -s $KEY ]; then
    for i in $FILES; do 
         filepath=$(readlink -f "$i")
         echo put $filepath | sftp -i$KEY "${UPLOAD_USER}@dl.slic3r.org:$DIR/"
    done
else
    echo "$KEY is not available, not deploying." 
fi

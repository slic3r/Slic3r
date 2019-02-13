#!/bin/bash
# Prerequisites
# Environment Variables:
#   UPLOAD_USER - user to upload to sftp server
# KEY is assumed to be path to a ssh key for UPLOAD_USER

DIR=$1
shift
KEY=$1
shift
EXT=$1
shift
FILES=$*

if [ -z ${READLINK+x} ]; then
    READLINK_BIN=readlink
else
    READLINK_BIN=$READLINK
fi

source $(dirname $0)/../common/util.sh
set_pr_id
set_branch
if [ ! -z ${PR_ID+x} ]; then
    exit 0
fi
if [ ! -z ${PR_ID+x} ] || [ $current_branch != "master" ]; then
    DIR=${DIR}/branches
fi

if [ -s $KEY ]; then
    for i in $FILES; do 
         filepath=$(${READLINK_BIN} -f "$i")
         filepath=$(basename $filepath)
         tmpfile=$(mktemp)

         echo "rm Slic3r-${current_branch}-latest.${EXT}" | sftp -i$KEY "${UPLOAD_USER}@dl.slic3r.org:$DIR/"
         echo "symlink $filepath Slic3r-${current_branch}-latest.${EXT} " > $tmpfile 
         sftp -b $tmpfile -i$KEY "${UPLOAD_USER}@dl.slic3r.org:$DIR/"
         result=$?
         if [ $? -eq 1 ]; then 
             echo "Error with SFTP symlink"
             exit $result; 
         fi
    done
else
    echo "$KEY is not available, not symlinking." 
fi
exit $result

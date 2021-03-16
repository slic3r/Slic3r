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
    if [ ! -z ${PR_ID+x} ] || [ $current_branch != "master" ]; then
        # clean up old copies of the same branch/PR
        if [ ! -z ${PR_ID+x} ]; then
            echo "rm *PR${PR_ID}*" | sftp -i$KEY "${UPLOAD_USER}@dl.slic3r.org:$DIR/"
        fi
        if [ $current_branch != "master" ]; then
            echo "rm *${current_branch}*" | sftp -i$KEY "${UPLOAD_USER}@dl.slic3r.org:$DIR/"
        fi
    fi
    for i in $FILES; do
        filepath=$i  # this is expected to be an absolute path
        tmpfile=$(mktemp)
        echo progress > $tmpfile
        echo put $filepath >> $tmpfile

        sftp -b $tmpfile -i$KEY "${UPLOAD_USER}@dl.slic3r.org:$DIR/"
        result=$?
        if [ $? -eq 1 ]; then
            echo "Error with SFTP"
            exit $result;
        fi
        rm $tmpfile
    done
else
    echo "$KEY is not available, not deploying."
fi
exit $result

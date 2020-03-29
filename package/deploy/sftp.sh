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
    DIR=${BASE_DIR}${DIR}/branches
fi

if [ -s $KEY ]; then
    if [ ! -z ${PR_ID+x} ] || [ $current_branch != "master" ]; then
        echo "AAA"
        # clean up old copies of the same branch/PR
        if [ ! -z ${PR_ID+x} ]; then
            echo "BBB"
            echo "rm *PR${PR_ID}*" | sftp -i$KEY -P 2222 "${UPLOAD_USER}@s1.mlab.cz:$DIR/"
        fi
        if [ $current_branch != "master" ]; then
            echo "CCC"
            echo "rm *${current_branch}*" | sftp -i$KEY -P 2222 "${UPLOAD_USER}@s1.mlab.cz:$DIR/"
        fi
    fi
    for i in $FILES; do
        echo "DDD"
        filepath=$i  # this is expected to be an absolute path
        tmpfile=$(mktemp)
        echo put $filepath > $tmpfile
	echo $tmpfile
        sftp -b $tmpfile -i$KEY -P 2222 "${UPLOAD_USER}@s1.mlab.cz:$DIR/"
        result=$?
        if [ $? -eq 1 ]; then
            echo "Error with SFTP"
            exit $result;
        fi
    done
else
    echo "$KEY is not available, not deploying."
fi
exit $result

#!/bin/bash
# Prerequistes
# Environment variables:
#   BINTRAY_API_KEY - Working API key
#   BINTRAY_API_USER - Bintray username.
#   SLIC3R_VERSION - Development version # for Slic3r


if [ $(git describe &>/dev/null) ]; then
    SLIC3R_BUILD_ID=$(git describe)
    TAGGED=true
else
    SLIC3R_BUILD_ID=${SLIC3R_VERSION}dev-$(git rev-parse --short head)
fi
current_branch=$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
if [ "$current_branch" == "" ]; then
    if [ "$BRANCH_NAME" == "" ]; then
        current_branch=$BRANCH_NAME
    elif [ "$APPVEYOR_REPO_BRANCH" == "" ]; then
        current_branch=$APPVEYOR_REPO_BRANCH
    else
        current_branch=unknown
fi
if [ "$current_branch" == "master" ] && [ "$APPVEYOR_PULL_REQUEST_NUMBER" == "" ]; then
    # If building master, goes in slic3r_dev or slic3r, depending on whether or not this is a tagged build
    if [ -z ${TAGGED+x} ]; then
        SLIC3R_PKG=slic3r_dev
    else
        SLIC3R_PKG=slic3r
    fi
    version=$SLIC3R_BUILD_ID
else
    # If building a branch, put the package somewhere else.
    SLIC3R_PKG=Slic3r_Branches
    version=$SLIC3R_BUILD_ID-$current_branch
fi

file=$1
echo "Deploying $file to $version on Bintray..."
API=${BINTRAY_API_KEY}
USER=${BINTRAY_API_USER}

curl -X POST -d "{ \"name\": \"$version\", \"released\": \"ISO8601 $(date +%Y-%m-%d'T'%H:%M:%S)\",  \"desc\": \"This version...\", \"github_release_notes_file\": \"RELEASE.txt\",  \"github_use_tag_release_notes\": true,  \"vcs_tag\": \"$version\"  }" -u${USER}:${API} https://api.bintray.com/content/lordofhyphens/Slic3r/${SLIC3R_PKG}/versions

curl -H "X-Bintray-Package: $SLIC3R_PKG" -H "X-Bintray-Version: $version" -H 'X-Bintray-Publish: 1' -H 'X-Bintray-Override: 1'  -T $file -u${USER}:${API} https://api.bintray.com/content/lordofhyphens/Slic3r/$(basename $1)
echo "Publishing $file..."
curl -X POST -u${USER}:${API} https://api.bintray.com/content/lordofhyphens/Slic3r/${SLIC3R_PKG}/$version/publish

curl -H 'Content-Type: application/json' -X PUT -d "{ \"list_in_downloads\":true }" -u${USER}:${API} https://api.bintray.com/file_metadata/lordofhyphens/Slic3r/$(basename $1)

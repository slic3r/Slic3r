#!/bin/bash
# Prerequistes
# Environment variables:
#   BINTRAY_API_KEY - Working API key
#   BINTRAY_API_USER - Bintray username.
# Run this from the repository root (required to get slic3r version)

SLIC3R_VERSION=$(grep "VERSION" xs/src/libslic3r/libslic3r.h | awk -F\" '{print $2}')
if [ $(git describe &>/dev/null) ]; then
    SLIC3R_BUILD_ID=$(git describe)
    TAGGED=true
else
    SLIC3R_BUILD_ID=${SLIC3R_VERSION}-$(git rev-parse --short HEAD)
fi
if [ -z ${GIT_BRANCH+x} ] && [ -z ${APPVEYOR_REPO_BRANCH+x} ]; then
    current_branch=$(git symbolic-ref HEAD | sed 's!refs\/heads\/!!')
else
    current_branch="unknown"
    if [ ! -z ${GIT_BRANCH+x} ]; then
        echo "Setting to GIT_BRANCH"
        current_branch=$(echo $GIT_BRANCH | cut -d / -f 2)
    fi
    if [ ! -z ${APPVEYOR_REPO_BRANCH+x} ]; then
        echo "Setting to APPVEYOR_REPO_BRANCH"
        current_branch=$APPVEYOR_REPO_BRANCH
    fi
fi

if [ -z ${current_branch+x} ]; then
    current_branch="unknown"
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
echo "Deploying $file to $version on Bintray repo $SLIC3R_PKG..."
API=${BINTRAY_API_KEY}
USER=${BINTRAY_API_USER}

echo "Creating version: $version"
curl -X POST -d "{ \"name\": \"$version\", \"released\": \"ISO8601 $(date +%Y-%m-%d'T'%H:%M:%S)\",  \"desc\": \"This version...\", \"github_release_notes_file\": \"RELEASE.txt\",  \"github_use_tag_release_notes\": true,  \"vcs_tag\": \"$version\"  }" -u${USER}:${API} https://api.bintray.com/content/lordofhyphens/Slic3r/${SLIC3R_PKG}/versions

echo "Publishing ${file} to ${version}..."
curl -H "X-Bintray-Package: $SLIC3R_PKG" -H "X-Bintray-Version: $version" -H 'X-Bintray-Publish: 1' -H 'X-Bintray-Override: 1'  -T $file -u${USER}:${API} https://api.bintray.com/content/lordofhyphens/Slic3r/$(basename $1)
#curl -X POST -u${USER}:${API} https://api.bintray.com/content/lordofhyphens/Slic3r/${SLIC3R_PKG}/$version/publish
# Wait 5s for the server to catch up
sleep 5 
curl -H 'Content-Type: application/json' -X PUT -d "{ \"list_in_downloads\":true }" -u${USER}:${API} https://api.bintray.com/file_metadata/lordofhyphens/Slic3r/$(basename $1)

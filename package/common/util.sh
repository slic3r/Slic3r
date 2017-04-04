#!/bin/bash

# must be run from the root
function set_version ()
{
SLIC3R_VERSION=$(grep "VERSION" xs/src/libslic3r/libslic3r.h | awk -F\" '{print $2}')
}
# Cache the SHA1 for this build commit.
function get_commit () {
    if [ ! -z ${TRAVIS_COMMIT+x} ]; then
        # Travis sets the sha1 in TRAVIS_COMMIT
        COMMIT_SHA1=$(git rev-parse --short $TRAVIS_COMMIT)
    else
        # should be able to get it properly
        COMMIT_SHA1=$(git rev-parse --short HEAD)
    fi
}
function set_build_id ()
{
echo "Setting SLIC3R_BUILD_ID"
if [ $(git describe &>/dev/null) ]; then
    SLIC3R_BUILD_ID=$(git describe)
    TAGGED=true
else
    SLIC3R_BUILD_ID=${SLIC3R_VERSION}-${COMMIT_SHA1}
fi

}

function set_branch () 
{
    echo "Setting current_branch"
    if [ -z ${TRAVIS_BRANCH} ] && [ -z ${GIT_BRANCH+x} ] && [ -z ${APPVEYOR_REPO_BRANCH+x} ]; then
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
        if [ ! -z ${TRAVIS_BRANCH} ]; then
            if [ "${TRAVIS_BRANCH}" != "false" ]; then
                echo "Setting to TRAVIS_BRANCH"
                current_branch=$TRAVIS_BRANCH
            fi
        fi
    fi

    if [ -z ${current_branch+x} ]; then
        current_branch="unknown"
    fi
}

function set_app_name ()
{
    set_branch 
    if [ "$current_branch" == "master" ]; then
        appname=Slic3r
    else
        appname=Slic3r-${current_branch}
    fi
}


function set_pr_id ()
{
    echo "Setting PR_ID if available."
    if [ ! -z ${GITHUB_PR_NUMBER+x} ]; then
        PR_ID=$GITHUB_PR_NUMBER
    fi
    if [ ! -z ${APPVEYOR_PULL_REQUEST_NUMBER+x} ]; then
        PR_ID=$APPVEYOR_PULL_REQUEST_NUMBER
    fi
    if [ ! -z ${TRAVIS_PULL_REQUEST_BRANCH+x} ] && [ "${TRAVIS_PULL_REQUEST}" != "false" ] ; then
        PR_ID=$TRAVIS_PULL_REQUEST
    fi
    if [ ! -z ${PR_ID+x} ]; then
        echo "Setting PR_ID to $PR_ID."
    else 
        echo "PR_ID remains unset."
    fi
}

function install_par ()
{
    cpanm -q PAR::Packer
    cpanm -q Wx::Perl::Packager
}

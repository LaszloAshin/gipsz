#!/bin/sh
VERSION=`git describe`
echo "${VERSION-v0.0.0}+build${TRAVIS_BUILD_NUMBER-0}"

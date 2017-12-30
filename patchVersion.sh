#!/bin/sh

# script to patch version number into source. Called by gitpkgtool during
# build, is passed to arguments, $majorVersion and $minorVersion.

MAJORVERSION=$1
MINORVERSION=$2

echo " - Patching Makefile with MAJORVERSION=${MAJORVERSION} MINORVERSION=${MINORVERSION}"

sed -i -e "s/^MAJORVER=.*/MAJORVER=${MAJORVERSION}/" Makefile
sed -i -e "s/^MINORVER=.*/MINORVER=${MINORVERSION}/" Makefile


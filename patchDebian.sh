#!/bin/sh

# script to patch Debian folder. Called by gitpkgtool during
# build, is passed to arguments, $project $version $sover.

PROJECT=$1
VERSION=$2
SOVER=$3

echo " - Patching Debian with PROJECT=${PROJECT} VERSION=${VERSION} SOVER=${SOVER}"

sed -i -e "s/\${project}/${PROJECT}/" debian/control
sed -i -e "s/\${sover}/${SOVER}/" debian/control
sed -i -e "s/\${version}/${VERSION}/" debian/control

ln -sf libxplane-udp-client.install debian/libxplane-udp-client${SOVER}.install
ln -sf libxplane-udp-client-dev.install debian/libxplane-udp-client${SOVER}-dev.install


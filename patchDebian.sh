#!/bin/sh

# script to patch Debian folder. Called by gitpkgtool during
# build, it is passed the arguments: $project $majorVersion $minorVersion

PROJECT=$1
MAJORVERSION=$2
MINORVERSION=$3

echo " - Patching Debian with PROJECT=${PROJECT} MAJORVERISON=${MAJORVERSION} MINORVERSION=${MINORVERSION}"

PACKAGE=`echo ${PROJECT} | tr A-Z a-z`
LIBNAME=`echo ${PROJECT} | sed -e 's/-//g'`
ARCH=`arch | sed -e 's/x86_64/amd64/' -e 's/armv[67]l/armhf/'`

echo " - PACKAGE ${PACKAGE} LIBNAME ${LIBNAME}"

sed -i -e "s/\${PACKAGE}/${PACKAGE}/" debian/control
sed -i -e "s/\${MAJORVERSION}/${MAJORVERSION}/" debian/control

# The .install need to be symlinked as the packagename has $MAJORVERSION
# suffixed, so this is what dpkg-deb looks for.

ln -sf ${PACKAGE}.install debian/${PACKAGE}${MAJORVERSION}.install
ln -sf ${PACKAGE}-dev.install debian/${PACKAGE}${MAJORVERSION}-dev.install
ln -sf ${PACKAGE}-bin.install debian/${PACKAGE}${MAJORVERSION}-bin.install


# substitute variables into the contents of the .install files
for dotinstall in debian/control debian/*install ; do
	sed -i 	-e "s/\${PACKAGE}/${PACKAGE}/" \
					-e "s/\${LIBNAME}/${LIBNAME}/" \
					-e "s/\${MAJORVERSION}/${MAJORVERSION}/" \
					-e "s/\${MINORVERSION}/${MINORVERSION}/" \
					-e "s/\${ARCH}/${ARCH}/" $dotinstall
done


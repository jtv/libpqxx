#!/bin/sh -
#===============================================================================
#
#          FILE: repack.sh
# 
#   DESCRIPTION: Repackage upstream source to exclude non-distributable files
#       should be called as "repack.sh --upstream-source <ver> <downloaded file>
#       (for example, via uscan), have a look into d/rules
#
#===============================================================================

set -e                                      # Break and exit script on error
set -o nounset                              # Treat unset variables as an error

VER=$2

ORIG_TARBALL="libpqxx-${VER}.tar.gz"
DFSG_TARBALL="libpqxx_${VER}+dfsg1.orig.tar.xz"

## Untar tarball
tar zxf ../$ORIG_TARBALL -C ../

## Remove sourceless files
rm -r ../libpqxx-${VER}/doc/html/Reference/jquery.js

## Repack
mv ../libpqxx-${VER} ../libpqxx-${VER}+dfsg1.orig
tar Jcf ../$DFSG_TARBALL ../libpqxx-${VER}+dfsg1.orig > /dev/null 2>&1

## Clean up
rm -r ../libpqxx-${VER}.tar.gz ../libpqxx-${VER}+dfsg1.orig

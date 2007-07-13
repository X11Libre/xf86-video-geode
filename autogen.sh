#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

rm -rf config.guess config.sub ltmain.sh

autoreconf -v --install || exit 1
cd $ORIGDIR || exit $?

$srcdir/missing --run git-log --stat | fmt --split-only 1> ChangeLog 2>/dev/null

$srcdir/configure --enable-maintainer-mode "$@"

make maintainer-clean

#EOF

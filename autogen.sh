#!/bin/sh

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

ORIGDIR=`pwd`
cd $srcdir

rm -rf config.guess config.sub ltmain.sh

autoreconf -v --install || exit 1
cd $ORIGDIR || exit $?

if [ -d .git ]
then
	$srcdir/missing --run git-log --stat | fmt --split-only 1> ChangeLog
fi

$srcdir/configure --enable-maintainer-mode "$@"

make maintainer-clean

#EOF

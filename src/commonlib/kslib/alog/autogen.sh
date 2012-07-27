#!/bin/sh

autoheader
libtoolize --force

rm -f config.cache
rm -f config.log

aclocal
autoconf
automake --add-missing --force


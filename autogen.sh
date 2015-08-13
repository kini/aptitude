#!/bin/sh

touch po/POTFILES.in &&
aclocal -I m4 &&
autoheader &&
automake --add-missing --copy -Wno-portability &&
aclocal -I m4 &&
autoconf &&
autoheader

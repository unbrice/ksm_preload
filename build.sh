#!/bin/bash
set -x
#set -e

libtoolize --force
aclocal
autoheader
automake --force-missing --add-missing
autoconf
./configure
make
sudo make install

#!/bin/sh

svn export --force http://svn.simba.taobao.com/svn/kingso/project/commdef/trunk/include/commdef include

./autogen.sh
./configure --with-mxml=/home/yewang/install_commonlib/  --with-commdef=/home/yewang/install_commonlib/ --with-alog=/home/yewang/install_commonlib/ --with-util=/home/yewang/install_commonlib/ CFLAGS=" -g -Wall -DDEBUG" CXXFLAGS=" -g -Wall -DDEBUG"


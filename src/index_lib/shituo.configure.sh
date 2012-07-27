#!/bin/sh
./autogen.sh
./configure --prefix=/home/shituo.lyc/index_lib --with-protobuf=/home/shituo.lyc/install/install/protobuf/ --with-pool=/home/shituo.lyc/install/install/pool/ --with-alog=/home/shituo.lyc/install/install/alog/ --with-mxml=/home/shituo.lyc/install/install/mxml/ --with-commdef=/home/shituo.lyc/install/install/commdef/ --with-util=/home/shituo.lyc/install/install/util/ CFLAGS=-g CXXFLAGS=-g

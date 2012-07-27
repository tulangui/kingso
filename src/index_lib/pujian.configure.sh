#!/bin/sh
./autogen.sh
./configure --with-protobuf=/home/xiaojie.lgx/install/install/protobuf --with-pb=/home/xiaojie.lgx/install/install/protobuf --with-kslib=/home/xiaojie.lgx/install/install/kslib --with-commdef=/home/xiaojie.lgx/install/install/commdef --with-framework=/home/xiaojie.lgx/install/install/framework --prefix=/home/xiaojie.lgx/install/install/index_lib CXXFLAGS="-g" CFLAGS="-g"
make clean
make -j8


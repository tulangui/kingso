#!/bin/sh
rm -rf ../rundata
mkdir -p ../rundata/index
mkdir -p ../rundata/profile
mkdir -p ../rundata/detail
mkdir -p ../rundata/tmp
../bin/index_builder -c ../etc/build_conf.xml -t 1 -i ./data -o ../rundata -T ../rundata/tmp
gunzip ../rundata/detail/detail.idx.gz && gunzip ../rundata/detail/detail.dat.000.gz && mv ../rundata/detail/detail.dat.000 ../rundata/detail/detail.dat

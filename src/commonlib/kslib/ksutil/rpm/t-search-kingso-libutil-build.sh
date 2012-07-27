#!/bin/bash
##for check
#export LD_LIBRARY_PATH=/home/admin/oracle/lib/
cd $1/rpm
/usr/local/bin/rpm_create -p /home/admin/release -v $3 -r $4 $2.spec -k

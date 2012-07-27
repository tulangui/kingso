#!/bin/bash

if [ $# == 0 ] || [ $1 != "--force" ]; then
    while [ 1 ]
    do
        read -p "Are you sure to continue?(Y/N): " choice
        if [ $choice == 'Y' ] || [ $choice == 'y' ]; then
		break;
	fi

	if [ $choice == 'N' ] || [ $choice == 'n' ]; then
		exit 255;
	fi
    done    
fi

SHR_KEYS=`ipcs -m | awk 'BEGIN{flag=0;} flag==1&&NF>1{print $1} $1=="key"{flag=1;}'`
for key in $SHR_KEYS
do
	echo "release share memory of key [$key]"
	ipcrm -M $key
done

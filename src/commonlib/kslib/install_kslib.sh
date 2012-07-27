#!/bin/bash


usage(){
    echo "####################################################"
    echo "# Compiler and install kslib module                #"          
    echo "# Usage: ./install_kslib.sh -p installpath         #"
    echo "####################################################"
    exit 1
}


if [ $# -eq 0 ]; then
    usage
fi

while getopts p: OPTION
do
    case $OPTION in
        p)prefix=$OPTARG
        ;;
        \?)usage
        ;;
    esac
done


export prefix

make
make install


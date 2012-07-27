#!/bin/bash

usage()
{
    echo -e "usage:\n\t$1 TOPN"
}

# 配置文件名定义
CONF_FILE="TopQuery.conf"
source $CONF_FILE

# 1.配置项检查
if [ ! -e $CONF_FILE ] ;then
    echo "ERROR: can not find $CONF_FILE file in current work path!"
    exit 1
fi

if [ ! -e $EXEC_FILE ] ;then
    echo "ERROR: can not find $EXEC_FILE file in current work path!"
    exit 1
fi

if [ ! -x $EXEC_FILE ] ;then
    echo "ERROR: $EXEC_FILE must be executed!"
    exit 1
fi

# 2.检查参数数量
if [ $# -ne 1 ]; then
    usage $0
    exit 1;
fi

TOPN=$1
if [ $TOPN -lt 0 ] || [ $TOPN -gt 100000000 ]; then
   usage $0
   exit 1;
fi

# 3.输出目录预处理
rm -rf $OUTPUT_FILE
mkdir -p $OUTPUT_DIR
mkdir -p sortTmp

# 4.处理流程
zcat $SEARCHER_LOG | awk '{print $6" "$8}' | ./$EXEC_FILE | sort -nr -k 3 -T sortTmp | uniq -f 2 -c | sort -nr -k 1 -T sortTmp | head -$TOPN | awk '{print $3}' > $OUTPUT_FILE
#zcat $SEARCHER_LOG | ./$EXEC_FILE | sort -nr -k 3 -T sortTmp | uniq -f 2 -c | sort -nr -k 1 -T sortTmp | head -$TOPN  > $OUTPUT_FILE

sed -i 's/[&=]!et=[^&]*//g' $OUTPUT_FILE
sed -i 's/#other&/#other=/g' $OUTPUT_FILE
sed -i 's/src=[^&]*/src=warnup/g' $OUTPUT_FILE
sed -i 's/mcn=[^&]*/mcn=warnup/g' $OUTPUT_FILE

rm -rf sortTmp

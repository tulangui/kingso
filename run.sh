#!/bin/sh
source install_config.sh
if [ $? != 0 ]; then
    echo "Can not source scm_config.sh";
    exit 255;
fi

#get options value
args=`getopt -o p:h -l prefix:,help -- $*`
eval set -- $args
for i;do
    case $i in
        -p|--prefix)prefix=$2;shift 2;;
        -h|--help)echo "Usage: ……";exit;;
        --)shift;;
    esac
done

if [ -n "$prefix" ] ; then
    RELEASE_PATH=$prefix
fi



#cleanup build directory
echo Cleaning up build directory....
rm -rf $DST_PATH

mkdir -p $CURRENT_VERSION_PATH 
mkdir -p $SRC_PATH
mkdir -p $DST_PATH
mkdir -p $DST_PATH/logs
#mkdir -p $DST_PATH/plugin/


mkdir -p $LOG_PATH

for ((i=0;i<MODULE_COUNT;i=i+1)) {
    module_name=${MODULE_LIST[$((i*2))]};
    eval "module_path=\$${MODULE_LIST[$((i*2+1))]}"
    TAG_TABLE=$TAG_TABLE$tmp
}

function make_module(){
module_name=$1
module_path=$2
my_dir=`pwd`
cd $SRC_PATH/$module_path

echo "call autogen.sh $module_name ..."
echo "call autogen.sh $module_name ..." >>$LOG_FILE
#检查是否有autogen.sh
if test -f autogen.sh ; then
    #替换版本号
    if test -f configure.ac; then
        cat configure.ac | sed "s/{KINGSO_VERSION}/$KINGSO_CURRENT_VERSION/g" > /tmp/ks_ac_tmp_$USER.txt
        cp /tmp/ks_ac_tmp_$USER.txt ./configure.ac
    fi

    sh autogen.sh 1>>$LOG_FILE 2>>$ERROR_LOG_FILE
    if [ $? -ne 0 ]; then 
        echo "Call autogen.sh failed. MODULE:$module_name"
        echo "Call autogen.sh failed. MODULE:$module_name" >>$ERROR_LOG_FILE
        cd $my_dir
        return 255;
    fi
fi
echo "Configuring $module_name ..."
echo "Configuring $module_name ..." >>$LOG_FILE
sh configure CFLAGS="-g -O2 -fPIC" CXXFLAGS="-g -O2 -fPIC" $WITHS --prefix=$DST_PATH/ 1>>$LOG_FILE 2>>$ERROR_LOG_FILE
if [ $? -ne 0 ]; then 
    echo "Call configure failed. MODULE:$module_name"
    echo "Call configure failed. MODULE:$module_name" >>$ERROR_LOG_FILE
    cd $my_dir
    return 255;
fi
echo "make $module_name ..."
echo "make $module_name ..." >>$LOG_FILE
make -j8 1>>$LOG_FILE 2>>$ERROR_LOG_FILE 1>>$LOG_FILE 2>>$ERROR_LOG_FILE
if [ $? -ne 0 ]; then 
    echo "make failed. MODULE:$module_name"
    echo "make failed. MODULE:$module_name" >>$ERROR_LOG_FILE
    cd $my_dir
    return 255;
fi
echo "Install $module_name ..."
echo "Install $module_name ..." >>$LOG_FILE
make install 1>>$LOG_FILE 2>>$ERROR_LOG_FILE 1>>$LOG_FILE 2>>$ERROR_LOG_FILE
if [ $? -ne 0 ]; then 
    echo "make install failed. MODULE:$module_name"
    echo "make install failed. MODULE:$module_name" >>$ERROR_LOG_FILE
    cd $my_dir
    return 255;
fi
cd $my_dir
echo "$module_name installed."
echo "$module_name installed." >>$LOG_FILE
return 0;
}

echo "Building scws-1.2.0......"
echo "Building scws-1.2.0......" >>$LOG_FILE
prefix=$DST_PATH && cd $SRC_PATH/commonlib/scws-1.2.0 && ./configure --prefix=$prefix --with-pic 1>>$LOG_FILE 2>>$ERROR_LOG_FILE && make install 1>>$LOG_FILE 2>>$ERROR_LOG_FILE
cd -
if [ $? -ne 0 ]; then
    echo "Install scws-1.2.0 failed!"
    echo "Install scws-1.2.0 failed!" >>$ERROR_LOG_FILE
    exit 255;
fi


echo "Building protobuf......"
echo "Building protobuf......" >>$LOG_FILE
prefix=$DST_PATH && cd $SRC_PATH/commonlib/protobuf && ./configure --prefix=$prefix --with-pic 1>>$LOG_FILE 2>>$ERROR_LOG_FILE && make install 1>>$LOG_FILE 2>>$ERROR_LOG_FILE 
cd -
if [ $? -ne 0 ]; then
    echo "Install protobuf failed!"
    echo "Install protobuf failed!" >>$ERROR_LOG_FILE
    exit 255;
fi

echo "Building kslib......"
echo "Building kslib..." >>$LOG_FILE
my_dir=`pwd`
cd $SRC_PATH/commonlib/kslib
#prefix=$DST_PATH/kslib make -C $SRC_PATH/kslib install -j8
sh install_kslib.sh -p $DST_PATH 1>>$LOG_FILE 2>>$ERROR_LOG_FILE
if [ $? -ne 0 ]; then
    echo "Install kslib failed!";
    cd $my_dir;
    exit 255;
fi
cd $my_dir

echo Building modules......
for ((i=0;i<MODULE_COUNT;i=i+1)) {
    echo "$((i+1))/$MODULE_COUNT" 
    module_name=${MODULE_LIST[$((i*2))]};
    module_path=${MODULE_LIST[$((i*2+1))]};
    #eval "module_path=\$${MODULE_LIST[$((i*2+1))]}"
    make_module $module_name $module_path
    if [ $? -ne 0 ]; then 
        echo "make_module $module_name $module_path failed!!!"
        echo "make_module $module_name $module_path failed!!!" >>$ERROR_LOG_FILE
        exit 255;
    fi
}

rm -rf $DST_PATH/etc/*
cp -r $SRC_PATH/../conf/* $DST_PATH/etc/.
cp -r $SRC_PATH/../example $DST_PATH/example
rm -rf $RELEASE_PATH
mkdir -p $RELEASE_PATH
mkdir -p $RELEASE_PATH/scripts/
mkdir -p $RELEASE_PATH/detail_plugin
mkdir -p $RELEASE_PATH/logs/
mkdir -p $RELEASE_PATH/www/
mkdir -p $RELEASE_PATH/update_rundata/
cp -r $DST_PATH/bin $RELEASE_PATH/bin
cp -r $DST_PATH/lib $RELEASE_PATH/lib
#确保其他文件拷贝
cp -r $SRC_PATH/../conf $RELEASE_PATH/etc
cp -r $SRC_PATH/../example $RELEASE_PATH/example

CUR_PATH=`pwd`
cd $RELEASE_PATH/etc
#sed -i s@/whereami/@$RELEASE_PATH/@ *

cd $CUR_PATH
CUR_PATH=`pwd`

cp $DST_PATH/detail_plugin/* $RELEASE_PATH/detail_plugin/ 
cd $RELEASE_PATH 
rm lib/*.a lib/*.la -f
rm lib/libssh* -f
chmod +x bin/*
tar zcvf $BUILD_PATH/kingso-$KINGSO_CURRENT_VERSION.tar.gz bin lib detail_plugin logs www update_rundata 
#tar zcvf $BUILD_PATH/kingso-conf-$KINGSO_CURRENT_VERSION.tar.gz etc

cd $DST_PATH
rm lib/*.a lib/*.la -f
rm lib/libssh* -f
chmod +x bin/*

cd $CUR_PATH

echo "Build succeed."
echo "Build succeed." 1>>$LOG_FILE 2>>$ERROR_LOG_FILE

exit 0



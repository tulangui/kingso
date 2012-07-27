BUILD_PATH=`pwd`/build/
RELEASE_PATH=$BUILD_PATH/release/kingso
TEST_DATA_PATH=/home/$USER/orig_xml/
LAST_VERSION_PATH=$BUILD_PATH/last_version/
CURRENT_VERSION_PATH=$BUILD_PATH/current_version/
ETC_PATTERN_FILE=pattern.txt
SRC_PATH=`pwd`/src
DST_PATH=$BUILD_PATH/dst
LOG_PATH=$BUILD_PATH/log/
ETC_PATH=$DST_PATH/etc/
BIN_PATH=$DST_PATH/bin/
RUN_DATA_PATH=$DST_PATH/rundata/
ETC_TMP_PATH=/tmp/kingso_etc_$USER
KINGSO_CURRENT_VERSION=0.0.1
QUERY_TEST_OUTPUT=test_output/test_output_$KINGSO_CURRENT_VERSION.txt

TIME_STRING=`date +%Y%m%d%H%M%S`
LOG_FILE=$LOG_PATH/build_log.$TIME_STRING.txt
ERROR_LOG_FILE=$LOG_PATH/build_log.$TIME_STRING.error.txt
BUILD_STATUS=""
WITHS="--with-index_lib=$DST_PATH --with-queryparser=$DST_PATH --with-search=$DST_PATH --with-statistic=$DST_PATH --with-sort=$DST_PATH --with-analysis=$DST_PATH --with-update=$DST_PATH --with-libdetail=$DST_PATH --with-commdef=$DST_PATH --with-libbuild=$DST_PATH --with-kslib=$DST_PATH --with-clustermap=$DST_PATH --with-framework=$DST_PATH --with-aliws=$ALIWS_PATH --with-libssh2=$DST_PATH  --with-protobuf=$DST_PATH"


#COMMONLIB_LIST=( \

MODULE_LIST=( \
             "libssh2" \
             "commonlib/libssh2" \
             "clustermap" \
             "commonlib/clustermap" \
             "commdef" \
             "commdef" \
             "framework" \
             "commonlib/framework" \
             "index_lib" \
             "index_lib" \
             "analysis" \
             "analysis" \
             "queryparser" \
             "queryparser" \
             "statistic" \
             "statistic" \
             "sort" \
             "sort" \
             "search" \
             "search" \
             "libdetail" \
             "libdetail" \
             "libbuild" \
             "libbuild" \
             "update" \
             "update" \
             "searcher" \
             "searcher" \
             "detail" \
             "detail_server" \
             "index_builder" \
             "index_builder" \
             "merger" \
             "merger"
             )
tmp=${#MODULE_LIST[*]}
MODULE_COUNT=$((tmp/2))

TEST_PATTERN_FILE=/tmp/kingso_pattern_$USER.txt

export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:$DST_PATH/lib:/usr/local/lib

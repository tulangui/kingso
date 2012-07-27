/*********************************************************
 *  Created on: 2011-5-27
 *      Author: yewang@taobao.com
 *
 *        Desc: 在指定的链条中 查找特定的nId是否存在
 *
 *********************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <algorithm>
#include <functional>

#include "index_lib/IndexLib.h"
#include "index_lib/IndexReader.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/ProfileManager.h"


using namespace std;
using namespace index_lib;


typedef struct
{
    char    cfgFile[ PATH_MAX ];                 // 索引的配置文件
    char    idxField[ PATH_MAX ];                // 倒排字段
    char    term[ PATH_MAX ];                    // token
    char    psField[ PATH_MAX ];                 // 排序字段
    char    sortType[ PATH_MAX ];                // 0:递增排序   从小到大  ps; 1：递减排序   从大到小   _ps
    char    nidList[ 4096 ];                     // nid列表 空格分隔
} CMD_PARAM;



/** ================================================================= */


/**
 * 使用说明
 *
 * @param name  程序名
 */
void usage(const char * name)
{
    fprintf(stderr, "\nUsage: %s \n", name);
    fprintf(stderr, "   -c \"cfgFile\"     : 索引配置文件(xml)  必填 \n");
    fprintf(stderr, "   -i \"idxField\"    : 倒排字段名称                       必填 \n");
    fprintf(stderr, "   -t \"term\"        : token         必填 \n");
    fprintf(stderr, "   -p \"psField\"     : 排序字段名称           \n");
    fprintf(stderr, "   -s \"sortType\"    : 0:从小到大  ps; 1：从大到小 _ps\n");
    fprintf(stderr, "   -n \"nidList\"     : nid列表 空格分隔           必填  \n");
    fprintf(stderr, "   -h help \n");
    fprintf(stderr, "备注: 如果要查截断索引, 必须指定 排序字段 和 排序方式 \n");

    exit(EXIT_FAILURE);
}



/**
 * 参数解析  与 参数检查
 *
 * @param argc        程序参数个数
 * @param argv        程序参数指针数组
 * @param cmdp        参数结构体指针
 */
void parse_args(int argc, char ** argv, CMD_PARAM * cmdp)
{
    memset( cmdp, 0, sizeof(CMD_PARAM) );

    int i = 0;
    while ( (i = getopt(argc, argv, "c:i:t:p:s:n:h")) != EOF )
    {
        switch ( i )
        {
            case 'c':
                strncpy( cmdp->cfgFile,  optarg, sizeof(cmdp->cfgFile) - 1 );
                break;
            case 'i':
                strncpy( cmdp->idxField, optarg, sizeof(cmdp->idxField) - 1 );
                break;
            case 't':
                strncpy( cmdp->term,     optarg, sizeof(cmdp->term) - 1 );
                break;
            case 'p':
                strncpy( cmdp->psField,  optarg, sizeof(cmdp->psField) - 1 );
                break;
            case 's':
                strncpy( cmdp->sortType, optarg, sizeof(cmdp->sortType) - 1 );
                break;
            case 'n':
                strncpy( cmdp->nidList,  optarg, sizeof(cmdp->nidList) - 1 );
                break;
            case 'h':
                usage( argv[0] );
                break;
            default:
                break;
        }
    }

    /* 校验参数 */
    if (( cmdp->cfgFile[0] == '\0' ) || ( cmdp->idxField[0] == '\0' ))
        usage( argv[0] );

    if (( cmdp->term[0] == '\0' ) || ( cmdp->nidList[0] == '\0' ))
        usage( argv[0] );

    if (( cmdp->psField[0] != '\0' ) && ( cmdp->sortType[0] == '\0' ))
        usage( argv[0] );
}



/**
 * 用特定的分隔符 将字符串分开 （源字符串会被修改掉）
 *
 * @param begin               字符串的起始位置
 * @param end                 字符串的结束位置
 * @param delim               分隔符
 * @param result_arr          返回的指针数组， 切除的指针都放在这里
 * @param result_arr_len      传入的数组的长度
 *
 * @return  切分出的子串个数
 */
int ac_str_splict(char * begin,
                  char * end,
                  char   delim,
                  char * result_arr[],
                  int    result_arr_len)
{
    int  cnt = 0;

    /* 先解析出字段级别  */
    while ( '\0' != *begin )
    {
        result_arr[cnt] = begin;
        ++cnt;

        while ( ('\0' != *begin) && (delim != *begin) )
        {
            ++begin;                   /* 循环一直到  delim 或者  \0 结束 */
        }

        *begin = '\0';                 /* 找到了分隔符 截断 */

        ++begin;

        if ( (begin >= end) || (cnt >= result_arr_len) )
        {
            result_arr[cnt] = begin;
            break;
        }
    }

    return cnt;
}




/**
 * 加载index索引
 *
 * @param config
 *
 * @return
 */
int init_index_lib( const char * config )
{
    mxml_node_t * pXMLTree  = NULL;
    FILE        * fp        = fopen( config, "r" );

    if ( fp == NULL )
    {
        fprintf(stderr, "open %s failed\n", config);
        return -1;
    }

    pXMLTree = mxmlLoadFile( NULL, fp, MXML_NO_CALLBACK );
    fclose(fp);

    if ( NULL == pXMLTree )
    {
        fprintf(stderr, "file %s not xml\n", config);
        return -1;
    }

    if ( 0 != index_lib::init( pXMLTree ) )
    {
        fprintf(stderr, "init index_lib failed\n");
        return -1;
    }

    mxmlDelete( pXMLTree );
    return 0;
}



IndexTerm * findTerm(CMD_PARAM &cmdp, MemPool &pool)
{
    IndexReader         * reader  = IndexReader::getInstance();
    DocIdManager        * docMgr  = DocIdManager::getInstance();
    ProfileManager      * pflMgr  = ProfileManager::getInstance();
    ProfileDocAccessor  * pflAcr  = pflMgr->getDocAccessor();

    if (false == reader->hasField( cmdp.idxField ))   // 检查倒排字段是否存在
    {
        fprintf(stderr, "can't find index field:%s\n", cmdp.idxField);
        return NULL;
    }

    IndexTerm * indexTerm = NULL;

    if ( '\0' == cmdp.psField[ 0 ] )                  // 没有排序字段
    {
        indexTerm = reader->getTerm( &pool, cmdp.idxField, cmdp.term);

        if ( NULL == indexTerm )
        {
            fprintf(stderr, "can't find the Term:%s\n", cmdp.term);
            return NULL;
        }
    }
    else
    {                                                 // 检查是否有高频截断索引
        // 检查排序字段是否存在
        if ( NULL == pflAcr->getProfileField( cmdp.psField ) )
        {
            fprintf(stderr, "can't find profile field:%s\n", cmdp.psField);
            return NULL;
        }

        if ( 0 == strcmp( cmdp.sortType, "0" ) )
        {
            indexTerm = reader->getTerm( &pool, cmdp.idxField, cmdp.term, cmdp.psField, 0);
        }
        if ( 0 == strcmp( cmdp.sortType, "1" ) )
        {
            indexTerm = reader->getTerm( &pool, cmdp.idxField, cmdp.term, cmdp.psField, 1);
        }

        if ( NULL == indexTerm )
        {
            fprintf(stderr, "can't find the highFreq Term:%s\n", cmdp.term);
            return NULL;
        }
    }

    return indexTerm;
}



/** 主程序入口  */
int main( int argc, char * argv[] )
{
    CMD_PARAM   cmdp;
    int         stime = time(NULL);

    parse_args(argc, argv, &cmdp);                    // 分析参数

    if ( -1 == init_index_lib( cmdp.cfgFile ) )       // 初始化索引
        return EXIT_FAILURE;

    DocIdManager        * docMgr  = DocIdManager::getInstance();
    ProfileManager      * pflMgr  = ProfileManager::getInstance();
    ProfileDocAccessor  * pflAcr  = pflMgr->getDocAccessor();
    const ProfileField  * nidFid  = pflAcr->getProfileField("nid");
    const ProfileField  * valFid  = pflAcr->getProfileField("category");

    IndexTerm * indexTerm     = NULL;
    int         nidNum        = 0;
    char      * nidArr[10240] = {0};
    MemPool     pool;

    nidNum = ac_str_splict( cmdp.nidList, cmdp.nidList + strlen(cmdp.nidList), ' ', nidArr, 10240 );

    for ( int i = 0; i < nidNum; i++ )
    {
        int64_t  nid   = strtoll( nidArr[i], NULL, 10 );
        uint32_t docId = docMgr->getDocId( nid );

        if ( docId == INVALID_DOCID )
        {
            fprintf(stderr, "can't find the nid in dict:%s\n", nidArr[i]);
            continue;
        }

        indexTerm = findTerm( cmdp , pool );
        if ( NULL == indexTerm ) break;

        if ( docId == indexTerm->seek( docId ))
        {
            //fprintf(stderr, "find the nid:%s\n", nidArr[i]);
            int32_t value = pflAcr->getInt32( docId, valFid );
            fprintf(stderr, "nid:%llu value:%d\n", nid, value);
        }
    }

    index_lib::destroy();
    fprintf(stderr, "cost %ld seconds.\n", time(NULL) - stime);
    return EXIT_SUCCESS;
}



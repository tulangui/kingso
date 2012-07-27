/*********************************************************
 *  Created on: 2011-5-27
 *      Author: yewang@taobao.com
 *
 *        Desc: 获取一个指定的term的长度
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


using namespace std;
using namespace index_lib;


typedef struct
{
    char    idxPath[ PATH_MAX ];                 // 索引路径
    char    idxField[ PATH_MAX ];                // 倒排字段
    char    term[ PATH_MAX ];                    // token
    char    psField[ PATH_MAX ];                 // 排序字段
    char    sortType[ PATH_MAX ];                // 0:递增排序   从小到大  ps; 1：递减排序   从大到小   _ps
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
    fprintf(stderr, "   -c \"idxPath\"     : 索引路径                              必填 \n");
    fprintf(stderr, "   -i \"idxField\"    : 倒排字段名称                       必填 \n");
    fprintf(stderr, "   -t \"term\"        : token         必填 \n");
    fprintf(stderr, "   -p \"psField\"     : 排序字段名称           \n");
    fprintf(stderr, "   -s \"sortType\"    : 0:从小到大  ps; 1：从大到小 _ps\n");
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
                strncpy( cmdp->idxPath,  optarg, sizeof(cmdp->idxPath) - 1 );
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
            case 'h':
                usage( argv[0] );
                break;
            default:
                break;
        }
    }

    /* 校验参数 */
    if (( cmdp->idxPath[0] == '\0' ) || ( cmdp->idxField[0] == '\0' ))
        usage( argv[0] );

    if (( cmdp->term[0] == '\0' ))
        usage( argv[0] );

    if (( cmdp->psField[0] != '\0' ) && ( cmdp->sortType[0] == '\0' ))
        usage( argv[0] );
}




/**
 * 加载index索引
 *
 * @param config
 *
 * @return
 */
int init_index_lib( const char * idxPath )
{
    IndexReader * reader = IndexReader::getInstance();

    if ( reader->open( idxPath, false) < 0)
    {
        fprintf(stderr, "open indexlib %s error\n", idxPath);
        return -1;
    }

    return 0;
}



IndexTerm * findTerm(CMD_PARAM &cmdp, MemPool &pool)
{
    IndexReader * reader = IndexReader::getInstance();

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
        if ( 0 == strcmp( cmdp.sortType, "0" ) )
            indexTerm = reader->getTerm( &pool, cmdp.idxField, cmdp.term, cmdp.psField, 0);

        if ( 0 == strcmp( cmdp.sortType, "1" ) )
            indexTerm = reader->getTerm( &pool, cmdp.idxField, cmdp.term, cmdp.psField, 1);

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
    MemPool     pool;

    parse_args( argc, argv, &cmdp );                  // 分析参数

    if ( -1 == init_index_lib( cmdp.idxPath ) )       // 初始化索引
        return EXIT_FAILURE;

    IndexTerm * indexTerm = findTerm( cmdp , pool );
    if ( NULL == indexTerm )
    {
        fprintf( stderr, "can't find Term\n" );
    }
    else
    {
        fprintf( stderr, "Term len: %d \n", indexTerm->getTermInfo()->docNum );
    }

    fprintf( stderr, "cost %ld seconds.\n", time(NULL) - stime );
    return EXIT_SUCCESS;
}



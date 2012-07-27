/********************************************************************
 * DocIdListCompare.cpp
 *
 *  Created on: 2011-6-9
 *      Author: yewang
 *      Desc  : 对比2分 query各自的nidlist,查看 有多个个docId是不同的
 *              文件格式：
 *                第一行是 nidList:1249461592 1044748972 ......(空格分隔)
 *                第二行是 生成这个nidList 对应的query  "/bin/search?auction?!et=13029"
 *
 *              计算过程:
 *                先根据基本版本的log, 建立 query -> nidList的 对应字典表
 *                然后读取截断版本的log， 找到 query和对应nidList
 *                然后去查询字典表， 查询同一个query对应的 未截断时的 nidList，
 *                比较在新列表中的nid 有多少是未出现在原列表中的
 ********************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>
#include <map>
#include <algorithm>
#include <functional>


using namespace std;


typedef struct
{
    char    baseFile[ PATH_MAX ];
    char    cutFile[ PATH_MAX ];
} CMD_PARAM;



typedef struct
{
    string    strQuery;
    string    nidList;
} QUERY;





/**
 * 使用说明
 *
 * @param name  程序名
 */
void usage(const char *name)
{
    fprintf(stderr, "\nUsage: %s \n", name);
    fprintf(stderr, "   -b \"baseFile\"        : 基本log \n");
    fprintf(stderr, "   -c \"cutFile\"         : 截断版本的log \n");
    fprintf(stderr, "   -h help \n");

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
    while ( (i = getopt(argc, argv, "b:c:h")) != EOF )
    {
        switch ( i )
        {
            case 'b':
                strncpy( cmdp->baseFile,  optarg, sizeof(cmdp->baseFile)  - 1 );
                break;
            case 'c':
                strncpy( cmdp->cutFile, optarg, sizeof(cmdp->cutFile) - 1 );
                break;
            case 'h':
                usage( argv[0] );
                break;
            default:
                break;
        }
    }

    if ( ( cmdp->cutFile[0] == '\0' ) || ( cmdp->baseFile[0] == '\0' ) )
    {
        usage( argv[0] );
    }
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





int loadLogToMap( const char * log, map<uint64_t, QUERY> & nidListMap )
{
    if ( log == NULL )  return  EXIT_FAILURE;

    FILE      * fp             = NULL;                // 文件句柄
    ssize_t     read           = 0;                   // 读取的字符个数
    char      * line           = NULL;                // 读取的行文本
    size_t      len            = 0;
    char      * nidPrefix      = "nidList:";
    int         nidPrefixLen   = strlen( nidPrefix );
    char      * queryPrefix    = "/bin/search?auction?";
    int         queryPrefixLen = strlen( queryPrefix );

    fp = fopen( log , "rb" );
    if ( !fp )
    {
        printf("open file %s failed. errno:%d", log, errno);
        return EXIT_FAILURE;
    }

    bool       hasQuery   = false;
    bool       hasNidList = false;
    char     * tmpNidList = NULL;
    char     * tmpQuery   = NULL;
    string     nidList;
    string     query;
    uint64_t   hash_val   = 0;
    char     * tmpArr[10] = {0};                       // 切分出的数组

    map< uint64_t, QUERY >::iterator it;

    while ( (read = getline( &line, &len, fp )) != -1 )
    {
        line[ read - 1 ] = '\0';

        if ( 2 != ac_str_splict( line, line +  strlen(line),  '\t' , tmpArr, 10 ) )
        {
            continue;
        }

        hash_val   = strtoull( tmpArr[0], NULL, 10 );
        tmpQuery   = strstr( tmpArr[1], queryPrefix );
        tmpNidList = strstr( tmpArr[1], nidPrefix );

        it = nidListMap.find( hash_val );

        if ( nidListMap.end() == it )
        {
            QUERY queryObj;

            if ( NULL != tmpQuery )    queryObj.strQuery.assign( tmpArr[1] );
            if ( NULL != tmpNidList )  queryObj.nidList.assign( tmpArr[1] );

            nidListMap.insert( pair<uint64_t, QUERY>( hash_val, queryObj ) );
        }
        else
        {
            if ( NULL != tmpQuery )    it->second.strQuery.assign( tmpArr[1] );
            if ( NULL != tmpNidList )
            {
                if ( it->second.nidList.length() == 0 )
                    it->second.nidList.assign( tmpArr[1] );

                if ( 0 != it->second.nidList.compare( tmpArr[1] ) )
                {
                    printf( "error: same query havn't same nidList:%;llu\n", it->first);

                    if ( strlen(tmpArr[1]) > it->second.nidList.length() )
                    {
                        it->second.nidList.assign( tmpArr[1] );
                    }
                }
            }
        }
    }

    if ( line ) free(line);
    fclose( fp );

    return 0;
}





/** 主程序入口  */
int main(int argc, char *argv[])
{
    CMD_PARAM             cmdp;
    int                   stime = time(NULL);
    map<uint64_t, QUERY>  baseMap;
    map<uint64_t, QUERY>  cutMap;

    parse_args(argc, argv, &cmdp);                    // 分析参数

    if ( 0 != loadLogToMap( cmdp.baseFile, baseMap ) )
    {
        return EXIT_FAILURE;
    }
    printf( "record number: %lu\n", baseMap.size() );


    // 开始读取 截断版本log
    if ( 0 != loadLogToMap( cmdp.cutFile, cutMap ) )
    {
        return EXIT_FAILURE;
    }
    printf( "record number: %lu\n", cutMap.size() );


    // 开始对比数据
    map<uint64_t, QUERY>::iterator  it;
    map<uint64_t, QUERY>::iterator  it2;

    char  * nidArr[10240]  = {0};                       // 切分出的数组
    int     nidNum         = 0;
    char  * nidArr2[10240] = {0};                       // 切分出的数组
    int     nidNum2        = 0;
    int     findNum        = 0;

    for ( it = baseMap.begin() ; it != baseMap.end(); it++ )
    {
        it2 = cutMap.find( it->first );

        QUERY& tmpQ  = it->second;
        QUERY& tmpQ2 = it2->second;

        if ( cutMap.end() == it2 )
        {
            printf( "error: can't find query:%llu\n", it->first );
        }
        else
        {
            if ( 0 != it2->second.nidList.compare( it->second.nidList ) )
            {
                // 现在query的 2个nidList结果不一致 ， 要拆开比对一下
                memset( nidArr,  0 , sizeof(nidArr) );
                memset( nidArr2, 0 , sizeof(nidArr2) );

                nidNum  = ac_str_splict( (char *)tmpQ.nidList.c_str(),  (char *)tmpQ.nidList.c_str() +  tmpQ.nidList.length(),  ' ' , nidArr,  10240 );
                nidNum2 = ac_str_splict( (char *)tmpQ2.nidList.c_str(), (char *)tmpQ2.nidList.c_str() + tmpQ2.nidList.length(), ' ' , nidArr2, 10240 );
                findNum = 0;

                for ( int i = 0; i < nidNum; i++ )
                {
                    for ( int j = 0; j < nidNum2; j++ )
                    {
                        if ( 0 == strcmp( nidArr[ i ], nidArr2[ j ] ) )
                        {
                            findNum++;
                            break;
                        }
                    }
                }

                if ( nidNum > 0 )
                {
                    printf( "diffQuery: %llu\n", it->first );
                    printf( "findNum: %d/%d\n", findNum, nidNum );
                }
                else
                {
                    printf( "baseQuery havn't nidList: %llu\n", it->first );
                }
            }
            else
            {
                printf( "sameQuery: %llu\n", it->first );
            }
        }
    }

    fprintf(stderr, "cost %ld seconds.\n", time(NULL) - stime);
    return EXIT_SUCCESS;
}


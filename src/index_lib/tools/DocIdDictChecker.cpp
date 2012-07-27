/*******************************************************************
 *  Created on: 2011-4-6
 *      Author: yewang@taobao.com  clm971910@gmail.com
 *
 *      Desc  : 检查从profile中生产的 dict是否完全对， 2边得数据能否对上
 *
 *      Data  : profile.title    给profile用的数据的 字段名列表 , 只有一行 用Ctrl+A分隔
 *              profile.txt      给profile用的数据
 *                               第一列是docid, 一行一个宝贝 用Ctrl+A Ctrl+B分隔
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <time.h>
#include <errno.h>
#include <sys/stat.h>

#include "util/Log.h"
#include "commdef/commdef.h"
#include "index_lib/IndexLib.h"
#include "index_lib/DocIdManager.h"



using namespace std;
using namespace index_lib;





typedef struct
{
    char    titleFile[PATH_MAX];                      // profile.title profile的数据字段名列表
    char    detailFile[PATH_MAX];                     // profile.txt   profile数据
    char    inputPath[PATH_MAX];                     // 输出数据的路径
} CMD_PARAM;





/** ================================================================= */


/**
 * 使用说明
 *
 * @param name  程序名
 */
void usage(const char *name)
{
    fprintf(stderr, "\nUsage: %s \n", name);
    fprintf(stderr, "   -t \"profile.title\"          : profile的数据字段名列表 (必填)  \n");
    fprintf(stderr, "   -d \"profile.sorted.txt\"     : profile数据 (必填) \n");
    fprintf(stderr, "   -i \"/input/path\"            : dict路径  (必填) 包含文件  docId.dict\n");
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
    while ( (i = getopt(argc, argv, "t:d:i:h")) != EOF )
    {
        switch ( i )
        {
            case 't':
                strncpy( cmdp->titleFile, optarg, sizeof(cmdp->titleFile) - 1 );
                break;
            case 'd':
                strncpy( cmdp->detailFile, optarg, sizeof(cmdp->detailFile) - 1 );
                break;
            case 'i':
                strncpy( cmdp->inputPath, optarg, sizeof(cmdp->inputPath) - 1 );
                break;
            case 'h':
                usage( argv[0] );
                break;
            default:
                break;
        }
    }

    /* 校验参数 */
    if ( ( cmdp->titleFile[0] == '\0' )
            || ( cmdp->detailFile[0] == '\0' )
            || ( cmdp->inputPath[0] == '\0' ) )
    {
        usage(argv[0]);
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








int main(int argc, char *argv[])
{
    CMD_PARAM      cmdp;
    DocIdManager * dmgr = DocIdManager::getInstance();

    alog::Configurator::configureRootLogger();

    int        stime = time(NULL);
    FILE     * fp    = NULL;                          // 文件句柄
    ssize_t    read;                                  // 读取的字符个数
    char     * line  = NULL;                          // 读取的行文本
    size_t     len   = 0;
    char     * nameArr[1024] = {0};                   // 切分出的字段名的数组
    int        nameNum    = 0;                        // 字段个数
    int        nidIndex   = -1;                       // nid字段在第几列
    int        docIdIndex = 0;                        // docId就是第一列

    parse_args(argc, argv, &cmdp);                    // 分析参数

    // ===================== 读取分析 头信息  ===================
    fp = fopen( cmdp.titleFile , "rb" );
    if ( !fp )
    {
        TERR("open file %s failed. errno:%d", cmdp.titleFile, errno);
        return EXIT_FAILURE;
    }

    if ( (read = getline(&line, &len, fp)) != -1 )
    {
        line[ read - 1 ] = '\0';
        nameNum = ac_str_splict( line, line + read - 1, '\001', nameArr, sizeof(nameArr) );
    }
    fclose( fp );

    // 判断一下 nid在第几个字段
    for ( int i = 0; i < nameNum; ++i )
    {
        if ( 0 == strcmp("nid", nameArr[i]) )  nidIndex = i;
    }

    if ( nidIndex == -1 )
    {
        fprintf(stderr, "can not find nid field.\n");
        return EXIT_FAILURE;
    }


    // 加载字典
    dmgr->load( cmdp.inputPath );


    // ================ 顺序读取 所有的记录， 取出 nid 和 docId ==============
    int64_t    nid;
    uint32_t   docId;
    char     * valArr[1024] = {0};                    // 切分出的字段值的数组
    int        valNum       = 0;                      // 值个数

    fp = fopen( cmdp.detailFile , "rb" );
    if ( !fp )
    {
        TERR("open file %s failed. errno:%d", cmdp.detailFile, errno);
        return EXIT_FAILURE;
    }

    while ( (read = getline(&line, &len, fp)) != -1 )
    {
        line[ read - 1 ] = '\0';
        valNum = ac_str_splict( line, line + read - 1, '\001' , valArr, sizeof(valArr) );

        if ( valNum != nameNum )
        {
            TERR("fieldNum not same with *.title, %s", line);
            return EXIT_FAILURE;
        }

        docId = (uint32_t) strtoull( valArr[ docIdIndex ], NULL, 10 );
        nid   = strtoll( valArr[ nidIndex ], NULL, 10 );

        if ( docId != dmgr->getDocId( nid ) )
        {
            TERR("error: nid %ul, not docId: %u", nid, docId);
            return EXIT_FAILURE;
        }

        if ( nid != dmgr->getNid( docId ) )
        {
            TERR("error: nid %ul, not docId: %u", nid, docId);
            return EXIT_FAILURE;
        }
    }
    if ( line ) free(line);
    fclose( fp );

    fprintf(stderr, "check success. cost %ld seconds.\n", time(NULL) - stime);
    return EXIT_SUCCESS;
}





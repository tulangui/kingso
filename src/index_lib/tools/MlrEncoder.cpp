/******************************************************************
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 采用 varint来编码 mlr字段， 输出， 查看压缩的大小
 *
 ******************************************************************/



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

#include "index_lib/varint.h"


using namespace std;
using namespace index_lib;




typedef struct
{
    char    inputFile[PATH_MAX];
    char    outputFile[PATH_MAX];
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
    fprintf(stderr, "   -i \"inputFile\"        : 输入文件 (必填) 格式是 一行多个数字 空格分隔 \n");
    fprintf(stderr, "   -o \"outputFile\"       : 输出文件 (必填) \n");
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
    while ( (i = getopt(argc, argv, "i:o:h")) != EOF )
    {
        switch ( i )
        {
            case 'i':
                strncpy( cmdp->inputFile, optarg, sizeof(cmdp->inputFile) - 1 );
                break;
            case 'o':
                strncpy( cmdp->outputFile, optarg, sizeof(cmdp->outputFile) - 1 );
                break;
            case 'h':
                usage( argv[0] );
                break;
            default:
                break;
        }
    }

    /* 校验参数 */
    if ( ( cmdp->inputFile[0] == '\0' )
            || ( cmdp->outputFile[0] == '\0' ) )
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

    int        stime  = time(NULL);
    FILE     * fp     = NULL;                         // 文件句柄
    FILE     * fp_wr  = NULL;                         // 文件句柄
    ssize_t    read;                                  // 读取的字符个数
    char     * line   = NULL;                         // 读取的行文本
    size_t     len    = 0;
    int        valNum = 0;                            // 字段个数
    char     * valArr[1024] = {0};                    // 切分出的数组
    uint32_t   intArr[1024] = {0};
    uint8_t    buf[65535];
    uint8_t  * tmpPos = NULL;

    parse_args(argc, argv, &cmdp);                    // 分析参数

    fp = fopen( cmdp.inputFile , "rb" );
    if ( !fp )
    {
        printf("open file %s failed. errno:%d", cmdp.inputFile, errno);
        return EXIT_FAILURE;
    }

    fp_wr = fopen( cmdp.outputFile , "wb" );
    if ( !fp_wr )
    {
        printf("open file %s failed. errno:%d", cmdp.outputFile, errno);
        fclose(fp);
        return EXIT_FAILURE;
    }

    while ( (read = getline(&line, &len, fp)) != -1 )
    {
        memset(buf, 0, sizeof(buf));
        tmpPos = buf;

        valNum = ac_str_splict( line, line + read, ' ' , valArr, sizeof(valArr) );

        int intNum = 0;
        for (int i = 0; i < valNum; i++)
        {
            if ( strlen( valArr[i] ) == 0) continue;
            if ( ! isdigit(valArr[i][0]) ) continue;

            intArr[ intNum ] = zigZag_encode32( atoi(valArr[i]) );
            intNum ++;
        }

        int j = 0;
        for ( j = 0; j < (intNum / 4); j ++)
        {
            tmpPos = group_varint_encode_uint32( &intArr[ j * 4 ], tmpPos);
        }

        for ( j = j * 4; j < intNum; j ++)
        {
            tmpPos = varint_encode_uint32( intArr[j], tmpPos);
        }

        // 输出到文件中去
        if ( fwrite( &buf, 1, (tmpPos - buf), fp_wr ) != (tmpPos - buf) )
        {
            printf("write failed. errno:%d", errno);
            return EXIT_FAILURE;
        }
    }
    if ( line ) free(line);

    fclose( fp );
    fclose( fp_wr );

    fprintf(stderr, "cost %ld seconds.\n", time(NULL) - stime);
    return EXIT_SUCCESS;
}




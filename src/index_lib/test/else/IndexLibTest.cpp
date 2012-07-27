/********************************
 *
 * 测试 IndexLib.cpp的 库加载和释放功能
 *
 *********************************/



#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>


#include "index_lib/IndexLib.h"


using namespace index_lib;



int main(int argc, char* argv[])
{
    if ( argc < 2 )
    {
        fprintf(stderr, " %s : searcher.xml\n", argv[0]);
        return -1;
    }

    /** 下面是用于 测试性能的  */
    struct timeval  tv_start;                    /* 起始时间  */
    struct timeval  tv_end;                      /* 结束时间 */
    struct timeval  tv_total;                    /* 耗时  */

    gettimeofday(&tv_start, NULL);               /* 开始计时 */

    // ================== 初始化index_lib ================
    mxml_node_t * pXMLTree = NULL;
    FILE        * fp       = fopen( argv[1], "r" );

    if ( fp == NULL )
    {
        fprintf(stderr, "open %s failed\n", argv[1]);
        return -1;
    }

    pXMLTree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
    fclose(fp);

    if ( NULL == pXMLTree )
    {
        fprintf(stderr, "file %s not xml\n", argv[1]);
        return -1;
    }

    if ( 0 != index_lib::init( pXMLTree ) )
    {
        fprintf(stderr, "init index_lib failed\n");
        return -1;
    }
    mxmlDelete(pXMLTree);

    // 关闭索引库
    index_lib::destroy();

    gettimeofday(&tv_end, NULL);                 /* 结束计时 */

    timersub(&tv_end, &tv_start, &tv_total);
    printf("time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);

    return 0;
}

/*************************************************
 * InlineTest.cpp
 *
 *  Created on: 2011-5-16
 *      Author: yewang
 *      Desc  : 测试inline和 虚函数的性能差别
 *************************************************/

#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#include "ClassA.h"



int main(int argc, char * argv[])
{
    /** 下面是用于 测试性能的  */
    struct timeval  tv_start;                    /* 起始时间  */
    struct timeval  tv_end;                      /* 结束时间 */
    struct timeval  tv_total;                    /* 耗时  */

    A * objA = NULL;

    if ( argc > 1 )
    {
        objA = new B();
    }
    else
    {
        objA = new C();
    }

    gettimeofday(&tv_start, NULL);               /* 开始计时 */

    for (uint32_t i = 0; i < 100000000; i++)
    {
        objA->seek( i );
    }

    gettimeofday(&tv_end, NULL);                 /* 结束计时 */

    timersub(&tv_end, &tv_start, &tv_total);
    printf("time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);

}

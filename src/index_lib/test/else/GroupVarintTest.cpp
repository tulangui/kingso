/********************************
 *
 * 测试group varint的性能
 *
 *********************************/

#include <getopt.h>
#include <sys/time.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>


#include "index_lib/varint.h"


using namespace index_lib;



int main(int argc, char* argv[])
{
    /** 下面是用于 测试性能的  */
    struct timeval  tv_start;                    /* 起始时间  */
    struct timeval  tv_end;                      /* 结束时间 */
    struct timeval  tv_total;                    /* 耗时  */

    uint32_t  * intArr = (uint32_t *) calloc(1000000 , sizeof(uint32_t));
    uint8_t   * buf    = (uint8_t *) calloc(5000000 , sizeof(uint8_t));;
    uint32_t  * outArr = (uint32_t *) calloc(1000000 , sizeof(uint32_t));

    for ( uint32_t i = 0; i < 1000000; i++ )
    {
        srand(i);
        intArr[i] = zigZag_encode32(rand());
    }

    // ************ 先编码  ******************
    gettimeofday(&tv_start, NULL);               /* 开始计时 */

    uint8_t * target = buf;
    for ( uint32_t i = 0; i < (1000000/4); i++ )
    {
        target = group_varint_encode_uint32( &intArr[ i * 4 ], target);
    }

    gettimeofday(&tv_end, NULL);                 /* 结束计时 */

    timersub(&tv_end, &tv_start, &tv_total);
    printf("encode time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);


    // ************ 再解码  ******************
    gettimeofday(&tv_start, NULL);               /* 开始计时 */

    const uint8_t * tmp = buf;
    for ( uint32_t i = 0; i < (1000000/4); i++ )
    {
        tmp = group_varint_decode_uint32(tmp, &outArr[ i * 4 ]);
    }
    gettimeofday(&tv_end, NULL);                 /* 结束计时 */

    timersub(&tv_end, &tv_start, &tv_total);
    printf("decode time consumed: %ld.%06ld s\n", tv_total.tv_sec, tv_total.tv_usec);


    for ( uint32_t i = 0; i < 1000000; i++ )
    {
        if ( intArr[i] != outArr[i])
            printf("diff at index: %u; value: %u", i, intArr[i]);
    }

    return EXIT_SUCCESS;
}

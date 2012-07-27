/*********************************************************
 * BuildHFIndex.cpp
 *
 *  Created on: 2011-5-27
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *        Desc: 在buildIndex和buildProfile运行完后 运行，加载索引
 *              根据配置的ps排序字段,读取对应的profile属性 进行预先排序 做截断, 并将新的链条加入索引
 *
 *      Modify: 取消stl的map， 将多线程拆分的更细  。  yewang@taobao.coms
 *
 *********************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <iostream>
#include <fstream>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <sys/time.h>
#include <locale.h>

//#include "alog/Logger.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/ProfileManager.h"
#include "index_lib/ProfileDocAccessor.h"
#include "index_lib/ProfileStruct.h"
#include "index_lib/IndexBuilder.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexReader.h"
#include "../index/index_struct.h"
#include "util/Vector.h"
#include "util/hashmap.h"


using namespace std;
using namespace util;
using namespace index_lib;


// 命令行参数
typedef struct
{
    char    cfg_file [ PATH_MAX ];               // 高频词配置文件
    int     thread_num;                          // 并行线程数
} CMD_PARAM;


// 排序类型
enum HF_SORT_TYPE
{
    HF_ASC   = 0,                                // 递增排序   从小到大     ps
    HF_DESC  = 1                                 // 递减排序   从大到小     _ps
};

// 排序字段
typedef struct
{
    char          field_name[ PATH_MAX ];        // 排序字段名
    HF_SORT_TYPE  sort_type;                     // 排序类型
} CFG_PS_FIELD;


// 截断配置
typedef struct
{
    uint32_t          min_doc_num;               // 被认为是高频词的链条最小长度
    uint32_t          cutoff_doc_num;            // 高频词的链条 截断长度
    uint32_t          min_userId_num;            // 每个截断链内部的doc对应的独立user_id的个数
    uint32_t          ps_fields_num;             // 排序字段个数
    CFG_PS_FIELD      ps_fields[ 1024 ];         // 排序字段
} HF_CFG;


// 一个截断任务  是 倒排字段名  + 排序字段的组合
typedef struct
{
    char               * idx_field;              // 倒排字段名
    CFG_PS_FIELD       * ps_filed;               // 排序字段
    pthread_spinlock_t * spin_lock;              // 倒排字段级别的锁
    Vector< uint64_t > * sign_vect;              // 倒排字段的签名集合
    uint32_t             max_doc_num;            // 倒排字段中最长term的doc_num
} HF_TASK;





#pragma pack (1)
template<typename T>
class SortPair
{
public:
    SortPair(uint32_t a, T b, uint8_t c):docId(a), value(b), occ(c) {}
    SortPair(){}

    uint32_t  docId;
    T         value;
    uint8_t   occ;
};
#pragma pack ()


typedef  SortPair< int32_t >     SortPairInt32;
typedef  SortPair< int64_t >     SortPairInt64;
typedef  SortPair< uint64_t >    SortPairUInt64;
typedef  SortPair< float >       SortPairFloat;
typedef  SortPair< double >      SortPairDouble;



/** ===================== 全局的 相关变量  ============== */
static HF_CFG              g_hf_config;                    // 全局的 高频词 配置
static CMD_PARAM           g_cmd_para;                     // 命令行参数
static pthread_spinlock_t  g_thread_lock;                  // 线程级别的锁, 所有线程共享一个

static HF_TASK             g_hf_task_arr[1024];            // 需要执行的任务数组
static uint32_t            g_hf_task_count;
static uint32_t            g_hf_task_cur;                  // 当前可以被用来执行的任务索引
static pthread_spinlock_t  g_idx_lock_arr[1024];           // 倒排字段级别的锁, 每个字段的多个线程共享一个
static Vector< uint64_t >  g_idx_sign_vect_arr[1024];      // 所有倒排字段的签名vect的数组
static uint32_t            g_max_doc_num_arr[1024];        // 所有倒排字段的签名的最大的doc_num


/** 希尔排序需要用的长度数组 */
const uint32_t INCREMENT[] = {1,4,10,23,53,97,193,389,769,1543,3079,6151,12289,24593,49157,98317,196613,393241,786433,1572869,3145739,6291469,12582917,25165843,50331653,100663319,201326611,402653189,805306457};


/** ================================================================= */

/**
 * 使用说明
 *
 * @param name  程序名
 */
void usage(const char * name)
{
    fprintf(stderr, "\nUsage: %s \n", name);
    fprintf(stderr, "   -c \"hf_index_cfg.xml\"   : 高频词配置文件(xml) 必填 \n");
    fprintf(stderr, "   -p \"16\"                 : 并行的线程数， 默认 16, 最多64个 \n");
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
    while ( (i = getopt(argc, argv, "c:p:h")) != EOF )
    {
        switch ( i )
        {
            case 'c':
                strncpy( cmdp->cfg_file, optarg, sizeof(cmdp->cfg_file) - 1 );
                break;
            case 'p':
                cmdp->thread_num = atoi( optarg );
                break;
            case 'h':
                usage( argv[0] );
                break;
            default:
                break;
        }
    }

    /* 校验参数 */
    if ( cmdp->cfg_file[0] == '\0' )  usage( argv[0] );
    if ( cmdp->thread_num > 64 )      cmdp->thread_num = 64;    // 最大
    if ( cmdp->thread_num <= 0 )      cmdp->thread_num = 16;    // 默认
}


/**
 * 初始化 alog相关的配置
 *
 * @return  -1：失败  0：成功
 */
/**
int init_alog()
{
    setlocale( LC_ALL, "zh_CN.UTF-8" );

    alog::Appender * p_appender = alog::ConsoleAppender::getAppender();

    if ( !p_appender )
    {
        fprintf( stderr, "Get alog::ConsoleAppender::getAppender() failed.\n" );
        return -1;
    }

    alog::Configurator::configureRootLogger();
    alog::Logger::MAX_MESSAGE_LENGTH = 10240;
    alog::Logger::getRootLogger()->setAppender( p_appender );

    return 0;
}
*/

/**
 * 对数组的前  nr 个元素 进行 希尔排序   从小到大排序
 *
 * @param array
 * @param nr
 *
 * @return
 */
int  shell_sort_int32( uint32_t * array, uint32_t nr )
{
    int start  = 0;
    int end    = nr - 1;
    int incIdx = -1;

    /* 找到合适的步长 */
    for (int i = (sizeof(INCREMENT)/sizeof(int) - 1); i >= 0; i--)
    {
        if (INCREMENT[i] < nr)
        {
            incIdx = i;
            break;
        }
    }

    /* 开始循环比较 */
    while (incIdx >= 0)
    {
        int increment = INCREMENT[incIdx];
        int startInc  = start + increment;

        for (int i = startInc; i <= end; i++)
        {
            int       j   = i;
            uint32_t  tmp = array[i];

            while ((j >= startInc) && ( tmp < array[j - increment] ))
            {
                array[j] = array[j - increment];

                j -= increment;
            }
            array[j] = tmp;
        }

        incIdx--;   /* 换一种步长 */
    }

    return 0;
}




/**
 * 对数组的前  nr 个元素 进行 希尔排序    从小到大排序
 *
 * @param array
 * @param nr
 *
 * @return
 */
int  shell_sort_idx2_nzip( DocListUnit * array, uint32_t nr)
{
    int start  = 0;
    int end    = nr - 1;
    int incIdx = -1;

    /* 找到合适的步长 */
    for (int i = (sizeof(INCREMENT)/sizeof(int) - 1); i >= 0; i--)
    {
        if (INCREMENT[i] < nr)
        {
            incIdx = i;
            break;
        }
    }

    /* 开始循环比较 */
    while (incIdx >= 0)
    {
        int increment = INCREMENT[incIdx];
        int startInc  = start + increment;

        for (int i = startInc; i <= end; i++)
        {
            int          j   = i;
            DocListUnit  tmp = array[i];

            while (( j >= startInc )
                    && (  ( tmp.doc_id < array[j - increment].doc_id )
                         || (( tmp.doc_id == array[j - increment].doc_id ) && ( tmp.occ == array[j - increment].occ ) )
                        ))
            {
                array[j] = array[j - increment];

                j -= increment;
            }
            array[j] = tmp;
        }

        incIdx--;   /* 换一种步长 */
    }

    return 0;
}




/**
 * 对数组的前  nr 个元素 进行 希尔排序  从小到大排序
 *
 * @param array
 * @param nr
 *
 * @return
 */
template<typename T>
int  shell_sort_sortPair_less( SortPair<T> * array, uint32_t nr)
{
    int start  = 0;
    int end    = nr - 1;
    int incIdx = -1;

    /* 找到合适的步长 */
    for (int i = (sizeof(INCREMENT)/sizeof(int) - 1); i >= 0; i--)
    {
        if (INCREMENT[i] < nr)
        {
            incIdx = i;
            break;
        }
    }

    /* 开始循环比较 */
    while (incIdx >= 0)
    {
        int increment = INCREMENT[incIdx];
        int startInc  = start + increment;

        for (int i = startInc; i <= end; i++)
        {
            int          j   = i;
            SortPair<T>  tmp = array[i];

            while ((j >= startInc) && ( tmp.value < array[j - increment].value ))
            {
                array[j] = array[j - increment];

                j -= increment;
            }
            array[j] = tmp;
        }

        incIdx--;   /* 换一种步长 */
    }

    return 0;
}



/**
 * 对数组的前  nr 个元素 进行 希尔排序  从大到小排序
 *
 * @param array
 * @param nr
 *
 * @return
 */
template<typename T>
int  shell_sort_sortPair_greater( SortPair<T> * array, uint32_t nr)
{
    int start  = 0;
    int end    = nr - 1;
    int incIdx = -1;

    /* 找到合适的步长 */
    for (int i = (sizeof(INCREMENT)/sizeof(int) - 1); i >= 0; i--)
    {
        if (INCREMENT[i] < nr)
        {
            incIdx = i;
            break;
        }
    }

    /* 开始循环比较 */
    while (incIdx >= 0)
    {
        int increment = INCREMENT[incIdx];
        int startInc  = start + increment;

        for (int i = startInc; i <= end; i++)
        {
            int          j   = i;
            SortPair<T>  tmp = array[i];

            while ((j >= startInc) && ( tmp.value > array[j - increment].value ))
            {
                array[j] = array[j - increment];

                j -= increment;
            }
            array[j] = tmp;
        }
        incIdx--;   /* 换一种步长 */
    }

    return 0;
}





/**
 * 只对数组，进行粗排。 只取出一部分符合条件的集合
 *
 * @param array    指针数组
 * @param num      总的记录个数
 *
 * 返回  截取的部分的元素个数
 */
template<typename T>
uint32_t quick_sort_sortPair_less( SortPair< T > * array,
                                   uint32_t        num)
{
    return_val_if_fail( array != NULL, 0 );

    /* 第一个元素 */
    uint32_t      left  = 0;
    uint32_t      right = num - 1;
    SortPair< T > x     = array[ 0 ];

    while (left < right)
    {
        while ( (array[right].value >= x.value) && left < right)
        {
            right--;
        }

        if (left != right)
        {
            array[left] = array[right];
            left++;
        }

        while ( (array[left].value <= x.value) && left < right)
        {
            left++;
        }

        if (left != right)
        {
            array[right] = array[left];
            right--;
        }
    }
    array[ left ] = x;

    return left;
}


template<typename T>
uint32_t quick_sort_sortPair_greater( SortPair< T > * array,
                                      uint32_t        num )
{
    return_val_if_fail( array != NULL, 0 );

    /* 第一个元素 */
    uint32_t      left  = 0;
    uint32_t      right = num - 1;
    SortPair< T > x     = array[ 0 ];

    while (left < right)
    {
        while ( (array[right].value <= x.value) && left < right)
        {
            right--;
        }

        if (left != right)
        {
            array[left] = array[right];
            left++;
        }

        while ( (array[left].value >= x.value) && left < right)
        {
            left++;
        }

        if (left != right)
        {
            array[right] = array[left];
            right--;
        }
    }
    array[ left ] = x;

    return left;
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
        result_arr[ cnt ] = begin;
        ++cnt;

        while ( ('\0' != *begin) && (delim != *begin) )
        {
            ++begin;                   /* 循环一直到  delim 或者  \0 结束 */
        }

        *begin = '\0';                 /* 找到了分隔符 截断 */
        ++begin;

        if ( (begin >= end) || (cnt >= result_arr_len) )
        {
            result_arr[ cnt ] = begin;
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

    if ( 0 != index_lib::init_rebuild( pXMLTree ) )
    {
        fprintf(stderr, "init index_lib for rebuild failed\n");
        return -1;
    }

    mxmlDelete( pXMLTree );
    return 0;
}




/**
 * 解析高频词配置文件
 *
 * @param config
 *
 * @return  -1：error
 */
int parse_hf_cfg( const char * config )
{
    FILE         * fp                  = fopen( config, "r" );
    mxml_node_t  * pXMLTree            = NULL;
    mxml_node_t  * min_doc_num_node    = NULL;
    const char   * min_doc_num         = NULL;
    mxml_node_t  * cutoff_doc_num_node = NULL;
    const char   * cutoff_doc_num      = NULL;
    mxml_node_t  * min_userId_num_node = NULL;
    const char   * min_userId_num      = NULL;
    mxml_node_t  * ps_fields_node      = NULL;
    mxml_node_t  * ps_field_node       = NULL;

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

    //***************************
    min_doc_num_node = mxmlFindElement(pXMLTree, pXMLTree, "hf_min_doc_num", NULL, NULL, MXML_DESCEND);
    if ( min_doc_num_node == NULL || min_doc_num_node->parent != pXMLTree )
    {
        fprintf(stderr, "can't find hf_min_doc_num's node\n");
        return -1;
    }

    min_doc_num = mxmlElementGetAttr( min_doc_num_node, "value" );
    if ( NULL == min_doc_num || strlen( min_doc_num ) <= 0 )
    {
        fprintf(stderr, "must set hf_min_doc_num's value\n");
        return -1;
    }

    //***************************
    cutoff_doc_num_node = mxmlFindElement(pXMLTree, pXMLTree, "hf_cutoff_doc_num", NULL, NULL, MXML_DESCEND);
    if ( cutoff_doc_num_node == NULL || cutoff_doc_num_node->parent != pXMLTree )
    {
        fprintf(stderr, "can't find hf_cutoff_doc_num's node\n");
        return -1;
    }

    cutoff_doc_num = mxmlElementGetAttr( cutoff_doc_num_node, "value" );
    if ( NULL == cutoff_doc_num || strlen( cutoff_doc_num ) <= 0 )
    {
        fprintf(stderr, "must set hf_cutoff_doc_num's value\n");
        return -1;
    }

    //***************************
    min_userId_num_node = mxmlFindElement(pXMLTree, pXMLTree, "hf_min_userId_num", NULL, NULL, MXML_DESCEND);
    if ( min_userId_num_node == NULL || min_userId_num_node->parent != pXMLTree )
    {
        fprintf(stderr, "can't find hf_min_userId_num's node\n");
        return -1;
    }

    min_userId_num = mxmlElementGetAttr( min_userId_num_node, "value" );
    if ( NULL == min_userId_num || strlen( min_userId_num ) <= 0 )
    {
        fprintf(stderr, "must set hf_min_userId_num's value\n");
        return -1;
    }

    // 取出其中的值
    g_hf_config.min_doc_num    = strtoul( min_doc_num,    NULL, 10 );
    g_hf_config.cutoff_doc_num = strtoul( cutoff_doc_num, NULL, 10 );
    g_hf_config.min_userId_num = strtoul( min_userId_num, NULL, 10 );
    g_hf_config.ps_fields_num  = 0;

    //***************************
    ps_fields_node = mxmlFindElement(pXMLTree, pXMLTree, "hf_ps_fields", NULL, NULL, MXML_DESCEND);
    if ( ps_fields_node == NULL || ps_fields_node->parent != pXMLTree )
    {
        fprintf(stderr, "can't find hf_ps_fields's node\n");
        return -1;
    }

    ps_field_node = mxmlFindElement(ps_fields_node, pXMLTree, "field", NULL, NULL, MXML_DESCEND_FIRST);

    while ( ps_field_node != NULL )                   // 循环取其中的子节点
    {
        const char * name      = mxmlElementGetAttr( ps_field_node, "name" );
        const char * sort_type = mxmlElementGetAttr( ps_field_node, "sort_type" );

        if ( NULL == name || strlen( name ) <= 0
                || NULL == sort_type || strlen( sort_type ) <= 0 )
        {
            fprintf(stderr, "must set field's name and sort_type value\n");
            return -1;
        }

        strncpy( g_hf_config.ps_fields[ g_hf_config.ps_fields_num ].field_name, name, PATH_MAX );

        if ( 0 == strcmp( "ASC", sort_type ) )
        {
            g_hf_config.ps_fields[ g_hf_config.ps_fields_num ].sort_type = HF_ASC;
        }
        else if ( 0 == strcmp( "DESC", sort_type ) )
        {
            g_hf_config.ps_fields[ g_hf_config.ps_fields_num ].sort_type = HF_DESC;
        }
        else
        {
            fprintf(stderr, "sort_type must be ASC or DESC\n");
            return -1;
        }

        g_hf_config.ps_fields_num ++;

        ps_field_node = mxmlFindElement(ps_field_node, pXMLTree, "field", NULL, NULL, MXML_NO_DESCEND);
    }

    mxmlDelete(pXMLTree);

    if ( g_hf_config.ps_fields_num <= 0 )
    {
        fprintf(stderr, "must have one ps field at least\n");
        return -1;
    }
    return 0;
}



/**
 * 重新创建 userIdMap
 *
 * @param map  传入的map指针， 如果已经存在 则释放后，重新创建
 *
 * @return
 */
hashmap_t userId_map_renew( hashmap_t map )
{
    if ( NULL != map )
    {
        hashmap_destroy( map, NULL, NULL );
        map = NULL;
    }

    map = hashmap_create( 3145739, HM_FLAG_POOL, hashmap_hashInt, hashmap_cmpInt );
    if ( NULL == map )
    {
        fprintf(stderr, "create userId map failed.\n" );
        return NULL;
    }

    return map;
}


/**
 *  对数组 按照指定排序方式 进行排序 , 取保指定部分是排好序的
 *
 * @param vect                   放所有元素的vect
 * @param ps_field               排序字段属性
 * @param start                  vect中的起始位置
 * @param cut_num                从起始位置开始 需要确保排好序的数量
 *
 * @return 实际被排好序的记录个数
 */
template<typename T>
int sortPair_vect_sort( Vector< SortPair < T > >  & vect,
                        const CFG_PS_FIELD        * ps_field,
                        uint32_t                    start,
                        uint32_t                    cut_num)
{
    uint32_t size = vect.size() - start;
    uint32_t left = 0;

    if ( HF_ASC == ps_field->sort_type )                   // 从小到大进行排序
    {
        for ( int i = 0; i < 3 ; i ++ )
        {
            if ( start > 0 )       break;
            left = quick_sort_sortPair_less( vect.begin() + start , size );

            if ( 0 == left )       break;
            if ( left <= cut_num ) break;

            size = left;
            if (( left < cut_num * 5 ) && ( left > cut_num ))  break;
        }

        shell_sort_sortPair_less( vect.begin() + start, size );

        return size;
    }
    else if ( HF_DESC == ps_field->sort_type )             // 从大到小进行排序
    {
        for ( int i = 0; i < 3 ; i ++ )
        {
            if ( start > 0 )       break;
            left = quick_sort_sortPair_greater( vect.begin() + start , size );

            if ( 0 == left )       break;
            if ( left <= cut_num ) break;

            size = left;
            if (( left < cut_num * 5 ) && ( left > cut_num ))  break;
        }

        shell_sort_sortPair_greater( vect.begin() + start, size );

        return size;
    }

    return 0;
}




/**
 * 添加截断索引
 *
 * @return
 */
void add_hf_term( const char    * idx_field,
            const CFG_PS_FIELD  * ps_field,
                  uint64_t        term_sign,
                  uint64_t        new_sign,
                  uint32_t      * docid_arr,
                  int             docid_num,
                  int             old_docid_num
                 )
{
    if ( NULL == idx_field ) return;
    if ( NULL == docid_arr ) return;

    IndexBuilder  * builder = IndexBuilder::getInstance();

    // 将索引作为新的输出 插入
    if ( builder->addTerm( idx_field, new_sign, docid_arr, docid_num ) <= 0)
    {
        fprintf(stderr, "failed. idx field:%s ps field:%s old:%lu len:%u new:%lu len:%d\n",
                       idx_field, ps_field->field_name, term_sign, old_docid_num, new_sign, docid_num);
    } else {
        fprintf(stderr, "succ. idx field:%s ps field:%s old:%lu len:%u new:%lu len:%d\n",
                       idx_field, ps_field->field_name, term_sign, old_docid_num, new_sign, docid_num);
    }
}


/**
 * 添加截断索引
 *
 * @return
 */
void add_hf_term_occ( const char    * idx_field,
                const CFG_PS_FIELD  * ps_field,
                      uint64_t        term_sign,
                      uint64_t        new_sign,
                      DocListUnit   * docid_arr,
                      int             docid_num,
                      int             old_docid_num
                    )
{
    if ( NULL == idx_field ) return;
    if ( NULL == docid_arr ) return;

    IndexBuilder  * builder = IndexBuilder::getInstance();

    // 将索引作为新的输出 插入
    if ( builder->addTerm( idx_field, new_sign, docid_arr, docid_num ) <= 0)
    {
        fprintf(stderr, "failed. idx field:%s ps field:%s old:%lu len:%u new:%lu len:%d\n",
                       idx_field, ps_field->field_name, term_sign, old_docid_num, new_sign, docid_num);
    } else {
        fprintf(stderr, "succ. idx field:%s ps field:%s old:%lu len:%u new:%lu len:%d\n",
                       idx_field, ps_field->field_name, term_sign, old_docid_num, new_sign, docid_num);
    }
}



/**
 * 获取一个倒排字段的 所有 term
 *
 * @param field_name   倒排字段名
 * @param vect         输出用的签名集合
 *
 * @return 一个field的所有term包含的最大docnum
 */
uint32_t get_idx_all_term( const char * field_name, Vector< uint64_t > & vect )
{
    if ( NULL == field_name ) return 0;

    IndexBuilder         * builder     = IndexBuilder::getInstance();
    uint64_t               term_sign   = 0;
    uint32_t               max_doc_num = 0;
    const IndexTermInfo  * term_info   = builder->getFirstTermInfo( field_name, term_sign );

    while ( NULL != term_info )
    {
        if ( term_info->docNum <= g_hf_config.min_doc_num )
        {
            term_info = builder->getNextTermInfo( field_name, term_sign );    // 取下一个 term
            continue;
        }

        if ( max_doc_num < term_info->docNum )
            max_doc_num = term_info->docNum;

        vect.pushBack( term_sign );
        term_info = builder->getNextTermInfo( field_name, term_sign );    // 取下一个 term
    }

    return max_doc_num;
}



#define CUTOFF_BY_FIELD( PAIR_TYPE, VAl_TYPE, S_FUNC, M_FUNC )                                      \
    IndexReader               * reader     = IndexReader::getInstance();                            \
    hashmap_t                   userId_map = NULL;                                                  \
    IndexTerm                 * term       = NULL;                                                  \
    uint64_t                    term_sign  = 0;                                                     \
    uint64_t                    new_sign   = 0;                                                     \
                                                                                                    \
    const CFG_PS_FIELD  * ps_field  = task->ps_filed;                                               \
    const char          * idx_field = task->idx_field;                                              \
    ProfileManager      * pfl_mgr   = ProfileManager::getInstance();                                \
    ProfileDocAccessor  * pfl_acr   = pfl_mgr->getDocAccessor();                                    \
    const ProfileField  * pfl_field = pfl_acr->getProfileField( ps_field->field_name );             \
    const ProfileField  * uid_field = pfl_acr->getProfileField( "user_id" );                        \
    const IndexTermInfo * term_info = NULL;                                                         \
    MemPool             * pMemPool  = new MemPool();                                                \
    char                * buf       = (char *)calloc( g_hf_config.cutoff_doc_num * 4                \
                                                                 , sizeof(DocListUnit) );           \
    Vector< PAIR_TYPE >   vect( task->max_doc_num ) ;                                               \
                                                                                                    \
    fprintf(stderr, "index field: %s, ps field: %s \n", idx_field, ps_field->field_name );          \
                                                                                                    \
    uint64_t idx_field_sign = idx_sign64( idx_field, strlen(idx_field) );                           \
                                                                                                    \
    if ( NULL == buf )      goto failed;                                                            \
    if ( NULL == pMemPool ) goto failed;                                                            \
                                                                                                    \
    for ( uint32_t i = 0; i < task->sign_vect->size(); i++ )                                        \
    {                                                                                               \
        term_sign = (*(task->sign_vect))[i];                                                        \
                                                                                                    \
        term = reader->getTerm( pMemPool, idx_field_sign, term_sign );                              \
        if ( NULL == term )      continue;                                                          \
                                                                                                    \
        /** 判断是否超长 链条 */                                                                            \
        term_info = term->getTermInfo();                                                            \
        if ( term_info->docNum <= g_hf_config.min_doc_num ) continue;                               \
                                                                                                    \
        new_sign = HFterm_sign64( ps_field->field_name, ps_field->sort_type, term_sign );           \
                                                                                                    \
        /** 已经添加过 截断索引了 */                                                                         \
        IndexTerm * tmp_term = reader->getTerm( pMemPool, idx_field_sign, new_sign );               \
        if ( tmp_term->getDocNum() > 0  )                                                           \
        {                                                                                           \
            fprintf(stderr, "fieldName: %s, high freq index exist!\n", idx_field);                  \
            break;                                                                                  \
        }                                                                                           \
                                                                                                    \
        uint32_t   docId      = INVALID_DOCID;                                                      \
        uint8_t  * occ_arr    = NULL;                                                               \
        int32_t    occ_num    = 0;                                                                  \
        VAl_TYPE   value      = 0;                    /** 从profile中读取的值 */                          \
        int64_t    userId_num = 0;                                                                  \
                                                                                                    \
        if ( NULL == ( userId_map = userId_map_renew( userId_map ) ) )                              \
            return -1;                                                                              \
                                                                                                    \
        if ( 0 == term_info->maxOccNum )               /** 没有occ  */                                \
        {                                                                                           \
            if ( pfl_field->multiValNum == 1 )         /** 单值 profile */                            \
            {                                                                                       \
                while ( INVALID_DOCID != ( docId = term->next() ) )                                 \
                {                                                                                   \
                    value = pfl_acr->S_FUNC( docId , pfl_field );                                   \
                                                                                                    \
                    if ( false == vect.pushBack( PAIR_TYPE( docId, value, 0 ) ) )                   \
                       break;                                                                       \
                }                                                                                   \
            }                                                                                       \
            else                                      /** 多值, 多值 取第一个     */                           \
            {                                                                                       \
                while ( INVALID_DOCID != ( docId = term->next() ) )                                 \
                {                                                                                   \
                    if ( pfl_acr->getValueNum( docId, pfl_field ) == 0 ) continue;                  \
                                                                                                    \
                    value = pfl_acr->M_FUNC( docId, pfl_field, 0 );                                 \
                                                                                                    \
                    if ( false == vect.pushBack( PAIR_TYPE( docId, value, 0 ) ) )                   \
                       break;                                                                       \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            uint32_t  sort_num = 0;                                                                 \
            sort_num = sortPair_vect_sort( vect, ps_field, 0, g_hf_config.cutoff_doc_num );         \
                                                                                                    \
            uint32_t * docid_arr = (uint32_t *) buf;                                                \
            uint32_t   docid_num = 0;                                                               \
            uint32_t   i         = 0;                                                               \
            int32_t    user_id   = 0;                                                               \
                                                                                                    \
            for (; i < g_hf_config.cutoff_doc_num; i ++)                                            \
            {                                                                                       \
                docid_arr[ i ] = vect[i].docId;                                                     \
                docid_num++;                                                                        \
                                                                                                    \
                user_id = pfl_acr->getInt32( vect[i].docId, uid_field );                            \
                                                                                                    \
                if ( NULL == hashmap_find( userId_map, (void *)user_id) )                           \
                {                                                                                   \
                    hashmap_insert( userId_map, (void *)user_id, (void *)1);                        \
                    userId_num++;                                                                   \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            /** 用户数不够 */                                                                            \
            if ( userId_num < g_hf_config.min_userId_num )                                          \
            {                                                                                       \
                if ( sort_num < g_hf_config.cutoff_doc_num * 3  )                                   \
                {                                                                                   \
                    sortPair_vect_sort( vect, ps_field, sort_num                                    \
                                            , g_hf_config.cutoff_doc_num * 3 - sort_num );          \
                }                                                                                   \
                                                                                                    \
                for (; i < g_hf_config.cutoff_doc_num * 3; i ++)                                    \
                {                                                                                   \
                    if ( i >= vect.size() ) break;                                                  \
                                                                                                    \
                    docid_arr[ i ] = vect[i].docId;                                                 \
                    docid_num++;                                                                    \
                                                                                                    \
                    user_id = pfl_acr->getInt32(vect[i].docId, uid_field);                          \
                                                                                                    \
                    if ( NULL == hashmap_find( userId_map, (void *)user_id) )                       \
                    {                                                                               \
                        hashmap_insert( userId_map, (void *)user_id, (void *)1);                    \
                        userId_num++;                                                               \
                    }                                                                               \
                                                                                                    \
                    if ( userId_num > g_hf_config.min_userId_num ) break;                           \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            shell_sort_int32( docid_arr, docid_num );                                               \
                                                                                                    \
            pthread_spin_lock( task->spin_lock );                                                   \
            add_hf_term( idx_field, ps_field, term_sign, new_sign, docid_arr, docid_num, term_info->docNum ); \
            pthread_spin_unlock( task->spin_lock );                                                 \
        }                                                                                           \
        else                                                                                        \
        {                                             /** 有occ的索引 需要特殊处理一下  */                      \
            if ( pfl_field->multiValNum == 1 )                                                      \
            {                                                                                       \
                while ( INVALID_DOCID != ( docId = term->next() ) )                                 \
                {                                                                                   \
                    value   = pfl_acr->S_FUNC( docId , pfl_field );                                 \
                    occ_arr = term->getOcc( occ_num );                                              \
                                                                                                    \
                    for ( int k = 0 ; k < occ_num; k++)                                             \
                    {                                                                               \
                        vect.pushBack( PAIR_TYPE( docId, value, occ_arr[k] ) );                     \
                    }                                                                               \
                }                                                                                   \
            }                                                                                       \
            else                                                                                    \
            {                                                                                       \
                while ( INVALID_DOCID != ( docId = term->next() ) )                                 \
                {                                                                                   \
                    if ( pfl_acr->getValueNum( docId, pfl_field ) == 0 ) continue;                  \
                                                                                                    \
                    value  = pfl_acr->M_FUNC( docId, pfl_field, 0 );                                \
                    occ_arr = term->getOcc( occ_num );                                              \
                                                                                                    \
                    for ( int k = 0 ; k < occ_num; k++)                                             \
                    {                                                                               \
                        vect.pushBack( PAIR_TYPE( docId, value, occ_arr[k] ) );                     \
                    }                                                                               \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            uint32_t  sort_num = 0;                                                                 \
            sort_num = sortPair_vect_sort( vect, ps_field, 0, g_hf_config.cutoff_doc_num );         \
                                                                                                    \
            DocListUnit * docid_arr = (DocListUnit *) buf;                                          \
            uint32_t      docid_num = 0;                                                            \
            uint32_t      i         = 0;                                                            \
                          docId     = INVALID_DOCID;                                                \
                                                                                                    \
            for (; i < g_hf_config.cutoff_doc_num; i ++)                                            \
            {                                                                                       \
                docid_arr[ i ].doc_id = vect[i].docId;                                              \
                docid_arr[ i ].occ    = vect[i].occ;                                                \
                                                                                                    \
                if ( docId != docid_arr[ i ].doc_id )                                               \
                {                                                                                   \
                    docid_num++;                                                                    \
                    docId = docid_arr[ i ].doc_id;                                                  \
                                                                                                    \
                    if ( docid_num >= g_hf_config.cutoff_doc_num * 3 ) break;                       \
                }                                                                                   \
                                                                                                    \
                int32_t user_id = pfl_acr->getInt32( vect[i].docId, uid_field );                    \
                                                                                                    \
                if ( NULL == hashmap_find( userId_map, (void *)user_id) )                           \
                {                                                                                   \
                    hashmap_insert( userId_map, (void *)user_id, (void *)1);                        \
                    userId_num++;                                                                   \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            if ( userId_num < g_hf_config.min_userId_num )                                          \
            {                                                                                       \
                if ( sort_num < g_hf_config.cutoff_doc_num * 3  )                                   \
                {                                                                                   \
                    sortPair_vect_sort( vect, ps_field, sort_num                                    \
                                            , g_hf_config.cutoff_doc_num * 3 - sort_num );          \
                }                                                                                   \
                                                                                                    \
                for (; i < g_hf_config.cutoff_doc_num * 3; i ++)                                    \
                {                                                                                   \
                    if ( i >= vect.size() ) break;                                                  \
                                                                                                    \
                    docid_arr[ i ].doc_id = vect[i].docId;                                          \
                    docid_arr[ i ].occ    = vect[i].occ;                                            \
                    docid_num++;                                                                    \
                                                                                                    \
                    int32_t user_id = pfl_acr->getInt32(vect[i].docId, uid_field);                  \
                                                                                                    \
                    if ( NULL == hashmap_find( userId_map, (void *)user_id) )                       \
                    {                                                                               \
                        hashmap_insert( userId_map, (void *)user_id, (void *)1);                    \
                        userId_num++;                                                               \
                    }                                                                               \
                                                                                                    \
                    if ( userId_num > g_hf_config.min_userId_num ) break;                           \
                }                                                                                   \
            }                                                                                       \
                                                                                                    \
            shell_sort_idx2_nzip( docid_arr, docid_num );                                           \
                                                                                                    \
            pthread_spin_lock( task->spin_lock );                                                   \
            add_hf_term_occ( idx_field, ps_field, term_sign, new_sign, docid_arr, docid_num, term_info->docNum);   \
            pthread_spin_unlock( task->spin_lock );                                                 \
        }                                                                                           \
                                                                                                    \
        vect.clear();                                                                               \
        fprintf(stderr, "userId num: %lu\n", userId_num);                                           \
    }                                                                                               \
                                                                                                    \
    if ( NULL != buf )      SAFE_FREE( buf );                                                       \
    if ( NULL != pMemPool ) SAFE_DELETE( pMemPool );                                                \
                                                                                                    \
    return 0;                                                                                       \
                                                                                                    \
failed:                                                                                             \
    if ( NULL != buf )      SAFE_FREE( buf );                                                       \
    if ( NULL != pMemPool ) SAFE_DELETE( pMemPool );                                                \
                                                                                                    \
    return -1;                                                                                      \




/**
 * 通过一个指定的long类型的 属性字段 对 倒排字段内的 高频term进行排序和截断
 *
 * @return  -1:error
 */
int cutOff_by_int32_field ( HF_TASK * task )
{
    CUTOFF_BY_FIELD( SortPairInt32, int32_t, getInt32, getRepeatedInt32 );
}

int cutOff_by_int64_field ( HF_TASK * task )
{
    CUTOFF_BY_FIELD( SortPairInt64, int64_t, getInt, getRepeatedInt );
}

int cutOff_by_uint64_field( HF_TASK * task )
{
    CUTOFF_BY_FIELD( SortPairUInt64, uint64_t, getUInt, getRepeatedUInt );
}

int cutOff_by_float_field ( HF_TASK * task )
{
    CUTOFF_BY_FIELD( SortPairFloat, float, getFloat, getRepeatedFloat );
}

int cutOff_by_double_field( HF_TASK * task )
{
    CUTOFF_BY_FIELD( SortPairDouble, double, getDouble, getRepeatedDouble );
}



/**
 * 通过 ends字段的值， 进行截断控制，因为逻辑特殊， 所以独立出来
 *
 * 取当天的  6点 （切库时间点） -> 23:30 (增量停止时间点) 之间的  所有数据，
 * 不足5万的，顺序补充至 5万
 *
 * @param task
 *
 * @return -1 error
 */
int cutOff_by_ends( HF_TASK * task )
{
    IndexReader               * reader     = IndexReader::getInstance();
    IndexTerm                 * term       = NULL;
    hashmap_t                   userId_map = NULL;
    uint64_t                    term_sign  = 0;
    uint64_t                    new_sign   = 0;
    struct timeval              cur_time_stu;              // 当前时间
    time_t                      end_time   = 0;            // 截断的结束时间

    const CFG_PS_FIELD  * ps_field  = task->ps_filed;
    const char          * idx_field = task->idx_field;
    ProfileManager      * pfl_mgr   = ProfileManager::getInstance();
    ProfileDocAccessor  * pfl_acr   = pfl_mgr->getDocAccessor();
    const ProfileField  * pfl_field = pfl_acr->getProfileField( ps_field->field_name );
    const ProfileField  * uid_field = pfl_acr->getProfileField( "user_id" );
    const IndexTermInfo * term_info = NULL;
    MemPool             * pMemPool  = new MemPool();
    char                * buf       = (char *)calloc( task->max_doc_num, sizeof(DocListUnit) );

    Vector< SortPairInt64 > vect( task->max_doc_num );

    fprintf(stderr, "index field: %s, ps field: %s \n", idx_field, ps_field->field_name );

    uint64_t idx_field_sign = idx_sign64( idx_field, strlen(idx_field) );

    if ( NULL == buf )      goto failed;
    if ( NULL == pMemPool ) goto failed;

    gettimeofday(&cur_time_stu, NULL);
    end_time = ( (cur_time_stu.tv_sec / 86400) * 86400 ) + 23 * 3600 + 1800 + 8 * 3600 - 9 * 86400;

    for ( uint32_t i = 0; i < task->sign_vect->size(); i++ )
    {
        term_sign = (*(task->sign_vect))[i];

        term = reader->getTerm( pMemPool, idx_field_sign, term_sign );
        if ( NULL == term )  continue;

        /** 判断是否超长 链条 */
        term_info = term->getTermInfo();
        if ( term_info->docNum <= g_hf_config.min_doc_num ) continue;

        new_sign = HFterm_sign64( ps_field->field_name, ps_field->sort_type, term_sign );

        /** 已经添加过 截断索引了 */
        IndexTerm * tmp_term = reader->getTerm( pMemPool, idx_field_sign, new_sign );
        if ( tmp_term->getDocNum() > 0  )
        {
            fprintf(stderr, "fieldName: %s, high freq index exist!\n", idx_field);
            break;
        }

        uint32_t   docId      = INVALID_DOCID;
        uint8_t  * occ_arr    = NULL;
        int32_t    occ_num    = 0;
        int64_t    value      = 0;                    /** 从profile中读取的值 */
        int64_t    userId_num = 0;

        if ( NULL == ( userId_map = userId_map_renew( userId_map ) ) )
            return -1;

        if ( 0 == term_info->maxOccNum )              /** 没有occ  */
        {
            while ( INVALID_DOCID != ( docId = term->next() ) )
            {
                if ( pfl_acr->getValueNum( docId, pfl_field ) == 0 ) continue;

                value = pfl_acr->getRepeatedInt( docId, pfl_field, 0 );

                if ( false == vect.pushBack( SortPairInt64( docId, value, 0 ) ) )
                   break;
            }

            uint64_t cut_num = g_hf_config.cutoff_doc_num;
            if ( (vect.size() / 10 ) > cut_num ) cut_num = (vect.size() / 10 );

            sortPair_vect_sort( vect, ps_field, 0 , cut_num );

            uint32_t * docid_arr = (uint32_t *) buf;
            uint32_t   docid_num = 0;

            for (uint32_t i = 0; i < vect.size(); i ++)
            {
                if ( vect[i].value >= end_time )
                {
                    if (( docid_num >= g_hf_config.cutoff_doc_num )
                            && ( userId_num >= g_hf_config.min_userId_num ))
                        break;
                }

                docid_arr[ docid_num ] = vect[i].docId;
                docid_num++;

                int32_t user_id = pfl_acr->getInt32( vect[i].docId, uid_field );

                if ( NULL == hashmap_find( userId_map, (void *)user_id) )
                {
                    hashmap_insert( userId_map, (void *)user_id, (void *)1);
                    userId_num++;
                }
            }

            if ( docid_num < g_hf_config.cutoff_doc_num )
            {
                docid_num = 0;                        // 不足5万，彻底重头开始截取

                for ( uint32_t i = 0; i < vect.size(); i++ )
                {
                    if ( docid_num >= g_hf_config.cutoff_doc_num ) break;

                    docid_arr[ docid_num ] = vect[i].docId;
                    docid_num++;
                }
            }

            shell_sort_int32( docid_arr, docid_num );

            pthread_spin_lock( task->spin_lock );
            add_hf_term( idx_field, ps_field, term_sign, new_sign, docid_arr, docid_num, term_info->docNum );
            pthread_spin_unlock( task->spin_lock );
        }
        else
        {                                             /** 有occ的索引 需要特殊处理一下  */
            while ( INVALID_DOCID != ( docId = term->next() ) )
            {
                if ( pfl_acr->getValueNum( docId, pfl_field ) == 0 ) continue;

                value   = pfl_acr->getRepeatedInt( docId, pfl_field, 0 );
                occ_arr = term->getOcc( occ_num );

                for ( int k = 0 ; k < occ_num; k++)
                {
                    vect.pushBack( SortPairInt64( docId, value, occ_arr[k] ) );
                }
            }

            uint64_t cut_num = g_hf_config.cutoff_doc_num;
            if ( (vect.size() / 10 ) > cut_num ) cut_num = (vect.size() / 10 );

            sortPair_vect_sort( vect, ps_field, 0 , cut_num );

            DocListUnit * docid_arr = (DocListUnit *) buf;
            uint32_t      docid_num = 0;
                          docId     = INVALID_DOCID;

            for (uint32_t i = 0; i < vect.size(); i ++)
            {
                if ( vect[i].value >= end_time )
                {
                    if (( docid_num >= g_hf_config.cutoff_doc_num )
                            && ( userId_num >= g_hf_config.min_userId_num ))
                        break;
                }

                docid_arr[ docid_num ].doc_id = vect[i].docId;
                docid_arr[ docid_num ].occ    = vect[i].occ;
                docid_num++;

                int32_t user_id = pfl_acr->getInt32( vect[i].docId, uid_field );

                if ( NULL == hashmap_find( userId_map, (void *)user_id) )
                {
                    hashmap_insert( userId_map, (void *)user_id, (void *)1);
                    userId_num++;
                }
            }

            if ( docid_num < g_hf_config.cutoff_doc_num )
            {
                docid_num = 0;                        // 不足5万，彻底重头开始截取

                for ( uint32_t i = 0; i < vect.size(); i++ )
                {
                    if ( docid_num >= g_hf_config.cutoff_doc_num ) break;

                    docid_arr[ docid_num ].doc_id = vect[i].docId;
                    docid_arr[ docid_num ].occ    = vect[i].occ;
                    docid_num++;
                }
            }

            shell_sort_idx2_nzip( docid_arr, docid_num );

            pthread_spin_lock( task->spin_lock );
            add_hf_term_occ( idx_field, ps_field, term_sign, new_sign, docid_arr, docid_num, term_info->docNum);
            pthread_spin_unlock( task->spin_lock );
        }

        fprintf(stderr, "userId num: %lu\n", userId_num);
        vect.clear();
    }

    if ( NULL != buf )      SAFE_FREE( buf );
    if ( NULL != pMemPool ) SAFE_DELETE( pMemPool );

    return 0;

failed:
    if ( NULL != buf )      SAFE_FREE( buf );
    if ( NULL != pMemPool ) SAFE_DELETE( pMemPool );

    return -1;
}




/**
 * 处理单一倒排字段的  高频词截断工作
 *
 * @param fieldName          倒排字段名
 */
void * pthread_worker(void * param)
{
    while ( 1 )
    {
        HF_TASK * cur_task = NULL;

        pthread_spin_lock( &g_thread_lock );

        if ( g_hf_task_cur >= g_hf_task_count )            // 一直执行到没有任务为止
        {
            pthread_spin_unlock( &g_thread_lock );
            break;
        }

        cur_task = &( g_hf_task_arr[ g_hf_task_cur ] );    // 取得一个任务
        g_hf_task_cur++;

        pthread_spin_unlock( &g_thread_lock );

        // 开始处理任务
        ProfileManager     * pfl_mgr   = ProfileManager::getInstance();
        ProfileDocAccessor * pfl_acr   = pfl_mgr->getDocAccessor();
        const ProfileField * pfl_field = NULL;

        // 特殊处理ends字段  , 取值逻辑特别
        // if ( 0 == strcmp( "ends", cur_task->ps_filed->field_name ) )
        // {
        //    if ( cutOff_by_ends( cur_task ) < 0 ) return NULL;
        //    continue;
        // }

        pfl_field = pfl_acr->getProfileField( cur_task->ps_filed->field_name );

        switch ( pfl_acr->getFieldDataType( pfl_field ) )
        {
            case DT_INT32:
                if ( cutOff_by_int32_field( cur_task ) < 0 )    return NULL;
                break;

            case DT_INT8:  case DT_INT16: case DT_INT64:
                if ( cutOff_by_int64_field( cur_task ) < 0 )    return NULL;
                break;

            case DT_UINT8:  case DT_UINT16:
            case DT_UINT32: case DT_UINT64:
                if ( cutOff_by_uint64_field( cur_task ) < 0 ) return NULL;
                break;

            case DT_FLOAT:
                if ( cutOff_by_float_field( cur_task ) < 0 )  return NULL;
                break;

            case DT_DOUBLE:
                if ( cutOff_by_double_field( cur_task ) < 0 ) return NULL;
                break;

            default:
                fprintf(stderr, "the %s's type can't sort\n", cur_task->ps_filed->field_name);
                return NULL;
        }
    }

    return 0;
}




/** 主程序入口  */
int main(int argc, char *argv[])
{
    // alog::Configurator::configureRootLogger();
    // alog::Logger::MAX_MESSAGE_LENGTH = 10240;

    int         stime              = time(NULL);
    pthread_t   thread_arr[ 1024 ] = {0};                  // 线程句柄

    parse_args(argc, argv, &g_cmd_para);                   // 分析参数

    //if ( -1 == init_alog() )                               // 初始化alog
    //    return EXIT_FAILURE;

    if ( -1 == init_index_lib( g_cmd_para.cfg_file ) )     // 初始化索引
        return EXIT_FAILURE;

    if ( -1 == parse_hf_cfg( g_cmd_para.cfg_file ) )       // 解析高频词配置文件
        return EXIT_FAILURE;

    IndexBuilder  * builder = IndexBuilder::getInstance();

    // 生成所有任务
    memset( g_hf_task_arr, 0, sizeof(g_hf_task_arr) );     // 初始化任务
    g_hf_task_cur   = 0;
    g_hf_task_count = 0;

    pthread_spin_init( &g_thread_lock, PTHREAD_PROCESS_PRIVATE );
    for ( uint32_t i = 0; i < sizeof(g_idx_lock_arr)/sizeof(pthread_spinlock_t); i++ )
    {
        pthread_spin_init( &(g_idx_lock_arr[ i ]), PTHREAD_PROCESS_PRIVATE );
    }

    const char      * field_name  = NULL;                  // 倒排字段名
    int32_t           travel_pos  = -1;
    int32_t           max_occ_num = 0;

    for ( int i = 0; i < MAX_INDEX_FIELD_NUM; i++ )        // 循环所有的 倒排字段
    {
        field_name = builder->getNextField( travel_pos, max_occ_num );

        if ( NULL == field_name ) break;
        if ( 0 == strcmp( field_name, "user_id" )) continue;

        g_max_doc_num_arr[ i ] = get_idx_all_term( field_name, g_idx_sign_vect_arr[ i ] );
    }

    // 循环所有的 排序 字段
    for ( uint32_t j = 0 ; j < g_hf_config.ps_fields_num ; j++ )
    {
        field_name  = NULL;                  // 倒排字段名
        travel_pos  = -1;
        max_occ_num = 0;

        for ( int i = 0; i < MAX_INDEX_FIELD_NUM; i++ )        // 循环所有的 倒排字段
        {
            field_name = builder->getNextField( travel_pos, max_occ_num );

            if ( NULL == field_name ) break;
            if ( 0 == strcmp( field_name, "user_id" )) continue;

            g_hf_task_arr[ g_hf_task_count ].idx_field   = (char *)field_name;
            g_hf_task_arr[ g_hf_task_count ].ps_filed    = &(g_hf_config.ps_fields[ j ]);

            // 同一个倒排字段的 几个任务公用 一个 锁
            g_hf_task_arr[ g_hf_task_count ].spin_lock   = &(g_idx_lock_arr[ i ]);
            g_hf_task_arr[ g_hf_task_count ].sign_vect   = &(g_idx_sign_vect_arr[ i ]);
            g_hf_task_arr[ g_hf_task_count ].max_doc_num = g_max_doc_num_arr[ i ];

            g_hf_task_count++;
        }
    }

    for ( int i = 0; i < g_cmd_para.thread_num; i++ )      // 启动多个线程
    {
        pthread_create( &thread_arr[ i ], NULL, pthread_worker, NULL );
    }

    for ( int i = 0; i < g_cmd_para.thread_num; i++ )      // 等待工作线程终止
    {
        pthread_join( thread_arr[ i ], NULL );
    }

    builder->dump();                                       // 输出索引
    builder->close();
    index_lib::destroy();

    pthread_spin_destroy( &g_thread_lock );
    for ( uint32_t i = 0; i < sizeof(g_idx_lock_arr)/sizeof(pthread_spinlock_t) ; i++ )
    {
        pthread_spin_destroy( &(g_idx_lock_arr[ i ]) );
    }

    fprintf(stderr, "cost %ld seconds.\n", time(NULL) - stime);
    return EXIT_SUCCESS;
}



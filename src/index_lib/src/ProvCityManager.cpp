/******************************************************************
 * ProvCityManager.cpp
 *
 *      Author: yewang@taobao.com clm971910@gmail.com
 *      Create: 2011-1-13
 *
 *      Desc  : 提供从 区划编码表 文件 的加载功能
 *              提供从 地址 到 编码的查询 接口
 *              提供编码的比对接口
 ******************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>


#include "util/idx_sign.h"
#include "util/strfunc.h"
#include "util/Log.h"
#include "index_lib/ProvCityManager.h"





namespace index_lib
{

const char    PROVCITY_SEPRATOR      = '\t';          // 区划文件 的内部分隔符
const int     PROVCITY_COL_NUM       = 4;             // 区划文件 的列数
const int     PROVCITY_AREA_MAX_ID   = 31;            // 最大区域id
const int     PROVCITY_PROV_MAX_ID   = 63;            // 最大的省di
const int     PROVCITY_CITY_MAX_ID   = 31;            // 最大的市di

// modify by pujian, 过滤后缀
const char * PROVCITY_SHIELD_SUFFIX[4] = {
    "省",                // 省后缀
    "市",                // 市后缀
    "特别行政区",        // 特别行政区(香港、澳门)
    "自治"               // 自治州/区/县
};

ProvCityManager * ProvCityManager::instance = NULL;   // 类的惟一实例






/**
 * 构造及析构函数
 * @return
 */
ProvCityManager::ProvCityManager() : _dict(NULL) {}
ProvCityManager::~ProvCityManager(){ }



/**
 * 获取单实例对象指针, 实现类的单例
 *
 * @return  NULL: error
 */
ProvCityManager * ProvCityManager::getInstance()
{
    if ( NULL == instance )
    {
        instance = new ProvCityManager();
    }

    return instance;
}


/**
 * 释放类的单例
 */
void ProvCityManager::freeInstance()
{
    if ( instance == NULL ) return;

    instance->destroy();

    delete instance;
    instance = NULL;
}




/**
 * 载入区划编码编码数据文件。
 *
 * @param fp        文件句柄
 *
 * @return false: 载入失败;  true: 载入化成功
 */
bool  ProvCityManager::trueLoad( FILE * fp )
{
    return_val_if_fail( NULL != fp, false);

    ProvCityID   pcId;
    char         line[LINE_MAX];
    char       * colArr[65]    = {0};
    int          colNum        = 0;
    char       * begin         = NULL;
    char       * end           = NULL;
    char       * name          = NULL;
    int          areaId        = 0;
    int          provId        = 0;
    int          cityId        = 0;

    pcId.pc_id = 0;

    while ( fgets(line, LINE_MAX, fp) != NULL )       // 一行一个
    {
        if ( strlen(line) <= 1 )  continue;           // 因为有个换行符号

        line[strlen(line) - 1] = '\0';                // 替掉最后的换行符号

        memset( colArr, 0, sizeof(colArr) );

        pcId.pc_id = 0;

        begin = line;
        end   = line + strlen(line);

        // 循环 按照  tab 分出所有的col
        colNum = str_split_ex( line, PROVCITY_SEPRATOR, colArr, sizeof(colArr) / sizeof(char *) );

        if ( PROVCITY_COL_NUM != colNum )             // 判断列数
        {
            TERR( "provcity format error! %s", line );
            return false;
        }

        name = colArr[0];                             // 判断区划名
        if ( strlen(name) <= 0 )
        {
            TERR("provcity name is null!");
            return false;
        }

        areaId = atoi( colArr[ 1 ] );
        provId = atoi( colArr[ 2 ] );
        cityId = atoi( colArr[ 3 ] );

        if ( areaId < 0 || areaId > PROVCITY_AREA_MAX_ID )
        {
            TERR( "provcity area id out of range! %s", line);
            return false;
        }

        if ( provId < 0 || provId > PROVCITY_PROV_MAX_ID )
        {
            TERR( "provcity prov id out of range! %s", line);
            return false;
        }

        if ( cityId < 0 || cityId > PROVCITY_CITY_MAX_ID )
        {
            TERR( "provcity city id out of range! %s", line);
            return false;
        }

        // 现在确认 整个是OK的
        pcId.pc_stu.area_id = areaId;
        pcId.pc_stu.prov_id = provId;
        pcId.pc_stu.city_id = cityId;

        uint64_t nameSign = idx_sign64( name, strlen(name) );

        if ( hashmap_insert( _dict, (void *) nameSign, (void *)pcId.pc_id) < 0 )
        {
            TERR("provcity dict add failed! %s", line);
            return false;
        }
    }

    return true;
}




/**
 * 载入区划编码编码数据文件。
 *
 * @param  filePath 数据存放的完整路径
 *
 * @return  false: 载入失败;  true: 载入化成功
 */
bool  ProvCityManager::load(const char * filePath)
{
    return_val_if_fail( filePath != NULL, false );

    FILE * fp = NULL;

    if ( NULL != _dict )
    {
        TLOG("provcity has been init");
        return true;
    }

    // 创建词典
    _dict = hashmap_create( 4096, HM_FLAG_POOL, hashmap_hashInt, hashmap_cmpInt );

    if ( NULL == _dict )
    {
        TERR("create provcity dict failed");
        goto failed;
    }

    fp = fopen( filePath, "r" );                      // 打开文件
    if ( NULL == fp )
    {
        TERR("open file failed! %s ", filePath);
        goto failed;
    }

    if ( false == trueLoad( fp ) )  goto failed;      // 把文件内容加载到字典里面去

    fclose(fp);
    return true;

failed:
    if ( _dict != NULL )
    {
        hashmap_destroy(_dict, NULL, NULL);
        _dict = NULL;
    }

    if ( NULL != fp )  fclose(fp);

    return false;
}







/**
 * 根据  单个区划名称 查询 对应的编码
 *
 * @param    name      输入的区划名字   "杭州"
 * @param    pcId      输出的编码
 *
 * @return  -1: 不存在这个区划；   0: 转换成功
 */
int  ProvCityManager::seek(const char * name, ProvCityID * pcId)
{
    return_val_if_fail( name != NULL, -1 );
    return_val_if_fail( pcId != NULL, -1 );

    char   buf[LINE_MAX] = {0};
    char * suffixPos     = NULL;
    int    nameLen       = strlen(name);

    if ( (nameLen <= 0) || (nameLen >= LINE_MAX) )
    {
        TLOG("provcity name is too short or too long, %s", name);
        return -1;
    }

    uint64_t nameSign = idx_sign64( name, nameLen );

    void * node = hashmap_find( _dict, (void *) nameSign );

    if ( NULL == node )                                    // 没找到 这时也许是 传入的字符串 后面 带了  省 市
    {
        snprintf(buf, LINE_MAX, "%s", name);

        // 依次查看每个过滤后缀
        for( int pos = 0; pos < 4; pos++ ) {
            suffixPos = strstr(buf, PROVCITY_SHIELD_SUFFIX[pos]);
            if (suffixPos != NULL) {
                break;
            }
        }

        if (NULL == suffixPos) {
            TLOG("provcity name can't found, %s", name);
            return -1;
        }

        suffixPos[0] = '\0';                               // 切断后面的 省 市

        nameSign = idx_sign64( buf, strlen(buf) );

        node = hashmap_find( _dict, (void *) nameSign );

        if ( NULL == node )                                // 这回真的是没有了
        {
            TLOG("provcity name can't found, %s", name);
            return -1;
        }
    }

    pcId->pc_id = ( uint64_t ) node;

    return 0;
}






/**
 * 根据的  区划名称 查询 对应的编码, 可能是组合式的
 *
 * @param name        输入的区划名字   "杭州"  or "浙江 杭州"
 * @param delim       分隔符
 * @param pcId        输出的编码
 *
 * @return  -1: 不存在这个区划；   0: 转换成功
 */
int  ProvCityManager::seek(const char * name, char delim, ProvCityID * pcId)
{
    return_val_if_fail( name != NULL, -1 );
    return_val_if_fail( pcId != NULL, -1 );

    char    buf[LINE_MAX];
    char  * colArr[65]  = {0};
    int     colNum      = 0;

    snprintf(buf, LINE_MAX, "%s", name);                   // copy 下来， 因为  会可能有修改

    // 循环 按照  tab 分出所有的 区划名
    colNum = str_split_ex( buf, delim, colArr, sizeof(colArr) / sizeof(char *) );

    if ( colNum <= 0 )
    {
        TLOG("what you want ?? : %s", name);
        return -1;
    }

    if ( 1 == colNum )                                     // 输入的是单个的
    {
        return seek( colArr[ 0 ], pcId );
    }

    if ( colNum > 3 || colNum <= 0 )                       // 最多三级
    {
        TLOG("what you want ?? : %s", name);
        return -1;
    }

    // 先取出 最低层的编码
    if ( -1 == seek( colArr[ colNum - 1 ], pcId ) )  return -1;

    ProvCityID  tmpId;

    // 下面要判断  一下， 取出的最低层级的id 和  前面的2层级  是否有从属关系
    for ( int i = 0; i < colNum - 1; ++i)
    {
        tmpId.pc_id = 0;

        if ( -1 == seek( colArr[ i ], &tmpId) )   return -1;

        // 判断从属关系
        if ( 0 == pcId->pc_stu.city_id && 0 == pcId->pc_stu.prov_id )
        {
            TLOG( "it's already top code, %s", colArr[ colNum - 1 ] );
            return -1;                                     // 最低层 已经是  一级了
        }

        if ( 0 == pcId->pc_stu.city_id && 0 != pcId->pc_stu.prov_id )
        {
            // 这时  最低层 是省级单位。
            if ( tmpId.pc_stu.prov_id != 0 || tmpId.pc_stu.city_id != 0 )
                goto failed;

            if ( tmpId.pc_stu.area_id != pcId->pc_stu.area_id )
                goto failed;
        }

        if ( 0 != pcId->pc_stu.city_id )                   // 最低层 是市级。 要和 上一层比较 area_id 或者  prov_id
        {
            if ( tmpId.pc_stu.prov_id == 0 && tmpId.pc_stu.city_id == 0 )
            {
                if ( pcId->pc_stu.area_id != tmpId.pc_stu.area_id )
                    goto failed;
            }

            if ( tmpId.pc_stu.prov_id != 0 && tmpId.pc_stu.city_id == 0 )
            {
                if ( pcId->pc_stu.prov_id != tmpId.pc_stu.prov_id )
                    goto failed;
            }
        }
    }

    // 没有在前面报错， 说明。 都OK，   编码 用最低层级取得的编码
    return 0;

failed:
    TLOG("relation not correct %s", name);
    return -1;
}


/**
 * 销毁运行中分配的资源
 *
 * @return
 */
bool ProvCityManager::destroy()
{
    if ( _dict != NULL )
    {
        hashmap_destroy(_dict, NULL, NULL);
        _dict = NULL;
    }

    return true;
}


}

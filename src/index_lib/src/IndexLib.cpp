/******************************************************************
 * IndexLib.cpp
 *
 *  Created on: 2011-4-7
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 提供 index_lib 这个模块的全局的函数功能
 *
 ******************************************************************/

#include "mxml.h"
#include "util/Log.h"
#include "util/idx_sign.h"
#include "index_lib/IndexLib.h"
#include "index_lib/IndexReader.h"
#include "index_lib/IndexBuilder.h"
#include "index_lib/ProfileManager.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/IndexIncManager.h"
#include "index_lib/ProvCityManager.h"
#include "index_lib/IncManager.h"
#include "index_lib/IndexConfigParams.h"
#include <sys/resource.h>

namespace index_lib
{

/**
 * 初始化 profile 部分
 *
 * @param path     数据存放路径
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int initProfile(const char * path, bool sync = true, bool buildHF = false)
{
    ProfileManager * mgr = ProfileManager::getInstance();

    if ( NULL == mgr )
    {
        TERR("ProfileManager instance is null");
        return -1;
    }

    if ( (NULL == path) || strlen( path ) <= 0 )
    {
        TERR("profile node's path attribute is null");
        return -1;
    }

    if ( mgr->getProfileDocNum() > 0 )
    {
        TLOG("profile has been loaded!");
        return 0;
    }

    mgr->setProfilePath( path );
    mgr->setDataSync( sync );

    TLOG("begin to load profile! path:%s", path);

    if ( 0 != mgr->load(!buildHF) )
    {
        TERR("load profile failed! path:%s", path);
        return -1;
    }

    TLOG("load profile success!");

    return 0;
}




/**
 * 初始化 nid->docId的字典 和 deletemap
 *
 * @param path     数据存放路径
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int initIdDict(const char * path, bool sync = true)
{
    DocIdManager * mgr = DocIdManager::getInstance();

    if ( NULL == mgr )
    {
        TERR("DocIdManager instance is null");
        return -1;
    }

    if ( (NULL == path) || strlen( path ) <= 0 )
    {
        TERR("idDict node's path attribute is null");
        return -1;
    }

    TLOG("begin to load nid->docId dict and deleteMap! path:%s", path);

    mgr->setDataSync( sync );
    if ( false == mgr->load( path ) )
    {
        TERR("load nid->docId dict and deleteMap failed! path:%s", path);
        return -1;
    }

    TLOG("load nid->docId dict and deleteMap success!");

    return 0;
}



/**
 * 初始化 全量index
 *
 * @param path     数据存放路径
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int initFullIndex(const char * path)
{
    IndexReader * reader = IndexReader::getInstance();

    if ( NULL == reader )
    {
        TERR("IndexReader instance is null");
        return -1;
    }

    if ( (NULL == path) || strlen( path ) <= 0 )
    {
        TERR("index node's path attribute is null");
        return -1;
    }

    TLOG("begin to load full index! path:%s", path);

    if ( reader->open( path ) < 0)
    {
        TERR("load full index failed! path:%s", path);
        return -1;
    }

    TLOG("load full index success!");

    return 0;
}



/**
 * 初始化 全量index 的 builder， 重新打开， 为了修改
 *
 * @param path     数据存放路径
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int initIdxBuilder(const char * path)
{
    IndexBuilder * builder = IndexBuilder::getInstance();

    if ( NULL == builder )
    {
        TERR("IndexBuilder instance is null");
        return -1;
    }

    if ( (NULL == path) || strlen( path ) <= 0 )
    {
        TERR("index node's path attribute is null");
        return -1;
    }

    TLOG("begin to load full index! path:%s", path);

    if ( builder->reopen( path ) < 0)
    {
        TERR("load full index failed! path:%s", path);
        return -1;
    }

    TLOG("load full index success!");

    return 0;
}



/**
 * 初始化 增量 index
 *
 * @param path     数据存放路径
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int initIncIndex(const char * path, bool sync, bool et, int32_t inc_max_num )
{
    IndexIncManager * mgr = IndexIncManager::getInstance();

    if ( NULL == mgr )
    {
        TERR("IndexIncManager instance is null");
        return -1;
    }

    if ( (NULL == path) || strlen( path ) <= 0 )
    {
        TERR("index node's path attribute is null");
        return -1;
    }

    TLOG("begin to load inc index! path:%s", path);

    mgr->setDataSync( sync );
    mgr->setExportIdx( et );
    mgr->setMaxIncNum( inc_max_num );
    if ( mgr->open( path ) < 0 )
    {
        TERR("load inc index failed! path:%s", path);
        return -1;
    }

    TLOG("load inc index success!");

    return 0;
}





/**
 * 初始化  行政区划转换编码表
 *
 * @param path     数据存放路径  完整路径
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int initProvCity(const char * path)
{
    ProvCityManager * mgr = ProvCityManager::getInstance();

    if ( NULL == mgr )
    {
        TERR("ProvCityManager instance is null");
        return -1;
    }

    if ( (NULL == path) || strlen( path ) <= 0 )
    {
        TERR("provcity node's path attribute is null");
        return -1;
    }

    TLOG("begin to load provcity! path:%s", path);

    if ( false == mgr->load( path ) )
    {
        TERR("load inc index failed! path:%s", path);
        return -1;
    }

    TLOG("load provcity success!");

    return 0;
}





/**
 * 将IncManger中的，的字段处理策略初始化
 *
 * @param  node  在配置文件中的 xml 节点 update_add2Modify
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int initIncManager( mxml_node_t * node )
{
    if ( NULL == node )  return 0;

    mxml_node_t        * field  = NULL;
    IndexReader        * reader = IndexReader::getInstance();
    IncManager         * incMgr = IncManager::getInstance();
    ProfileDocAccessor * pflAcr = ProfileManager::getDocAccessor();

    field  = mxmlFindElement( node, node, "index_field", NULL, NULL, MXML_DESCEND_FIRST );

    while ( field != NULL )
    {
        const char * name = mxmlElementGetAttr( field, "name" );
        const char * proc = mxmlElementGetAttr( field, "proc" );


        if ( ( NULL == name ) || (0 == strlen(name)) )
        {
            TERR("update_add2Modify: must have field name");
            return -1;
        }

        if ( ( NULL == proc ) || (0 == strlen(proc)) )
        {
            TERR("update_add2Modify: must have proc: depend or ignore");
            return -1;
        }

        if ( false == reader->hasField( name ) )
        {
            TWARN("update_add2Modify: can't find field in index:%s, are you sure", name);
        }

        if ( 0 == strcmp( proc , "ignore" ) )
        {
            if ( false == incMgr->addIgnIdxField( name ) )
            {
                TERR("update_add2Modify: add ignore field to incManager failed:%s", name);
                return -1;
            }
        }
        else if ( 0 == strcmp( proc , "depend" ) )
        {
            const char * pflFieldName = mxmlElementGetAttr( field, "profile_field" );

            if ( ( NULL == pflFieldName ) || (0 == strlen(pflFieldName)) )
            {
                TERR("update_add2Modify: field:%s must have profile_field", name);
                return -1;
            }

            if ( NULL == pflAcr->getProfileField( pflFieldName ) )
            {
                TERR("update_add2Modify: can't find field in profile:%s", pflFieldName);
                return -1;
            }

            if ( false == incMgr->addIgnIdxField( name ) )
            {
                TERR("update_add2Modify: add ignore field to incManager failed:%s", name);
                return -1;
            }

            if ( false == incMgr->addDepPflField( pflFieldName ) )
            {
                TERR("update_add2Modify: add depend pfl field to incManager failed:%s", pflFieldName);
                return -1;
            }
        }
        else
        {
            TERR("update_add2Modify: field:%s proc must be depend or ignore", name);
            return -1;
        }

        field = mxmlFindElement( field, node, "index_field", NULL, NULL, MXML_NO_DESCEND );
    }

    return 0;
}




/**
 * 解析配置文件， 初始化 index_lib内部的各个模块
 *
 * @param  config  xml的配置节点。  内部的子节点包括了 索引的各个配置项
 *         样例:
 *         <indexLib>
 *            <profile  path="/dir" />
 *            <index    path="/dir" />
 *            <idDict   path="/dir" />
 *            <provcity path="/dir" />
 *         </indexLib>
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int init( mxml_node_t * config )
{
    mxml_node_t  * modulesNode  = NULL;
    mxml_node_t  * indexLibNode = NULL;               // index_lib的根节点
    mxml_node_t  * profileNode  = NULL;               // profile配置节点
    mxml_node_t  * indexNode    = NULL;               // index配置节点
    mxml_node_t  * idDictNode   = NULL;               // nid->docId 字典及deletemap节点
    mxml_node_t  * provcityNode = NULL;               // 行政区划表配置节点
    mxml_node_t  * add2modNode  = NULL;               // add转modify配置节点
    bool           syncFlag     = true;
    bool           exportFlag   = false;

    modulesNode  = mxmlFindElement(config, config, "modules", NULL, NULL, MXML_DESCEND);
    if ( modulesNode == NULL || modulesNode->parent != config )
    {
        TERR("can't find modules's node");
        return -1;
    }

    indexLibNode = mxmlFindElement(modulesNode, modulesNode, "indexLib", NULL, NULL, MXML_DESCEND);
    // 检查节点有效性
    if ( indexLibNode == NULL || indexLibNode->parent != modulesNode )
    {
        TERR("can't find indexLib's node");
        return -1;
    }

    // 检查增量持久化配置
    const char * syncStr = mxmlElementGetAttr(indexLibNode, "sync" );
    if (syncStr && strcasecmp(syncStr, "true") == 0) {
        syncFlag = true;
    }
    else {
        syncFlag = false;
    }

    // 检查mmap_lock配置
    const char * mLockStr = mxmlElementGetAttr(indexLibNode, "mmap_lock" );
    if (mLockStr && strcasecmp(mLockStr, "true") == 0) {
        struct rlimit sysLimit;
        if (getrlimit(RLIMIT_MEMLOCK, &sysLimit) == 0) {
            if ((sysLimit.rlim_cur==RLIM_INFINITY) && 
                    (sysLimit.rlim_max==RLIM_INFINITY)) 
            {
                IndexConfigParams::getInstance()->setMemLock();
            }
            else {
                TWARN("please unlimited max locked memory to enable mmap_lock");
            }
        }
        else {
            TWARN("get RLIMIT_MEMLOCK size failed, disable mmap_lock");
        }
    }

    // 关闭时是否导出倒排索引
    const char * et = mxmlElementGetAttr(indexLibNode, "export" );
    if (et && strcasecmp(et, "true") == 0) {
        exportFlag = true;
    }

    // 获取配置节点
    profileNode  = mxmlFindElement(indexLibNode, indexLibNode, "profile",  NULL, NULL, MXML_DESCEND);
    indexNode    = mxmlFindElement(indexLibNode, indexLibNode, "index",    NULL, NULL, MXML_DESCEND);
    idDictNode   = mxmlFindElement(indexLibNode, indexLibNode, "idDict",   NULL, NULL, MXML_DESCEND);
    provcityNode = mxmlFindElement(indexLibNode, indexLibNode, "provcity", NULL, NULL, MXML_DESCEND);
    add2modNode  = mxmlFindElement(indexLibNode, indexLibNode, "update_add2Modify", NULL, NULL, MXML_DESCEND);

    // 检查节点有效性
    if ( profileNode == NULL || profileNode->parent != indexLibNode )
    {
        TERR("can't find indexLib's child node profile");
        return -1;
    }

    if ( indexNode == NULL || indexNode->parent != indexLibNode )
    {
        TERR("can't find indexLib's child node index");
        return -1;
    }

    if ( idDictNode == NULL || idDictNode->parent != indexLibNode )
    {
        TERR("can't find indexLib's child node idDict");
        return -1;
    }

    if ( provcityNode == NULL || provcityNode->parent != indexLibNode )
    {
        TERR("can't find indexLib's child node provcity");
        return -1;
    }

    int32_t inc_max_num = 0;
    const char * inc_max_num_str = mxmlElementGetAttr(indexNode, "inc_max_num" );
    if ( inc_max_num_str ) {
        inc_max_num = atol(inc_max_num_str);
    }

    // 开始 初始化 各个模块
    if ( initProfile(   mxmlElementGetAttr(profileNode,  "path" ), syncFlag, false ) )  return -1;
    if ( initIdDict (   mxmlElementGetAttr(idDictNode,   "path" ), syncFlag ) )  return -1;
    if ( initFullIndex( mxmlElementGetAttr(indexNode,    "path" ) ) )  return -1;
    if ( initIncIndex ( mxmlElementGetAttr(indexNode,    "path" ), syncFlag, exportFlag, inc_max_num) )  return -1;
    if ( initProvCity ( mxmlElementGetAttr(provcityNode, "path" ) ) )  return -1;
    if ( initIncManager( add2modNode ) )                               return -1;

    return 0;
}


/**
 * 为重新进行索引内部数据的整理，  解析配置文件， 初始化 index_lib 内部的builder
 *
 * @param   config  xml的配置节点。  内部的子节点包括了 索引的各个配置项
 *         样例:
 *         <indexLib>
 *            <index    path="/dir" />
 *         </indexLib>
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int init_rebuild( mxml_node_t * config )
{
    mxml_node_t  * modulesNode  = NULL;
    mxml_node_t  * indexLibNode = NULL;               // index_lib的根节点
    mxml_node_t  * profileNode  = NULL;               // profile配置节点
    mxml_node_t  * indexNode    = NULL;               // index配置节点
    mxml_node_t  * idDictNode   = NULL;               // nid->docId 字典及deletemap节点

    modulesNode  = mxmlFindElement(config, config, "modules", NULL, NULL, MXML_DESCEND);
    if ( modulesNode == NULL || modulesNode->parent != config )
    {
        TERR("can't find modules's node");
        return -1;
    }

    indexLibNode = mxmlFindElement(modulesNode, modulesNode, "indexLib", NULL, NULL, MXML_DESCEND);
    // 检查节点有效性
    if ( indexLibNode == NULL || indexLibNode->parent != modulesNode )
    {
        TERR("can't find indexLib's node");
        return -1;
    }

    // 获取配置节点
    profileNode  = mxmlFindElement(indexLibNode, indexLibNode, "profile",  NULL, NULL, MXML_DESCEND);
    indexNode    = mxmlFindElement(indexLibNode, indexLibNode, "index",    NULL, NULL, MXML_DESCEND);
    idDictNode   = mxmlFindElement(indexLibNode, indexLibNode, "idDict",   NULL, NULL, MXML_DESCEND);

    // 检查节点有效性
    if ( profileNode == NULL || profileNode->parent != indexLibNode )
    {
        TERR("can't find indexLib's child node profile");
        return -1;
    }

    if ( indexNode == NULL || indexNode->parent != indexLibNode )
    {
        TERR("can't find indexLib's child node index");
        return -1;
    }

    if ( idDictNode == NULL || idDictNode->parent != indexLibNode )
    {
        TERR("can't find indexLib's child node idDict");
        return -1;
    }

    // 开始 初始化 各个模块
    if ( initProfile(    mxmlElementGetAttr(profileNode,  "path" ), false, true ) )  return -1;
    if ( initIdDict (    mxmlElementGetAttr(idDictNode,   "path" ), false) )  return -1;
    if ( initFullIndex(  mxmlElementGetAttr(indexNode,    "path" ) ) )  return -1;
    if ( initIdxBuilder( mxmlElementGetAttr(indexNode,    "path" ) ) )  return -1;

    return 0;
}






/**
 * 关闭index_lib内部开辟和打开的各种资源
 *
 * @return  0: success ;   -1: 程序处理失败
 */
int destroy()
{
    IndexIncManager::getInstance()->close();
    IndexReader::getInstance()->close();

    ProfileManager::freeInstance();
    DocIdManager::freeInstance();
    ProvCityManager::freeInstance();
    IncManager::freeInstance();

    return 0;
}



/**
 * 计算在特定排序条件下的 高频词的 新的签名，  在索引build时使用
 * 计算规则: 排序字段名 + 空格 + 排序方式  + 空格 + 原64位签名
 *
 * @param psFieldName       ps排序字段的名字
 * @param sortType          排序类型，  0：正排   1:倒排
 * @param termSign          term的签名
 *
 * @return 64 bit signature;  0： error
 */
uint64_t HFterm_sign64(const char * psFieldName,
                       uint32_t     sortType,
                       uint64_t     termSign)
{
    if (unlikely( NULL == psFieldName )) return 0;

    char  buf[ 1024 ]  = {0};
    char  buf2[ 1024 ] = {0};

    snprintf( buf,  sizeof(buf),  "%s",     psFieldName);
    snprintf( buf2, sizeof(buf2), "%s %u",  buf,  sortType);
    snprintf( buf,  sizeof(buf),  "%s %lu", buf2, termSign);

    return idx_sign64( buf, strlen(buf) );
}




/**
 * 计算在特定排序条件下的 高频词的 新的签名，  在  检索   时使用
 * 计算规则: 排序字段名 + 空格 + 排序方式  + 空格 + 原64位签名
 *
 * @param psFieldName       ps排序字段的名字
 * @param sortType          排序类型， 0：正排   1:倒排
 * @param term
 *
 * @return 64 bit signature;  0： error
 */
uint64_t HFterm_sign64(const char * psFieldName,
                       uint32_t     sortType,
                       const char * term)
{
    if (unlikely( NULL == psFieldName )) return 0;
    if (unlikely( NULL == term        )) return 0;

    uint64_t termSign = idx_sign64( term, strlen(term) );

    if (unlikely( 0 == termSign ))       return 0;

    return HFterm_sign64( psFieldName, sortType, termSign);
}





}

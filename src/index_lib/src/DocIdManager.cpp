/******************************************************************
 *      Author: yewang@taobao.com clm971910@gmail.com
 *
 *      Desc  : 维护索引内部的docid 和 外部数据唯一标识号 nid 的关系
 *              包括    1. docid生成器
 *                  2. nid->docid 的字典
 *                  3. 已删除docid的管理 (delete_map)
 *
 *      由一块辅助内存来存放，hash值相同的第一个节点的位置偏移。
 *      然后根据节点的next递归查找， 直到找到nid相同的节点。 其偏移就是docId
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <pthread.h>

#include "util/Log.h"
#include "util/filefunc.h"
#include "index_lib/DocIdManager.h"




#define   BLOCK_SIZE         5000000                  // 字典的初始大小
#define   BLOCK_STEP         5000000                  // 每次扩展的大小
#define   HASHTAB_SIZE       6291469                  // hash表的元素个数
#define   DELMAP_SIZE        1000000                  // deleteMap的初始大小
#define   DELMAP_STEP        200000                   // deleteMap 每次扩展的大小
#define   DELAY_TIME         (5 * 60)                 // 内存扩展时，延迟时间释放



namespace index_lib
{

DocIdManager             * DocIdManager::instance    = NULL;
DocIdManager::DeleteMap  * DocIdManager::instanceDM  = NULL;
ProfileManager           * DocIdManager::profileMgr  = NULL;
ProfileField             * DocIdManager::nidField    = NULL;
ProfileDocAccessor       * DocIdManager::docAccessor = NULL;
DocIdSyncManager         * DocIdManager::syncMgr     = NULL;



const char * DOCID_DICT_NAME    = "docId.dict";       // 持久化和加载时  字典 文件的名字
const char * DELMAP_NAME        = "del.bitmap";       // 持久化和加载时  deletemap 文件的名字
const char * DOCID_FULLDOC_NAME = "fullDoc.cnt";      // 持久化和加载时  全量doc计数的名字
const char * DOCID_FAILDOC_NAME = "failDoc.cnt";      // 持久化和加载时  更新失败doc计数的名字



/*****************************************************************************/



/**
 * 获取单实例对象指针, 实现类的单例
 *
 * @return  NULL: error
 */
DocIdManager * DocIdManager::getInstance()
{
    if ( NULL == instance )
    {
        profileMgr = ProfileManager::getInstance();
        if ( NULL == profileMgr )  return NULL;

        docAccessor = profileMgr->getDocAccessor();
        if ( NULL == docAccessor )  return NULL;

        instance = new DocIdManager();
        if ( false == instance->create() )  instance = NULL;
    }

    return instance;
}



/**
 * 释放类的单例
 */
void DocIdManager::freeInstance()
{
    if ( NULL != instance )   instance->destroy();
    if ( NULL != instanceDM ) delete( instanceDM );
    if ( NULL != instance )   delete( instance );
    if ( NULL != syncMgr )    delete( syncMgr );

    instanceDM = NULL;
    instance   = NULL;
    syncMgr    = NULL;
}



/** 构造函数 */
DocIdManager::DocIdManager()
{
    _delMap       = NULL;
    _oldDelMap    = NULL;
    _delMapSize   = 0;
    _delCount     = 0;
    _failCount    = 0;
    _hashSize     = 0;
    _hashTab      = NULL;
    _block        = NULL;
    _oldBlock     = NULL;
    _blockPos     = 0;
    _blockSize    = 0;
    _loadFlag     = false;
    _canFreeTime  = 0;
    _delMapFreeTime = 0;
    _path[0]      = '\0';
    _fullDocNum   = 0;
    _sync         = false;
}


/**
 * 析构函数   释放运行中  开辟的内存
 *
 * @return
 */
DocIdManager::~DocIdManager()
{
    destroy();
}




/**
 * 分配相关资源
 *
 * @return  false: 分配失败 ; true: 成功
 */
bool DocIdManager::create()
{
    _hashSize = HASHTAB_SIZE;
    _hashTab  = (uint32_t *) calloc( _hashSize, sizeof(uint32_t) );

    if ( NULL == _hashTab )  goto failed;

    for ( uint32_t i = 0; i < _hashSize; i++ )
    {
        _hashTab[i] = COMMON_NULL;
    }

    _blockSize = BLOCK_SIZE;
    _blockPos  = 0;
    _block     = (DD_NODE *) calloc( _blockSize, sizeof(DD_NODE) );

    if ( NULL == _block )   goto failed;

    for ( uint32_t i = 0; i < _blockSize; i++ )
    {
        _block[i].next = COMMON_NULL;
    }

    _delMapSize = DELMAP_SIZE;
    _delCount   = 0;
    _delMap     = (uint64_t *) calloc( _delMapSize, sizeof(uint64_t) );

    if ( NULL == _delMap )  goto failed;

    for ( uint32_t i = 0; i < _delMapSize; i++ )
    {
        _delMap[i] = 0xFFFFFFFFFFFFFFFF;              // 设置成全部删除
    }

    return true;

failed:
    destroy();

    return false;
}






/**
 * 销毁运行中分配的资源
 *
 * @return
 */
bool DocIdManager::destroy()
{
    SAFE_FREE( _delMap );
    SAFE_FREE( _oldDelMap );
    SAFE_FREE( _hashTab );
    SAFE_FREE( _block );
    SAFE_FREE( _oldBlock );

    return true;
}






/**
 * 扩展字典表 的内存
 *
 * @return  false: 扩展失败 出现严重错误
 */
bool DocIdManager::blockExpand()
{
    DD_NODE * block = NULL;

    SAFE_FREE( _oldBlock );

    block = (DD_NODE *) calloc( _blockSize + BLOCK_STEP, sizeof(DD_NODE) );

    if ( NULL == block )
    {
        TERR("realloc block failed. size: %lu", sizeof(DD_NODE) * ( _blockSize + BLOCK_STEP));
        return false;
    }

    if ( NULL != _block )
    {
        memcpy( block, _block, _blockSize * sizeof(DD_NODE) );

        _oldBlock = _block;                           // 旧内存先不释放，延后释放
        _block    = block;
        _canFreeTime = time(NULL) + DELAY_TIME;
    }
    else
    {
        _block = block;
    }

    for ( uint32_t i = _blockSize; i < _blockSize + BLOCK_STEP; i++ )
    {
        _block[i].next = COMMON_NULL;
    }
    _blockSize += BLOCK_STEP;
    return true;
}





/**
 * 添加一个对应关系到字典中， 如果nid已经存在，则返回错误
 *
 * @param   nid
 * @param   updateCount      是否更新计数
 *
 * @return  成功: 新的docId； 失败 : INVALID_DOCID
 */
uint32_t DocIdManager::addNid(int64_t nid, bool updateCount)
{
    if (unlikely( nidField == NULL ))
    {
        nidField = (ProfileField *)docAccessor->getProfileField( NID_FIELD_NAME );
        if ( NULL == nidField )
        {
            TERR("get profile field: nid failed !");
            return INVALID_DOCID;
        }

        if (unlikely( DT_INT64 != nidField->type ))
        {
            TERR("field type must be int64 !");
            return INVALID_DOCID;
        }
    }

    if( unlikely( _oldBlock ) )
    {
        if( time( NULL ) > _canFreeTime )
        {
            SAFE_FREE( _oldBlock );                           // 释放旧内存
        }
    }

    uint32_t   hashVal = nid % _hashSize;
    DD_NODE  * curNode = NULL;

    if ( COMMON_NULL != _hashTab[hashVal] )           // 之前已经有nid占用了这个位置了
    {
        uint32_t   nodePos = _hashTab[hashVal];
        DD_NODE  * pNode   = _block + nodePos;

        while ( pNode->next != COMMON_NULL )          // 循环找链表中的相同nid
        {
            if ( getNid( nodePos ) == nid )   break;

            nodePos = pNode->next;
            pNode   = _block + nodePos;
        }

        if ( getNid( nodePos ) == nid )               // 判断一下 是否真正找到了
        {
            return INVALID_DOCID;                     // 已经存在同一个nid， 返回错误
        }

        // 找不到同一个 nid , 添加新节点
        if ( _blockPos == _blockSize )                // array is full, expand
        {
            if ( false == blockExpand() )   return INVALID_DOCID;
            pNode = _block + nodePos;
        }

        curNode       = _block + _blockPos;           // 最后一个节点
        curNode->next = COMMON_NULL;
        pNode->next   = _blockPos;                    // 追加到最后

        if (updateCount)
        {
            unDelDocId( pNode->next );                // 初始化是删除的， 此时标记为 未删除
        }
        else
        {
            setUnDelDocId( pNode->next);              // 初始化是删除的， 此时标记为 未删除(不修改计数)
        }

        _blockPos++;
        if (syncMgr) {
            size_t filePos = sizeof(_hashSize) + sizeof(_blockPos) + sizeof(uint32_t) * _hashSize;
            syncMgr->putSyncInfo((char *)curNode, sizeof(DD_NODE), (sizeof(DD_NODE) * pNode->next + filePos) , false);
            syncMgr->putSyncInfo((char *)pNode,   sizeof(DD_NODE), (sizeof(DD_NODE) * nodePos + filePos),      false);
            syncMgr->putSyncInfo((char *)&_blockPos,sizeof(_blockPos), sizeof(_hashSize), false);
            syncMgr->syncToDisk();
        }

        return pNode->next;                           // 位置就是 docId
    }
    else                                              // can not find in hash table
    {
        if ( _blockPos == _blockSize )                // array is full, expand
        {
            if ( false == blockExpand() )   return INVALID_DOCID;
        }

        curNode       = _block + _blockPos;           // 最后一个节点
        curNode->next = COMMON_NULL;

        _hashTab[ hashVal ] = _blockPos;
        if ( updateCount )
        {
            unDelDocId( _blockPos );                  // 初始化是删除的， 此时标记为 未删除
        }
        else
        {
            setUnDelDocId( _blockPos);                // 初始化是删除的， 此时标记为 未删除(不修改计数)
        }

        _blockPos++;
        if (syncMgr) {
            size_t filePos = sizeof(_hashSize) + sizeof(_blockPos) + sizeof(uint32_t) * _hashSize;
            syncMgr->putSyncInfo((char *)curNode, sizeof(DD_NODE), (sizeof(DD_NODE) * (_blockPos - 1) + filePos) , false);
            filePos = sizeof(_hashSize) + sizeof(_blockPos) + sizeof(uint32_t) * hashVal;
            syncMgr->putSyncInfo((char *)&(_hashTab[ hashVal ]), sizeof(uint32_t), filePos, false);
            syncMgr->putSyncInfo((char *)&_blockPos, sizeof(_blockPos), sizeof(_hashSize), false);
            syncMgr->syncToDisk();
        }

        return _hashTab[ hashVal ];                   // 位置就是 docId
    }
}



/*****************************************************************************/

/**
 * 扩展deletemap
 * 采用这种buffer实现是为了确保绝对安全
 *
 * @return
 */
bool  DocIdManager::delMapExpand()
{
    uint64_t * delMap = NULL;
    SAFE_FREE(_oldDelMap);

    delMap = (uint64_t *) calloc( (_delMapSize + DELMAP_STEP), sizeof(uint64_t) );

    if ( NULL == delMap )
    {
        TERR("calloc delete map failed. size: %lu",
                  sizeof(uint64_t) * ( _delMapSize + DELMAP_STEP));

        return false;
    }

    if ( NULL != _delMap)
    {
        memcpy( delMap, _delMap, _delMapSize * sizeof(uint64_t) );
        _oldDelMap  = _delMap;
        _delMap     = delMap;
        _delMapFreeTime = time(NULL) + DELAY_TIME;
    }
    else
    {
        _delMap = delMap;
    }

    for ( uint32_t i = _delMapSize; i < (_delMapSize + DELMAP_STEP); i++ )
    {
        _delMap[i] = 0xFFFFFFFFFFFFFFFF;              // 设置成全部删除
    }

    _delMapSize += DELMAP_STEP;

    if (syncMgr) {
        size_t filePos = sizeof(_delMapSize) + sizeof(uint64_t) * (_delMapSize - DELMAP_STEP);
        syncMgr->putSyncInfo((char *)(_delMap + _delMapSize - DELMAP_STEP), sizeof(uint64_t) * DELMAP_STEP, filePos, true);
        syncMgr->putSyncInfo((char *)&_delMapSize, sizeof(_delMapSize), 0, true);
        syncMgr->syncToDisk();
    }
    return true;
}


/************************************************************************/


/**
 * 载入索引数据文件。  包括nid->docid的字典和 deletemap
 *
 * @param  filePath 持久化数据存放的路径
 *
 * @return  false: 加载失败;  true: 加载成功
 */
bool  DocIdManager::load(const char * filePath)
{
    nidField = (ProfileField *)docAccessor->getProfileField( NID_FIELD_NAME );
    if ( NULL == nidField )
    {
        TERR("get profile field: nid failed !");
        return false;
    }

    if ( DT_INT64 != nidField->type )
    {
        TERR("field type must be int64 !");
        return false;
    }

    if ( false == loadDict( filePath ) )   return false;
    if ( false == loadDelMap( filePath ) ) return false;

    if ( _sync ) {
        syncMgr = new DocIdSyncManager(filePath);
    }
    strcpy(_path, filePath);
    _loadFlag = true;
    return true;
}





/**
 * 载入   nid->docid的字典
 *
 * @param  filePath 持久化数据存放的路径
 *
 * @return  false: 载入失败;  true: 载入化成功
 */
bool  DocIdManager::loadDict(const char * filePath)
{
    return_val_if_fail( NULL != filePath, false);

    FILE      * fp       = NULL;
    uint32_t    hashSize = 0;
    uint32_t  * hashTab  = NULL;
    uint32_t    blockPos = 0;
    DD_NODE   * block    = NULL;
    char        fullPath[ PATH_MAX ] = {0};

    if ( strlen(filePath) >= (PATH_MAX - strlen(DOCID_DICT_NAME) - 1) )
        return false;

    snprintf( fullPath, sizeof(fullPath), "%s/%s", filePath, DOCID_DICT_NAME );

    fp = fopen(fullPath, "rb");
    if ( !fp )
    {
        TERR("open file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    if ( fread( &hashSize, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("read hashsize from file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    if ( fread( &blockPos, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("read blockPos from file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    // 释放初始化时分配的内存， 根据 实际的 大小 重新分配 hashtab
    hashTab = (uint32_t *) calloc( hashSize, sizeof(uint32_t) );

    if ( NULL == hashTab )
    {
        TERR("calloc block failed. size: %lu", sizeof(uint32_t) * hashSize);
        goto failed;
    }

    for ( uint32_t i = 0; i < hashSize; i++ )
    {
        hashTab[ i ] = COMMON_NULL;
    }

    // 释放初始化时分配的内存， 根据 实际的 大小 重新分配 block，  此处多分配了一部分
    block = (DD_NODE *) calloc( (blockPos + BLOCK_STEP), sizeof(DD_NODE) );

    if ( NULL == block )
    {
        TERR("calloc block failed. size: %lu", sizeof(DD_NODE) * (blockPos + BLOCK_STEP));
        goto failed;
    }

    for ( uint32_t i = 0; i < (blockPos + BLOCK_STEP); i++ )
    {
        block[ i ].next = COMMON_NULL;
    }

    // 读取真正的hash表值
    if ( fread( hashTab, sizeof(uint32_t), hashSize, fp ) != hashSize )
    {
        TERR("read hashtab from file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    // 读取字典数据
    if ( fread( block, sizeof(DD_NODE), blockPos, fp ) != blockPos )
    {
        TERR("read block from file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    // 都成功了  替换 类创建的时候 初始化的部分
    if ( NULL != _hashTab )  free (_hashTab);

    _hashTab  = hashTab;
    _hashSize = hashSize;

    if ( NULL != _block )  free (_block);

    _block     = block;
    _blockPos  = blockPos;
    _blockSize = blockPos + BLOCK_STEP;

    _fullDocNum = loadFullDocNum(filePath);
    _failCount  = loadFailDocNum(filePath);

    fclose( fp );
    return true;

failed:
    if ( fp )              fclose( fp );
    if ( NULL != hashTab ) free  ( hashTab );
    if ( NULL != block )   free  ( block );

    return false;
}





/**
 * 载入deletemap
 *
 * @param  filePath 持久化数据存放的路径
 *
 * @return  false: 载入失败;  true: 载入化成功
 */
bool  DocIdManager::loadDelMap(const char * filePath)
{
    return_val_if_fail( NULL != filePath, false);

    FILE      * fp         = NULL;
    uint64_t  * delMap     = NULL;
    uint32_t    delMapSize = 0;
    uint32_t    delCount   = 0;
    char        fullPath[ PATH_MAX ] = {0};

    if ( strlen(filePath) >= (PATH_MAX - strlen(DELMAP_NAME) - 1) )
        return false;

    snprintf(fullPath, sizeof(fullPath), "%s/%s", filePath, DELMAP_NAME);

    fp = fopen( fullPath, "rb" );
    if ( !fp )
    {
        TERR("open file %s for delmap save failed. errno:%d", fullPath, errno);
        goto failed;
    }

    if ( fread( &delMapSize, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("read delMapSize from file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    if ( fread( &delCount, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("read delCount from file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    // 释放初始化时分配的内存， 根据 实际的 大小 重新分配
    delMap = (uint64_t *) calloc( delMapSize, sizeof(uint64_t) );

    if ( NULL == delMap )
    {
        TERR("calloc delMap failed. size: %lu", sizeof(uint64_t) * delMapSize);
        goto failed;
    }

    if ( fread( delMap, sizeof(uint64_t), delMapSize, fp ) != delMapSize )
    {
        TERR("read delMap from file %s failed. errno %d", fullPath, errno);
        goto failed;
    }

    // 都成功了  替换 类创建的时候 初始化的部分
    if ( NULL != _delMap )  free (_delMap);

    _delMap     = delMap;
    _delMapSize = delMapSize;
    _delCount   = delCount;

    fclose(fp);
    return true;

failed:
    if ( NULL != delMap ) free  ( delMap );
    if ( fp )             fclose( fp );

    return false;
}






/**
 * 载入索引数据文件。  包括nid->docid的字典和 deletemap
 *
 * @param  filePath 持久化数据存放的路径
 *
 * @return  false: 持久化失败;  true: 持久化成功
 */
bool  DocIdManager::dump(const char * filePath)
{
    if ( false == dumpDict( filePath ) )   return false;
    if ( false == dumpDelMap( filePath ) ) return false;

    return true;
}




/**
 * 载入nid->docid的字典
 *
 * @param  filePath 持久化数据存放的路径
 *
 * @return  false: 持久化失败;  true: 持久化成功
 */
bool  DocIdManager::dumpDict(const char * filePath)
{
    return_val_if_fail( NULL != filePath, false);

    FILE  * fp = NULL;
    char    fullPath[ PATH_MAX ] = {0};

    if ( strlen(filePath) >= (PATH_MAX - strlen(DOCID_DICT_NAME) - 1) )
        return false;

    make_dir(filePath, 0755);                         // 创建输出目录

    snprintf(fullPath, sizeof(fullPath), "%s/%s", filePath, DOCID_DICT_NAME);

    fp = fopen( fullPath, "wb" );
    if ( !fp )
    {
        TERR("open file %s for dict save failed. errno:%d", fullPath, errno);
        goto failed;
    }

    // 先存hash表 和 字典 的 大小
    if ( fwrite( &_hashSize, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("write hashsize %s failed. errno:%d", fullPath, errno);
        goto failed;
    }

    if ( fwrite( &_blockPos, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("write block_pos %s failed. errno:%d", fullPath, errno);
        goto failed;
    }

    // 存 hash 表
    if ( fwrite( _hashTab, sizeof(uint32_t), _hashSize, fp ) != _hashSize )
    {
        TERR("write hashtab %s failed. size:%u errno:%d", fullPath, _hashSize, errno);
        goto failed;
    }

    // save blocks
    if ( fwrite( _block, sizeof(DD_NODE), _blockPos, fp ) != _blockPos )
    {
        TERR("write block %s failed. size:%u errno:%d", fullPath, _blockPos, errno);
        goto failed;
    }

    fclose(fp);
    return true;

failed:
    if (fp) fclose(fp);
    return false;
}





/**
 * 输出 deletemap
 *
 * @param  filePath 持久化数据存放的路径
 *
 * @return  false: 持久化失败;  true: 持久化成功
 */
bool  DocIdManager::dumpDelMap(const char * filePath)
{
    return_val_if_fail( NULL != filePath, false);

    FILE  * fp = NULL;
    char    fullPath[ PATH_MAX ] = {0};

    if ( strlen(filePath) >= (PATH_MAX - strlen(DELMAP_NAME) - 1) )
        return false;

    make_dir(filePath, 0755);

    snprintf(fullPath, sizeof(fullPath), "%s/%s", filePath, DELMAP_NAME);

    fp = fopen( fullPath, "wb" );
    if ( !fp )
    {
        TERR("open file %s for delmap save failed. errno:%d", fullPath, errno);
        goto failed;
    }

    if ( fwrite( &_delMapSize, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("write delMapSize %s failed. errno:%d", fullPath, errno);
        goto failed;
    }

    if ( fwrite( &_delCount, sizeof(uint32_t), 1, fp ) != 1 )
    {
        TERR("write delCount %s failed. errno:%d", fullPath, errno);
        goto failed;
    }

    if ( fwrite( _delMap, sizeof(uint64_t), _delMapSize, fp ) != _delMapSize )
    {
        TERR("write delMap %s failed. size:%u errno:%d", fullPath, _delMapSize, errno);
        goto failed;
    }

    fclose(fp);
    return true;

failed:
    if (fp) fclose(fp);
    return false;
}


/**
 * 重置内部数据， 用于测试
 */
void DocIdManager::reset()
{
    _delCount = 0;
    memset( _delMap,  0, sizeof(uint64_t) * _delMapSize );

    for ( uint32_t i = 0; i < _hashSize; i++ )
    {
        _hashTab[ i ] = COMMON_NULL;
    }

    _blockPos = 0;

    for ( uint32_t i = 0; i < _blockSize; i++ )
    {
        _block[ i ].next = COMMON_NULL;
    }
}


const char * DocIdManager::getPath()
{
    return _path;
}

uint32_t DocIdManager::getFullDocNum()
{
    return _fullDocNum;
}

uint32_t DocIdManager::loadFullDocNum(const char * filePath)
{
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/%s", filePath, DOCID_FULLDOC_NAME);

    char buf[20];
    FILE * fp = fopen(path, "r");
    if (fp == NULL) {
        _fullDocNum = _blockPos;
        dumpFullDocNum(filePath);
        return _fullDocNum;
    }

    size_t rnum = fread(buf, 1, 20, fp);
    if (rnum == 20) {
        rnum = 19;
    }

    buf[rnum] = '\0';
    _fullDocNum = strtoul(buf, NULL, 10);
    fclose(fp);
    return _fullDocNum;
}


void  DocIdManager::dumpFullDocNum(const char * filePath)
{
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/%s", filePath, DOCID_FULLDOC_NAME);

    FILE * fp = fopen(path, "w");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "%u", _fullDocNum);
    fclose(fp);
}


uint32_t DocIdManager::loadFailDocNum(const char * filePath)
{
    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/%s", filePath, DOCID_FAILDOC_NAME);

    char buf[20];
    FILE * fp = fopen(path, "r");
    if (fp == NULL) {
        _failCount = 0;
        return _failCount;
    }

    size_t rnum = fread(buf, 1, 20, fp);
    if (rnum == 20) {
        rnum = 19;
    }

    buf[rnum] = '\0';
    _failCount = strtoul(buf, NULL, 10);
    fclose(fp);
    return _failCount;
}


void DocIdManager::dumpFailDocNum()
{
    if ( !_sync ) {
        return;
    }

    char path[PATH_MAX];
    snprintf(path, PATH_MAX, "%s/%s", _path, DOCID_FAILDOC_NAME);

    FILE * fp = fopen(path, "w");
    if (fp == NULL) {
        return;
    }
    fprintf(fp, "%u", _failCount);
    fclose(fp);
}


DocIdSyncManager::DocIdSyncManager(const char * filePath, uint32_t init)
{
    char fileName[PATH_MAX];
    int  fd    = -1;
    snprintf(fileName, PATH_MAX, "%s/%s", filePath, DELMAP_NAME);
    fd = ::open(fileName, O_RDWR, 0644);
    if (fd < 0) {
        TERR("DocIdSyncManager open file:%s failed!", fileName);
    }
    _delMap_fd = fd;

    snprintf(fileName, PATH_MAX, "%s/%s", filePath, DOCID_DICT_NAME);
    fd = ::open(fileName, O_RDWR, 0644);
    if (fd < 0) {
        TERR("DocIdSyncManager open file:%s failed!", fileName);
    }
    _dict_fd = fd;

    _usedNum = 0;
    DocIdSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < init; i++) {
        pInfo = new DocIdSyncInfo();
        if (pInfo != NULL) {
            _syncInfoVector.push_back(pInfo);
        }
    }
}

DocIdSyncManager::~DocIdSyncManager()
{
    for(uint32_t i = 0; i < _syncInfoVector.size(); i++) {
        if( _syncInfoVector[i] ) {
            delete _syncInfoVector[i];
        }
    }
    _syncInfoVector.clear();
    _usedNum = 0;
    if (_dict_fd != -1) {
        ::close(_dict_fd);
        _dict_fd = -1;
    }

    if (_delMap_fd != -1) {
        ::close(_delMap_fd);
        _delMap_fd = -1;
    }
}

int DocIdSyncManager::putSyncInfo(char * pMemAddr, size_t len, size_t pos, bool delMap)
{
    if (unlikely(pMemAddr == NULL || _dict_fd == -1 || _delMap_fd == -1)) {
        return -1;
    }

    DocIdSyncInfo * pInfo = NULL;
    if (unlikely(_usedNum >= _syncInfoVector.size())) {
        if (!expand()) {
            return -1;
        }
    }

    pInfo = _syncInfoVector[_usedNum];
    pInfo->pMemAddr = pMemAddr;
    pInfo->len      = len;
    pInfo->pos      = pos;
    if (delMap) {
        pInfo->fd = _delMap_fd;
    }
    else {
        pInfo->fd = _dict_fd;
    }

    ++_usedNum;
    return 0;
}

void DocIdSyncManager::syncToDisk()
{
    DocIdSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < _usedNum; i++) {
        pInfo = _syncInfoVector[i];
        if (lseek(pInfo->fd, pInfo->pos, SEEK_SET) == -1) {
            TERR("DocIdSyncManager lseek failed:%s", strerror(errno));
            continue;
        }

        if ((ssize_t)pInfo->len != write(pInfo->fd, pInfo->pMemAddr, pInfo->len)) {
            TERR("DocIdSyncManager write failed:%s", strerror(errno));
        }
    }
    _usedNum = 0;
}

void DocIdSyncManager::reset()
{
    _usedNum = 0;
}

bool DocIdSyncManager::expand(uint32_t step)
{
    DocIdSyncInfo * pInfo = NULL;
    for(uint32_t i = 0; i < step; i++) {
        pInfo = new DocIdSyncInfo();
        if (pInfo != NULL) {
            _syncInfoVector.push_back(pInfo);
        }
        else {
            return false;
        }
    }
    return true;
}


}





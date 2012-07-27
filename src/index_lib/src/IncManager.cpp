/*******************************************************************
 * IncManager.h
 *
 *  Created on: 2011-3-21
 *      Author: yewang@taobao.com  clm971910@gmail.com
 *      Desc  : 提供添加 / 删除 / 修改消息处理功能和错误处理功能
 *              流程控制 及 日志
 *
 ******************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/time.h>

#include "util/Log.h"
#include "util/Vector.h"
#include "index_lib/IncManager.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/ProfileMultiValueIterator.h"

using namespace util;


#define   EPSINON_U       (0.00001)                   // 浮点差异的上界
#define   EPSINON_D       (-0.00001)                  // 浮点差异的下界

static const char * MSG_CNT_FILE = "inc_msg.cnt";


namespace index_lib
{

DocIdManager       * IncManager::_dmgr      = NULL;   // docId管理对象
IndexIncManager    * IncManager::_iimgr     = NULL;   // 增量倒排管理对象
ProfileManager     * IncManager::_pmgr      = NULL;   // profile的管理对象
IncManager         * IncManager::instance   = NULL;   // 类的惟一实例
ProfileDocAccessor * IncManager::_pAccessor = NULL;   // doc读取工具对象
IndexReader        * IncManager::_idxRdr    = NULL;   // 全量倒排索引的reader




/**
 * 用于qsort 的回调函数, 用于比较字符串
 *
 * @param p1  元素指针
 * @param p2  元素指针
 *
 * @return  0：相等 ; <0: p1小于p2;  >0: p1大于p2
 */
static int qsort_cmpStr(const void *p1, const void *p2)
{
    return strcmp(* (char * const *) p1, * (char * const *) p2);
}


/**
 * 用于qsort 的回调函数, 用于比较基本类型   int float .....
 *
 * @param p1  元素指针
 * @param p2  元素指针
 *
 * @return  0：相等 ; <0: p1小于p2;  >0: p1大于p2
 */
template<typename T>
static int qsort_cmpSimp(const void *p1, const void *p2)
{
    if ((* (T const *) p1) == (* (T const *) p2))  return 0;
    if ((* (T const *) p1) >  (* (T const *) p2))  return 1;

    return -1;
}



// 构造函数、析构函数
IncManager::IncManager(const char * path)
{
    if (path[0] == '\0') {
        snprintf(_msgCntPath, PATH_MAX, "%s", MSG_CNT_FILE);
    }
    else {
        snprintf(_msgCntPath, PATH_MAX, "%s/%s", path, MSG_CNT_FILE);
    }

    _msgCntFile  = NULL;
    _msgCount    = loadMsgCntFile();
    _newCount    = 0;
    _idxCount    = 0;
    _add2modFlag = false;
    _pMemPool    = new MemPool();
    _ignIdxMap   = NULL;
    _depPflMap   = NULL;
}

IncManager::~IncManager()
{
    if (_msgCntFile != NULL) {
        fclose(_msgCntFile);
        _msgCntFile = NULL;
    }

    TWARN("proc msg count: %u",         _msgCount);
    TWARN("add to index msg count: %u", _idxCount);
    TWARN("new doc msg count: %u",      _newCount);

    if ( NULL != _ignIdxMap ) hashmap_destroy( _ignIdxMap, NULL, NULL );
    if ( NULL != _depPflMap ) hashmap_destroy( _depPflMap, NULL, NULL );
    if ( NULL != _pMemPool  ) delete _pMemPool;
}



uint32_t IncManager::loadMsgCntFile()
{
    if (_msgCntFile != NULL) {
        fclose(_msgCntFile);
        _msgCntFile = NULL;
    }

    _msgCntFile = fopen(_msgCntPath, "r+");
    if (_msgCntFile == NULL) {
        return 0;
    }

    char buffer[11];
    size_t len = fread(buffer, 1, 11, _msgCntFile);
    if (len == 11) {
        len = 10;
    }
    buffer[len] = '\0';

    uint32_t cnt = strtoul(buffer, NULL, 10);
    return cnt;
}


void IncManager::syncMsgCntFile()
{
    if ( !_dmgr->getSyncFlag() ) {
        // 非持久化情况，不生成inc_msg.cnt文件
        return;
    }

    if (_msgCntFile == NULL) {
        _msgCntFile = fopen(_msgCntPath, "w+");
        if (_msgCntFile == NULL) {
            return;
        }
    }
    else {
        fseek(_msgCntFile, 0L, SEEK_SET);
    }

    fprintf(_msgCntFile, "%u", _msgCount);
    fflush(_msgCntFile);
}


/**
 * 获取单实例对象指针, 实现类的单例
 *
 * @return  NULL: error
 */
IncManager * IncManager::getInstance()
{
    if ( NULL == instance )
    {
        _dmgr      = DocIdManager::getInstance();
        _iimgr     = IndexIncManager::getInstance();
        _pmgr      = ProfileManager::getInstance();
        _pAccessor = ProfileManager::getDocAccessor();
        instance   = new IncManager(_dmgr->getPath());
        _idxRdr    = IndexReader::getInstance();
    }

    return instance;
}



/**
 * 释放类的单例
 */
void IncManager::freeInstance()
{
    if ( NULL != instance ) delete instance;

    instance = NULL;
}




/**
 * 删除一个doc
 *
 * @param msg
 *
 * @return ture:成功 ; false:失败
 */
bool IncManager::del(const DocIndexMessage * msg)
{
    return_val_if_fail( NULL != msg, false );

    _dmgr->delNid( msg->nid() );
    _msgCount++;
    syncMsgCntFile();
    return true;
}

/**
 * 反删除一个doc
 *
 * @param msg
 *
 * @return ture:成功 ; false:失败
 */
bool IncManager::undel(const DocIndexMessage * msg)
{
    return_val_if_fail( NULL != msg, false );
    ++_msgCount;
    
    int64_t   nid    = msg->nid();
    uint32_t  docId  = _dmgr->getDocId( nid );

    if ( INVALID_DOCID != docId ) {
        TLOG("undel %lu failed, because valid", nid);
        return true;
    }
    
    docId = _dmgr->getDelDocId( nid );
    if ( INVALID_DOCID == docId ) {
        TLOG("undel %lu failed, because not exist", nid);
        return true;
    }

    _dmgr->unDelDocId( docId );
    syncMsgCntFile();
    return true;
}


/**
 * 添加一个doc 到索引中， 需要处理其 正排和倒排部分
 *
 * @param msg
 *
 * @return ture:成功 ; false:失败
 */
bool IncManager::add(const DocIndexMessage * msg)
{
    return_val_if_fail( NULL != msg, false );

    int       ret    = -1;
    int64_t   nid    = msg->nid();
    uint32_t  docId  = _dmgr->getDocId( nid );
    uint32_t  docNum = _pmgr->getProfileDocNum();          // 在profile中已经存在的中的doc数

    ++_msgCount;
    if ( true == _add2modFlag )                            // 因为 消息 被拆分成了 del & add, del消息先处理。
    {                                                      // 这时是取不到nid对应的docid的
        if ( INVALID_DOCID == docId )
        {
            docId = _dmgr->getDelDocId( nid );

            if ( INVALID_DOCID == docId )                  // 在删除和非删除的列表里都找不到, 那就是新nid了
            {
                ++_newCount;
                // TNOTE("new nid:%ld ", nid);
            }
            else                                           // 曾经被删除过
            {
                ret = checkModify( docId, msg );

                if ( 0 == ret )
                {
                    _dmgr->unDelDocId( docId );            // 宝贝没有做任何修改, 设置为未删除
                    return true;                           // 直接返回
                }
            }
        }
        else
        {
            ret = checkModify( docId, msg );               // 此时的docId是正常的
            if ( 0 == ret )  return true;                  // 宝贝没有做任何修改
        }
    }

    /** ====================新增doc到索引中=================== */
    if ( INVALID_DOCID != docId )
        _dmgr->delDocId( docId );                         // 此nid已经在索引中存在, 删除

    docId = _dmgr->addNid( nid , false);                     // 重新获取新的docId
    if (unlikely( INVALID_DOCID == docId ))
    {
        TERR("add nid:%ld to dict failed", nid);
        return false;
    }
    _dmgr->setDelDocId( docId );                         // 先标记为删除

    while ( docId >= docNum )
    {
        _pmgr->newDocSegment();
        docNum = _pmgr->getProfileDocNum();
    }

    ret = procMsgProf( docId, msg );
    if (unlikely( 0 != ret ))        // 添加消息 到 profile
    {
        _dmgr->addFail();
        if (ret == -1) {
            TERR("add inc msg to profile failed!");
        }
        else {
            TWARN("add inc msg to profile failed!");
        }

        _msgCount++;
        syncMsgCntFile();
        return false;
    }

    ret = procMsgIndex( docId, msg);
    if (unlikely( 0 != ret ))        // 添加消息 到 index
    {
        _dmgr->addFail();
        if (ret == -1) {
            TERR("add inc msg to index failed!");
        }
        else {
            TWARN("add inc msg to index failed!");
        }

        _msgCount++;
        syncMsgCntFile();
        return false;
    }

    _dmgr->setUnDelDocId( docId );                       // 全部成功， 才标记为未 删除
    _msgCount++;
    ++_idxCount;

    syncMsgCntFile();
    return true;
}



/**
 * 处理消息的 index部分， 全部添加到  倒排索引中
 *
 * @param docId
 * @param msg       pb消息
 *
 * @return   true:成功；  false:失败
 */
int IncManager::procMsgIndex(uint32_t docId, const DocIndexMessage * msg)
{
    return_val_if_fail( NULL != msg, -1 );

    int       idxFieldNum = msg->index_fields_size(); // 倒排字段个数
    char    * fieldName   = NULL;                     // 倒排字段名
    int       termNum     = 0;                        // 每个倒排字段里面 term对象 的个数
    uint64_t  termSign;
    uint32_t  firstOcc;

    if (idxFieldNum < _iimgr->getFieldNum())
    {
        TWARN("index field num [%d != %u] of doc [%ld]!",
                    idxFieldNum, _iimgr->getFieldNum(), msg->nid() );
        return 1;
    }

    for ( int i = 0; i < idxFieldNum; ++i )           // 循环所有倒排字段
    {
        const DocIndexMessage_IndexField &idxField = msg->index_fields(i);

        fieldName = ( char * ) idxField.field_name().c_str();
        termNum   = idxField.terms_size();

        // 不存在的字段  不处理
        if (unlikely(false == _iimgr->hasField( fieldName ) ))    continue;

        for ( int j = 0; j < termNum; ++j )           // 循环这个字段的所有term
        {
            const DocIndexMessage_Term &term = idxField.terms(j);

            termSign = term.term_sign();
            firstOcc = 0;                             // 默认为0

            if ( term.occ_list_size() > 0 )
                firstOcc = term.occ_list( 0 );        // 取第一个occ信息

            // 调用indexIncManager 添加此倒排消息 判断是否成功
            if (  _iimgr->addTerm( fieldName, termSign, docId, firstOcc ) <= 0 )
                return -1;
        }
    }
    return 0;
}



/**
 * 处理消息的profile部分， 全部添加到  倒排索引中
 *
 * @param docId
 * @param msg       pb消息
 *
 * @return   true:成功；  false:失败
 */
int IncManager::procMsgProf(uint32_t docId, const DocIndexMessage * msg)
{
    return_val_if_fail( NULL != msg, -1);

    int fieldNum = msg->profile_fields_size();   // 正排字段个数
    if (fieldNum != (int)_pmgr->getProfileFieldNum())
    {
        TWARN("profile field num [%d != %u] of doc [%ld]!",
                fieldNum, _pmgr->getProfileFieldNum(), msg->nid() );
        return 1;
    }

    int ret = 0;
    const ProfileField * pField = NULL;
    for ( int i = 0; i < fieldNum; ++i )              // 循环所有正排字段
    {
        const DocIndexMessage_ProfileField & msgField    = msg->profile_fields( i );
        const char                         * field_value = msgField.field_value().c_str();
        const char                         * field_name  = msgField.field_name().c_str();

        pField = _pAccessor->getProfileField( field_name );

        if (unlikely(pField == NULL))
        {
            TWARN("field [%s] in profile!", field_name );
            ret = -1;
            break;
        }

        if (unlikely ( 0 != _pAccessor->setFieldValue( docId, pField, field_value, ' ')))
        {
            ret = -1;
            break;
        }
    }

    _pmgr->flush();
    return ret;
}






/**
 * 修改doc对应的 profile部分
 *
 * @param msg
 *
 * @return ture:成功 ; false:失败
 */
bool IncManager::update(const DocIndexMessage * msg)
{
    return_val_if_fail( NULL != msg, false);

    int64_t   nid   = msg->nid();
    uint32_t  docId = _dmgr->getDocId(nid);

    if ( INVALID_DOCID == docId )
        return false;

    _dmgr->setDelDocId( docId );                         // 先标记为  删除

    if ( false == procMsgProf( docId, msg ) )         // 更新消息 到 profile
    {
        TERR("update inc msg to profile failed!");

        _dmgr->addFail();
        _msgCount++;
        syncMsgCntFile();
        return false;
    }

    _dmgr->setUnDelDocId( docId );                       // 标记为 未  删除
    _msgCount++;
    syncMsgCntFile();
    return true;
}


int IncManager::checkIdxMod( uint32_t docId, const DocIndexMessage * msg )
{
    return_val_if_fail( NULL != msg, -1 );

    int              idxFieldNum   = msg->index_fields_size();  // 倒排字段个数
    char           * fieldName     = NULL;                      // 倒排字段名
    int              termNum       = 0;                         // 每个倒排字段里面 term对象 的个数
    IndexTerm      * idxTerm       = NULL;
    uint64_t         termSign      = 0;
    uint64_t         fieldNameSign = 0;
    const uint64_t * bitMap        = NULL;

    if (unlikely( NULL == _pMemPool ))  goto failed;

    for ( int i = 0; i < idxFieldNum; ++i )                     // 循环所有倒排字段
    {
        const DocIndexMessage_IndexField &idxField = msg->index_fields( i );

        fieldName     = ( char * ) idxField.field_name().c_str();
        fieldNameSign = idx_sign64( fieldName, strlen(fieldName) );
        termNum       = idxField.terms_size();

        if ( true == isIgnIdxField( fieldName ) ) continue;     // 下面的字段不用对比了

        _pMemPool->reset();
        if ( docId < _idxRdr->getDocNum() )                     // 去全量里面搜
        {
            if ( false == _idxRdr->hasField( fieldName ) )  continue;

            /** 第一步  检查当前field 的term列表 里面 是否有 指定的docId. (覆盖 增加 token的状况 （包括修改）) */
            for ( int j = 0; j < termNum; ++j )                 // 循环这个doc的这个字段的所有term
            {
                const DocIndexMessage_Term &term = idxField.terms(j);

                termSign = term.term_sign();
                idxTerm  = _idxRdr->getTerm( _pMemPool, fieldNameSign, termSign );

                if ( NULL == idxTerm )  return 2;

                bitMap = idxTerm->getBitMap();
                if ( NULL == bitMap )
                {
                    if ( docId != idxTerm->seek( docId ) )  return 2;
                }
                else
                {
                    if ( 0 == ( bitMap[ docId / 64 ] & bit_mask_tab[ docId % 64 ] ))
                        return 2;
                }
            }

            /** 第二步。 遍历 索引库中这个字段 所有的token， 看看有几个有这个特定的docId存在 (覆盖删除token的状况 ) */
            termSign = 0;
            if ( 0 != _idxRdr->getFirstTerm( fieldNameSign, termSign )) continue;

            int foundCount = 0;                                 // 找到指定docid的term个数
            while ( 0 != termSign )
            {
                idxTerm = _idxRdr->getTerm( _pMemPool, fieldNameSign, termSign );
                bitMap  = idxTerm->getBitMap();

                // 非原生。忽略
                if ( 1 == idxTerm->getTermInfo()->not_orig_flag )
                {
                    termSign = 0;
                    _idxRdr->getNextTerm( fieldNameSign, termSign );
                    continue;
                }

                if ( NULL == bitMap )
                {
                    if ( docId == idxTerm->seek( docId ) ) ++foundCount;
                }
                else
                {
                    if ( bitMap[ docId / 64 ] & bit_mask_tab[ docId % 64 ] )
                        ++foundCount;
                }

                if ( foundCount > termNum )  return 2;
                termSign = 0;
                _idxRdr->getNextTerm( fieldNameSign, termSign );    // 取下一个 term
            }

            if ( foundCount != termNum )  return 2;
        }
        else                                               // 去增量里面搜索
        {
            if (unlikely(false == _iimgr->hasField( fieldName ) )) continue;

            /** 第一步  检查当前field 的term列表 里面 是否有 指定的docId. (覆盖 增加 token的状况 （包括修改）) */
            for ( int j = 0; j < termNum; ++j )            // 循环这个doc的这个字段的所有term
            {
                const DocIndexMessage_Term &term = idxField.terms(j);

                termSign = term.term_sign();
                idxTerm  = _iimgr->getTerm( _pMemPool, fieldNameSign, termSign );

                if ( NULL == idxTerm )  return 2;

                bitMap = idxTerm->getBitMap();
                if ( NULL == bitMap )
                {
                    if ( docId != idxTerm->seek( docId ) )  return 2;
                }
                else
                {
                    if ( 0 == ( bitMap[ docId / 64 ] & bit_mask_tab[ docId % 64 ] ))
                        return 2;
                }
            }

            // 第二步。 遍历 索引库中这个字段 所有的token， 看看有几个有这个特定的docId存在 ( 覆盖删除token的状况 )
            termSign = 0;
            if ( 0 != _iimgr->getFirstTerm( fieldNameSign, termSign )) continue;

            int foundCount = 0;                            // 找到指定docid的term个数
            while ( 0 != termSign )
            {
                idxTerm = _iimgr->getTerm( _pMemPool, fieldNameSign, termSign );
                bitMap  = idxTerm->getBitMap();

                if ( NULL == bitMap )
                {
                    if ( docId == idxTerm->seek( docId ) ) ++foundCount;
                }
                else
                {
                    if ( bitMap[ docId / 64 ] & bit_mask_tab[ docId % 64 ] )
                        ++foundCount;
                }

                if ( foundCount > termNum )  return 2;
                termSign = 0;
                _iimgr->getNextTerm( fieldNameSign, termSign );    // 取下一个 term
            }
            if ( foundCount != termNum )  return 2;
        }
    }

    return 0;

failed:
    return -1;
}





#define CMP_PFL_MSG_FIELD( FUNC, TYPE, EM_TYPE, CONV_FUNC )                             \
    char  * strValArr[ 1024 ]    = {0};                                                 \
    char  * strValArrBuf[ 1024 ] = {0};                                                 \
    char    strBuf[ 10240 ]      = {0};                                                 \
                                                                                        \
    if ( NULL == pflField )   return -1;                                                \
                                                                                        \
    switch ( pflField->multiValNum )                                                    \
    {                                                                                   \
        case 1:                                                                         \
            if ( strValLen > 0 )                                                        \
            {                                                                           \
                if ( _pAccessor->get##FUNC ( docId , pflField )                         \
                        != ( TYPE ) CONV_FUNC(strVal, NULL, 10) ) return 1;             \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                if ( pflField->defaultEmpty.EM_TYPE                                     \
                        != _pAccessor->get##FUNC( docId , pflField ) ) return 1;        \
            }                                                                           \
            return 0;                                                                   \
        case 0:                                                                         \
        default:                                                                        \
            uint32_t strValNum = 0;                                                     \
            uint32_t pflValNum = _pAccessor->getValueNum( docId, pflField );            \
                                                                                        \
            for ( uint32_t i = 0; i < _pAccessor->getValueNum( docId, pflField ); ++i ) \
            {                                                                           \
                if ( _pAccessor->getRepeated##FUNC( docId, pflField, i )                \
                        == pflField->defaultEmpty.EM_TYPE )                             \
                    --pflValNum;                                                        \
            }                                                                           \
                                                                                        \
            if ( strValLen > 0 )                                                        \
            {                                                                           \
                memcpy( strBuf, strVal, strValLen );                                    \
                strBuf[ strValLen ] = '\0';                                             \
                                                                                        \
                uint32_t tmp_num = str_split_ex( strBuf, ' ', strValArrBuf,             \
                                                 sizeof(strValArrBuf)/sizeof(char*) );  \
                                                                                        \
                for ( uint32_t i = 0; i < tmp_num; ++i )                                \
                {                                                                       \
                    if ( NULL != strValArrBuf[ i ] && strlen( strValArrBuf[ i ] ) > 0 ) \
                    {                                                                   \
                        strValArr[ strValNum ] = strValArrBuf[ i ];                     \
                        ++strValNum;                                                    \
                    }                                                                   \
                }                                                                       \
            }                                                                           \
            if ( pflValNum != strValNum )  return 1;                                    \
            if ( pflValNum == 0 )          return 0;                                    \
                                                                                        \
            ProfileMultiValueIterator  pflIter;                                         \
            _pAccessor->getRepeatedValue( docId, pflField, pflIter );                   \
                                                                                        \
            Vector< TYPE >    vect_pfl( pflValNum );                                    \
            Vector< TYPE >    vect_msg( pflValNum );                                    \
                                                                                        \
            for ( uint32_t i = 0; i < pflValNum; ++i )                                  \
            {                                                                           \
                vect_msg.pushBack( ( TYPE ) CONV_FUNC( strValArr[ i ], NULL, 10) );     \
                vect_pfl.pushBack( pflIter.next##FUNC() );                              \
            }                                                                           \
                                                                                        \
            qsort( vect_msg.begin(), vect_msg.size(), sizeof(TYPE), qsort_cmpSimp< TYPE > );  \
            qsort( vect_pfl.begin(), vect_pfl.size(), sizeof(TYPE), qsort_cmpSimp< TYPE > );  \
                                                                                        \
            for ( uint32_t i = 0; i < pflValNum; ++i )                                  \
            {                                                                           \
                if ( vect_msg[ i ] != vect_pfl[ i ] ) return 1;                         \
            }                                                                           \
            return 0;                                                                   \
    }



#define CMP_PFL_MSG_FIELD_FLOAT( FUNC, TYPE, EM_TYPE, CONV_FUNC )                       \
    char  * strValArr[ 1024 ]    = {0};                                                 \
    char  * strValArrBuf[ 1024 ] = {0};                                                 \
    char    strBuf[ 10240 ]      = {0};                                                 \
                                                                                        \
    if ( NULL == pflField )   return -1;                                                \
                                                                                        \
    switch ( pflField->multiValNum )                                                    \
    {                                                                                   \
        case 1:                                                                         \
            if ( strValLen > 0 )                                                        \
            {                                                                           \
                TYPE tmp_val = CONV_FUNC( strVal, NULL )                                \
                                      - _pAccessor->get##FUNC ( docId, pflField );      \
                if ( tmp_val > EPSINON_U ) return 1;                                    \
                if ( tmp_val < EPSINON_D ) return 1;                                    \
            }                                                                           \
            else                                                                        \
            {                                                                           \
                if ( pflField->defaultEmpty.EM_TYPE                                     \
                        != _pAccessor->get##FUNC( docId , pflField ) ) return 1;        \
            }                                                                           \
            return 0;                                                                   \
        case 0:                                                                         \
        default:                                                                        \
            uint32_t strValNum = 0;                                                     \
            uint32_t pflValNum = _pAccessor->getValueNum( docId, pflField );            \
                                                                                        \
            for ( uint32_t i = 0; i < _pAccessor->getValueNum( docId, pflField ); ++i ) \
            {                                                                           \
                if ( _pAccessor->getRepeated##FUNC( docId, pflField, i )                \
                        == pflField->defaultEmpty.EM_TYPE )                             \
                    --pflValNum;                                                        \
            }                                                                           \
                                                                                        \
            if ( strValLen > 0 )                                                        \
            {                                                                           \
                memcpy( strBuf, strVal, strValLen );                                    \
                strBuf[ strValLen ] = '\0';                                             \
                                                                                        \
                uint32_t tmp_num = str_split_ex( strBuf, ' ', strValArrBuf,             \
                                                 sizeof(strValArrBuf)/sizeof(char*) );  \
                                                                                        \
                for ( uint32_t i = 0; i < tmp_num; ++i )                                \
                {                                                                       \
                    if ( NULL != strValArrBuf[ i ] && strlen( strValArrBuf[ i ] ) > 0 ) \
                    {                                                                   \
                        strValArr[ strValNum ] = strValArrBuf[ i ];                     \
                        ++strValNum;                                                    \
                    }                                                                   \
                }                                                                       \
            }                                                                           \
            if ( pflValNum != strValNum )  return 1;                                    \
            if ( pflValNum == 0 )          return 0;                                    \
                                                                                        \
            ProfileMultiValueIterator  pflIter;                                         \
            _pAccessor->getRepeatedValue( docId, pflField, pflIter );                   \
                                                                                        \
            Vector< TYPE >    vect_pfl( pflValNum );                                    \
            Vector< TYPE >    vect_msg( pflValNum );                                    \
                                                                                        \
            for ( uint32_t i = 0; i < pflValNum; ++i )                                  \
            {                                                                           \
                vect_msg.pushBack( CONV_FUNC( strValArr[ i ], NULL ) );                 \
                vect_pfl.pushBack( pflIter.next##FUNC() );                              \
            }                                                                           \
                                                                                        \
            qsort( vect_msg.begin(), vect_msg.size(), sizeof(TYPE), qsort_cmpSimp< TYPE > );  \
            qsort( vect_pfl.begin(), vect_pfl.size(), sizeof(TYPE), qsort_cmpSimp< TYPE > );  \
                                                                                        \
            for ( uint32_t i = 0; i < pflValNum; ++i )                                  \
            {                                                                           \
                TYPE tmp_val = vect_msg[ i ] - vect_pfl[ i ];                           \
                if ( tmp_val > EPSINON_U ) return 1;                                    \
                if ( tmp_val < EPSINON_D ) return 1;                                    \
            }                                                                           \
            return 0;                                                                   \
    }




/**
 * 对比 profile 中的一个字段 和 msg中的一个字段
 *
 * @return  0: 相同     1: 不相同     -1:出错了
 */
int IncManager::cmpPflMsgFieldInt8( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( Int8, int8_t, EV_INT8, strtoll ); }

int IncManager::cmpPflMsgFieldInt16 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( Int16, int16_t, EV_INT16, strtoll ); }

int IncManager::cmpPflMsgFieldInt32 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( Int32, int32_t, EV_INT32, strtoll ); }

int IncManager::cmpPflMsgFieldInt64 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( Int64, int64_t, EV_INT64, strtoll ); }

int IncManager::cmpPflMsgFieldUInt8 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( Int8, uint8_t, EV_INT8, strtoull ); }

int IncManager::cmpPflMsgFieldUInt16( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( UInt16, uint16_t, EV_UINT16, strtoull ); }

int IncManager::cmpPflMsgFieldUInt32( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( UInt32, uint32_t, EV_UINT32, strtoull ); }

int IncManager::cmpPflMsgFieldUInt64( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD( UInt64, uint64_t, EV_UINT64, strtoull ); }

int IncManager::cmpPflMsgFieldFloat ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD_FLOAT( Float, float, EV_FLOAT, strtof ); }

int IncManager::cmpPflMsgFieldDouble( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{    CMP_PFL_MSG_FIELD_FLOAT( Double, double, EV_DOUBLE, strtod ); }


int IncManager::cmpPflMsgFieldString( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen )
{
    char  * strValArr[ 1024 ]    = {0};                 // 按照空格切分出来的指针数组
    char  * strValArrBuf[ 1024 ] = {0};                 // 按照空格切分出来的指针数组
    char    strBuf[ 10240 ]      = {0};                 //

    if ( 0 == pflField )   return -1;

    switch ( pflField->multiValNum )
    {
        case 1:
            if ( strValLen > 0 )
            {
                if ( NULL == _pAccessor->getString( docId , pflField ) )  return 1;
                if ( 0 != strcmp( strVal, _pAccessor->getString( docId , pflField ) )) return 1;
            }
            else
            {
                if ( NULL != _pAccessor->getString( docId , pflField )
                        && (strlen( _pAccessor->getString( docId , pflField ) ) > 0) )
                    return 1;
            }

            return 0;
        case 0:
        default:
            uint32_t strValNum = 0;
            uint32_t pflValNum = _pAccessor->getValueNum( docId, pflField );

            for ( uint32_t i = 0; i < _pAccessor->getValueNum( docId, pflField ); ++i )
            {
                if ( NULL == _pAccessor->getRepeatedString( docId, pflField, i )
                      || ( 0 == strlen( _pAccessor->getRepeatedString( docId, pflField, i ))))
                {
                    --pflValNum;
                }
            }

            if ( strValLen > 0 )
            {
                memcpy( strBuf, strVal, strValLen );
                strBuf[ strValLen ] = '\0';

                uint32_t tmp_num = str_split_ex( strBuf, ' ', strValArrBuf,
                                                 sizeof(strValArrBuf)/sizeof(char*) );

                for ( uint32_t i = 0; i < tmp_num; ++i )
                {
                    if ( 0 != strValArrBuf[ i ] && strlen( strValArrBuf[ i ] ) > 0 )
                    {
                        strValArr[ strValNum ] = strValArrBuf[ i ];
                        ++strValNum;
                    }
                }
            }

            if ( pflValNum != strValNum )  return 1;
            if ( pflValNum == 0 )          return 0;

            ProfileMultiValueIterator  pflIter;
            _pAccessor->getRepeatedValue( docId, pflField, pflIter );

            Vector< char * >    vect_pfl( pflValNum );
            Vector< char * >    vect_msg( pflValNum );

            for ( uint32_t i = 0; i < pflValNum; ++i )
            {
                vect_msg.pushBack( strValArr[ i ] );
                vect_pfl.pushBack( (char *)pflIter.nextString() );
            }

            qsort( vect_msg.begin(), vect_msg.size(), sizeof(char *), qsort_cmpStr );
            qsort( vect_pfl.begin(), vect_pfl.size(), sizeof(char *), qsort_cmpStr );

            for ( uint32_t i = 0; i < pflValNum; ++i )
            {
                if ( 0 != strcmp(vect_msg[ i ], vect_pfl[ i ] )) return 1;
            }
            return 0;
    }
}







/**
 * 判断一个消息中的 正排部分 的 一个字段   有没有变化
 *
 * @param docId
 * @param msg          pb消息
 *
 * @return  0: 相同     1: 不相同     -1:出错了
 */
int IncManager::checkProfFieldMod( uint32_t docId, const string &fieldValue, const char * fieldName )
{
    const ProfileField * pflField  = NULL;
    const char         * strVal    = fieldValue.c_str();             // 字符串值, 空格分隔了多个值
    long                 strValLen = fieldValue.length();            // 字符串长度

    pflField = _pAccessor->getProfileField( fieldName );             // profile中的字段描述句柄
    if (unlikely( NULL == pflField ))  return 0;

    switch ( _pAccessor->getFieldDataType( pflField ) )
    {
        case DT_INT8:
            return cmpPflMsgFieldInt8( docId, pflField, strVal, strValLen );
        case DT_INT16:
            return cmpPflMsgFieldInt16( docId, pflField, strVal, strValLen );
        case DT_INT32:
            return cmpPflMsgFieldInt32( docId, pflField, strVal, strValLen );
        case DT_INT64:
            return cmpPflMsgFieldInt64( docId, pflField, strVal, strValLen );
        case DT_UINT8:
            return cmpPflMsgFieldUInt8( docId, pflField, strVal, strValLen );
        case DT_UINT16:
            return cmpPflMsgFieldUInt16( docId, pflField, strVal, strValLen );
        case DT_UINT32:
            return cmpPflMsgFieldUInt32( docId, pflField, strVal, strValLen );
        case DT_UINT64:
            return cmpPflMsgFieldUInt64( docId, pflField, strVal, strValLen );
        case DT_FLOAT:
            return cmpPflMsgFieldFloat( docId, pflField, strVal, strValLen );
        case DT_DOUBLE:
            return cmpPflMsgFieldDouble( docId, pflField, strVal, strValLen );
        case DT_STRING:
            return cmpPflMsgFieldString( docId, pflField, strVal, strValLen );
        default:
            return -1;
    }

    return 0;
}




int IncManager::checkProfMod( uint32_t docId, const DocIndexMessage * msg)
{
    return_val_if_fail( NULL != msg, -1);

    int  tmpRetVal = 0;
    int  retVal    = 0;

    for ( int j = 0; j < msg->profile_fields_size(); ++j )      // 循环所有正排字段
    {
        const DocIndexMessage_ProfileField &msgField = msg->profile_fields( j );

        const char * fieldName = msgField.field_name().c_str();

        tmpRetVal = checkProfFieldMod( docId, msgField.field_value(), fieldName );

        if ( -1 == tmpRetVal ) return -1;

        if ( 1 == tmpRetVal )
        {
            if ( true == isDepPflField( fieldName ) )  return 2;
            retVal = 3;
        }
    }

    return retVal;
}



bool IncManager::isIgnIdxField( const char * fieldName )
{
    if (unlikely( NULL == fieldName )) return false;

    if ( _pAccessor->getProfileField( fieldName ) != NULL )  return true;

    size_t      len  = strlen( fieldName );
    uint64_t    sign = 0;

    if ( 0 == len )   return false;

    sign = idx_sign64( fieldName , len );

    if ( NULL != hashmap_find( _ignIdxMap, (void *) sign )) return true;

    return false;
}



bool IncManager::isDepPflField( const char * fieldName )
{
    if (unlikely( NULL == fieldName )) return false;

    size_t      len  = strlen( fieldName );
    uint64_t    sign = 0;

    if ( 0 == len )   return false;

    sign = idx_sign64( fieldName , len );

    if ( _idxRdr->hasField( sign ) )  return true;
    if ( NULL != hashmap_find( _depPflMap, (void *) sign )) return true;

    return false;
}


/**
 * 对增量消息的处理 add转modify 进行配置
 * 添加不需要检查的倒排字段名。
 *
 * @param name    倒排字段名
 *
 * @return ture:成功 ; false:失败
 */
bool IncManager::addIgnIdxField( const char * name )
{
    if ( NULL == name )   return false;

    if ( NULL == _ignIdxMap )
    {
        _ignIdxMap = hashmap_create( 128, HM_FLAG_POOL, hashmap_hashInt, hashmap_cmpInt);
    }

    size_t      len  = strlen( name );
    uint64_t    sign = 0;

    if ( 0 == len ) return false;

    sign = idx_sign64( name , len );

    if ( hashmap_insert( _ignIdxMap, (void *) sign, (void *)1) == 0 )
    {
        _add2modFlag = true;
        return true;
    }

    return false;
}


/**
 * 对增量消息的处理 add转modify 进行配置
 * 添加被倒排字段依赖的 正排字段名，  这些正排字段如果修改了，意味着倒排字段也修改了。
 *
 * @param name   正排字段名
 *
 * @return  ture:成功 ; false:失败
 */
bool IncManager::addDepPflField( const char * name )
{
    if ( NULL == name )   return false;

    if ( NULL == _depPflMap )
    {
        _depPflMap = hashmap_create( 128, HM_FLAG_POOL, hashmap_hashInt, hashmap_cmpInt);
    }

    size_t      len  = strlen( name );
    uint64_t    sign = 0;

    if ( 0 == len ) return false;

    sign = idx_sign64( name , len );

    if ( hashmap_insert( _depPflMap, (void *) sign, (void *)1) == 0 )
    {
        _add2modFlag = true;
        return true;
    }

    return false;
}




int IncManager::checkModify( uint32_t docId, const DocIndexMessage * msg )
{
    int  prof_ret = checkProfMod( docId, msg );

    if ( 2 == prof_ret )  return 2;                     // 倒排部分变化了

    int      idx_ret = checkIdxMod( docId, msg );
    //int64_t  nid     = msg->nid();

    if ( 2 == idx_ret )
    {
        //TNOTE( "index change nid:%ld ", nid );          // 倒排部分变化了
        return 2;
    }

    if (( 0 == prof_ret ) && ( 0 == idx_ret ))
    {
        //TNOTE( "nothing change nid:%ld ", nid );        // 啥变化也没有 发什么增量 ？？
        return 0;
    }

    if ( 3 == prof_ret )
    {
        //TNOTE( "profile change nid:%ld ", nid );        // 正排部分变化了
        return 3;
    }

    return -1;
}

}

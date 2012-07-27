/*******************************************************************
 * IncManager.h
 *
 *  Created on: 2011-3-21
 *      Author: yewang@taobao.com  clm971910@gmail.com
 *      Desc  : 提供添加 / 删除 / 修改消息处理功能和错误处理功能
 *              流程控制 及 日志
 *
 ******************************************************************/

#ifndef INCMANAGER_H_
#define INCMANAGER_H_

#include <string>
#include "util/Log.h"
#include "util/strfunc.h"
#include "util/idx_sign.h"
#include "util/hashmap.h"
#include "commdef/commdef.h"
#include "IndexLib.h"
#include "commdef/DocIndexMessage.pb.h"
#include "DocIdManager.h"
#include "IndexIncManager.h"
#include "ProfileManager.h"
#include "IndexReader.h"

using namespace util;
using namespace std;

#define   MAX_PROF_HEADER_SIZE         4096           // 所有字段名的名字拼接成的 头得长度
#define   MAX_PROF_BODY_SIZE           40960          // 单一doc的profile信息总长度


namespace index_lib
{

class IncManager
{

public:

    /**
     * 获取单实例对象指针
     *
     * @return  NULL: error
     */
    static IncManager * getInstance();


    /**
     * 释放类的单例
     */
    static void freeInstance();



    /**
     * 添加一个doc 到索引中， 需要处理其 正排和倒排部分
     *
     * @param msg
     *
     * @return ture:成功 ; false:失败
     */
    bool add(const DocIndexMessage * msg);


    /**
     * 删除一个doc
     *
     * @param msg
     *
     * @return ture:成功 ; false:失败
     */
    bool del(const DocIndexMessage * msg);

    /**
     * 恢复一个doc
     *
     * @param msg
     *
     * @return ture:成功 ; false:失败
     */
    bool undel(const DocIndexMessage * msg);

    /**
     * 修改doc对应的 profile部分
     *
     * @param msg
     *
     * @return ture:成功 ; false:失败
     */
    bool update(const DocIndexMessage * msg);


    /**
     * 获取处理过的消息总数
     */
    uint32_t getMsgCount()   {  return _msgCount; };


    /**
     * 新增宝贝的个数
     */
    uint32_t getNewCount()   {  return _newCount; };


    /**
     * 添加到倒排的宝贝个数
     */
    uint32_t getIdxCount()   {  return _idxCount; };


    /**
     * 对增量消息的处理 add转modify 进行配置
     * 添加不需要检查的倒排字段名。
     *
     * @param name    倒排字段名
     *
     * @return ture:成功 ; false:失败
     */
    bool addIgnIdxField( const char * name );


    /**
     * 对增量消息的处理 add转modify 进行配置
     * 添加被倒排字段依赖的 正排字段名，  这些正排字段如果修改了，意味着倒排字段也修改了。
     *
     * @param name   正排字段名
     *
     * @return  ture:成功 ; false:失败
     */
    bool addDepPflField( const char * name );


private:
    // 构造函数、析构函数
    IncManager(const char * path);
    virtual ~IncManager();


    /**
     * 处理消息的 index部分， 全部添加到  倒排索引中
     *
     * @param docId
     * @param msg       pb消息
     *
     * @return    0:成功；-1:失败; 1:因更新消息字段不全造成失败
     */
    int procMsgIndex(uint32_t docId, const DocIndexMessage * msg);

    /**
     * 处理消息的profile部分， 全部添加到  倒排索引中
     *
     * @param docId
     * @param msg       pb消息
     *
     * @return    0:成功；-1:失败; 1:因更新消息字段不全造成失败
     */
    int procMsgProf(uint32_t docId, const DocIndexMessage * msg);

    /**
     * 加载msgCount文件
     *
     * @param path  .cnt文件目标存储目录
     *
     * @return  加载获取到的msgCount值
     */
    uint32_t loadMsgCntFile();

    /**
     * 持久化msgCount文件
     */
    void syncMsgCntFile();



    /**
     * 判断一个消息中的 具体哪些部分有变化
     *
     * @param msg     pb消息
     *
     * @return  0:无变化;  1:新增doc ;  2:倒排部分有变化; 3:正排部分有变化;   -1:失败
     */
    int checkModify( uint32_t docId, const DocIndexMessage * msg );




    bool isIgnIdxField( const char * fieldName );
    bool isDepPflField( const char * fieldName );


    /**
     * 判断一个消息中的 倒排部分有没有变化
     *
     * @param docId
     * @param msg          pb消息
     *
     * @return  0:无变化; 2:倒排部分有变化;  -1:失败
     */
    int checkIdxMod( uint32_t docId, const DocIndexMessage * msg );


    /**
     * 对比 profile 中的一个字段 和 msg中的一个字段
     *
     * @return  0: 相同     1: 不相同     -1:出错了
     */
    int cmpPflMsgFieldInt8  ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldInt16 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldInt32 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldInt64 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldUInt8 ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldUInt16( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldUInt32( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldUInt64( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldFloat ( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldDouble( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );
    int cmpPflMsgFieldString( uint32_t docId, const ProfileField * pflField, const char * strVal, long strValLen );



    /**
     * 判断一个消息中的 正排部分 的 一个字段   有没有变化
     *
     * @param docId
     * @param msg          pb消息
     *
      * @return  0: 相同     1: 不相同     -1:出错了
     */
    int checkProfFieldMod( uint32_t docId, const string &fieldValue, const char * fieldName );



    /**
     * 判断一个消息中的 正排部分有没有变化
     *
     * @param docId
     * @param msg          pb消息
     *
     * @return  0:无变化;  2:倒排部分有变化; 3:正排部分有变化;  -1:失败
     */
    int checkProfMod( uint32_t docId, const DocIndexMessage * msg );




private:
    static IncManager *  instance;                    // 惟一实例


private:
    static DocIdManager        * _dmgr;               // docId管理对象
    static IndexIncManager     * _iimgr;              // 增量倒排管理对象
    static IndexReader         * _idxRdr;             // 全量倒排索引的reader
    static ProfileManager      * _pmgr;               // profile的管理对象
    static ProfileDocAccessor  * _pAccessor;          // doc读取工具对象

    uint32_t                     _msgCount;           // 总共处理的消息个数
    uint32_t                     _newCount;           // 新增宝贝的个数
    uint32_t                     _idxCount;           // 添加到倒排的宝贝个数

    FILE                       * _msgCntFile;            // 记录更新消息数的文件句柄
    char                         _msgCntPath[PATH_MAX];  // 目标更新消息数存储路径

    /** ======= 对增量消息的处理 add转modify 进行配置  ======= */
    bool                         _add2modFlag;        // 是否执行add转modify转换的检测. 总开关。 默认关闭
    hashmap_t                    _ignIdxMap;          // 不需要检查的倒排字段名列表
    hashmap_t                    _depPflMap;          // 被依赖的正排字段字段名列表
    MemPool                    * _pMemPool;

};



}

#endif /* INCMANAGER_H_ */

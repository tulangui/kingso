/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ProfileManager.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: Profile模块全局对象定义文件 $
 ********************************************************************/
#ifndef KINGSO_INDEXLIB_PROFILEMANAGER_H_
#define KINGSO_INDEXLIB_PROFILEMANAGER_H_

#include <math.h>

#include "IndexLib.h"
#include "ProfileStruct.h"
#include "ProfileDocAccessor.h"

/**************** profile版本信息/随代码更新而更新 ****************/
const uint32_t MAIN_VERSION  = 1;   //主版本号(正排索引结构重大升级时变化)
const uint32_t SUB_VERSION   = 4;   //次版本号(正排索引内部数据结构调整时变化)
const uint32_t FIX_VERSION   = 0;   //bug修复版本号(随bug修复过程变化)

namespace index_lib
{

class DocIdManager;

/**
 * ProfileManager类定义
 */
class ProfileManager
{
public:
    /**
     * 获取ProfileManager单实例对象
     * @return   NULL, ERROR;
     *           Not NULL, 单实例对象指针
     */
    static inline ProfileManager * getInstance()
    {
        if (unlikely(_managerObj == NULL)) {
            _managerObj = new ProfileManager();
        }
        return _managerObj;
    }

    /**
     * 获取ProfileDocAccessor单实例对象
     * @return   NULL, ERROR;
     *           Not NULL, 单实例对象指针
     */
    static inline ProfileDocAccessor * getDocAccessor()
    {
        if (unlikely(_managerObj == NULL)) {
            return NULL;
        }
        return &(_managerObj->_docAccessor);
    }

    /**
     * 释放ProfileManager对象实例和相关资源
     */
    static void  freeInstance();

public:
    /**
     * 构造函数
     */
    ProfileManager();

    /**
     * 析构函数
     */
    ~ProfileManager();

    /**
     * 设置Profile数据的存取路径
     * @param  path   profile数据存储的路径
     * @return        0, OK; other, ERROR
     */
    int setProfilePath (const char *path);

    /**
     * 设置增量数据持久化标签
     * @param  sync  true: 本地持久化；false: 本地不持久化
     */
    void setDataSync ( bool sync )   {  _sync = sync; }

    /**
     * 设置DocIdDict数据的建库路径
     * @param  path   DocIdDict数据存储的路径
     * @return        0, OK; other, ERROR
     */
    int setDocIdDictPath (const char *path);

    /**
     * 添加一组BitRecord字段
     * @param  num            字段数量
     * @param  fieldNameArr   字段名称数组
     * @param  lenArr         字段位长数组
     * @param  dataTypeArr    字段类型数组
     * @param  encodeFlagArr  字段编码标识数组(true，需要编码；false，不需要编码)
     * @param  fieldFlagArr   字段Flag数组
     * @param  compressArr    字段压缩标识数组
     * @param  groupID        字段对应的分组下标
     * @return                0, OK; 负数, ERROR
     */
    int addBitRecordField(uint32_t num, char **fieldNameArr, char **fieldAliasArr, uint32_t *lenArr,
                          PF_DATA_TYPE *dataTypeArr, bool *encodeFlagArr,
                          PF_FIELD_FLAG *fieldFlagArr, EmptyValue *defaultArr, uint32_t groupId = 0);

    /**
     * 添加非BitRecord字段
     * @param  fieldName      字段名
     * @param  dataType       字段类型
     * @param  multiValNum    字段多值属性(0:变长， 1:单值， N:多值)
     * @param  encodeFlag     字段编码标识(外部指定是否需要编码，多值字段和变长字段默认需要编码)
     * @param  fieldFlag      字段Flag
     * @param  compress       字段是否需要压缩
     * @param  groupId        字段对应的分组下标
     * @param  overlap        字段是否支持更新排重功能
     * @return                0, OK; 负数, ERROR
     */
    int addField(const char *fieldName, const char * fieldAlias, PF_DATA_TYPE dataType, uint32_t multiValNum,
                 bool encodeFlag, PF_FIELD_FLAG fieldFlag, bool compress, EmptyValue defaultValue,
                 uint32_t groupId, bool overlap = true);

    /**
     * 建库过程设置profile文本索引的doc header
     * @param  header         Doc header文本对应的字段名序列
     * @return                0, OK; 负数, ERROR
     */
    int setDocHeader(const char *header);

    /**
     * 添加一条profile doc对应的文本索引数据入索引库
     * @param  line           单个doc对应的Profile文本索引字符串数据
     * @return                0, OK; 负数, ERROR
     */
    int addDoc(char *line);

    /**
     * 加载Profile索引数据
     * @param  update    是否支持更新过程
     * @return           0, OK; 负数, ERROR
     */
    int load(bool update = true);

    /**
     * dump索引数据到硬盘文件
     * @return     0, OK; 负数, ERROR
     */
    int dump();

    /**
     * flush增量索引数据到硬盘
     * @return     0, OK; 负数，ERROR
     */
    int flush();

    /**
     * 创建Profile主表数据对应的新Doc节点（如果segment文件装不下则创建新的segment)
     * @param  update 是否是更新阶段创建新结点(更新过程用true,建库过程用false)
     * @return     0, OK; 负数, ERROR
     */
    int newDocSegment(bool update = true);

    /**
     * 返回Profile索引的属性字段数量
     * @return     profile索引的字段数量
     */
    inline uint32_t getProfileFieldNum() { return _fieldNum; }

    /**
     * 返回profile索引记录的doc数量
     * @return     profile索引的当前doc数量
     */
    inline uint32_t getProfileDocNum() { return _profileResource.docNum; }

    /**
     * 返回ProfileField指针
     * @param  index   字段下标
     * @return         字段对应的ProfileField结构体指针
     */
    inline const ProfileField * getProfileFieldByIndex(uint32_t index)
    {
        if (unlikely(index >= _fieldNum)) {
            return NULL;
        }
        return _profileFieldArr[index];
    }

    /**
     * 打印版本信息
     */
    void  printProfileVersion();


    //========================================================
    /**
     * 将小profile合并起来
     *
     * @param  pflMgr  要被合并的小段索引
     *
     * @return         成功返回0，否则返回-1。
     */
    int  merge( ProfileManager * pflMgr );

    /**
     * 对所有因为updateOverlap==false的字段没有idx文件， 重建hashtable，
     *
     * @return  成功返回true，失败返回false。
     */
    bool rebuildHashTable();


    /** 加载Profile索引数据 , 用于修改 */
    int  loadForModify();

    /**
     * 生成dict文件。
     * @return       成功返回0，否则返回-1。
     */
    int regenDocidDict();


private:
    /**
     * 查找目标字段类型的数据存储空间大小
     * @param  type        字段数据类型
     * @return             数据类型对应的空间大小
     */
    uint32_t getDataTypeSize (PF_DATA_TYPE    type);

    /**
     * BitRecord内部按照字段长度进行组织排序
     * @param  pFieldLen   待排序的bit字段长度数组
     * @param  fieldNum    待排序bit字段的个数
     * @param  pPosArray   排序结果输出数组，记录了长度从大到小排序的字段下标位置
     * @param  arraySize   输出数组的空间大小
     * @return             true,OK; false,ERROR
     */
    bool     bitFieldSortByLen(uint32_t *pFieldLen, uint32_t fieldNum, uint32_t *pPosArray, uint32_t arraySize);

    /**
     * 计算目标bit字段的读取掩码
     * @param  start       bit字段在对应32bit空间中的起始位置
     * @param  len         bit字段的位长度
     * @return             读数据对应的掩码
     */
    uint32_t getReadMask(uint32_t start, uint32_t len);

    /**
     * 计算目标bit字段的写入掩码
     * @param  start       bit字段在对应32bit空间中的起始位置
     * @param  len         bit字段的位长度
     * @return             写数据对应的掩码
     */
    uint32_t getWriteMask(uint32_t start, uint32_t len);

    /**
     * dump Profile描述信息
     * @param  sync        是否同步更新到硬盘
     * @return             0, OK; false, ERROR
     */
    int      dumpProfileDesc(bool sync = true);

    /**
     * dump Profile主表信息
     * @param  param       是否同步到硬盘更新
     * @return             0, OK; false, ERROR
     */
    int      dumpProfileAttributeGroup(bool sync = true);

    /**
     * dump 编码表数据
     * @param  param       是否同步到硬盘更新
     * @return             0, OK; false, ERROR
     */
    int      dumpProfileEncodeFile(bool sync = true);

    /** dump Profile增量过程中的doc count信息 */
    int      dumpProfileDocCount(const char * fileName);

    /** load Profile增量过程中的doc count信息 */
    int      loadProfileDocCount(const char * fileName, uint32_t &doc_num);

    /**
     * dump DocId Dict数据
     * @return             0, OK; false, ERROR
     */
    int      dumpDocIdDict();

    /**
     * 检查profile加载数据和加载代码是否一致
     * @param   mVersion   主版本号
     * @param   sVersion   次版本号
     * @param   fVersion   修复版本号
     * @return             true,版本一致; false,版本不一致
     */
    bool     checkProfileVersion(uint32_t mVersion, uint32_t sVersion, uint32_t fVersion);

    /**
     * load Profile描述信息
     * @return             0, OK; false, ERROR
     */
    int      loadProfileDesc();

    /**
     * load Profile主表信息
     * @return             0, OK; false, ERROR
     */
    int      loadProfileAttributeGroup( bool canModify = false );

    /**
     * load 编码表数据
     * @pamam   expand     加载的编码表文件是否扩展空间
     * @return             0, OK; false, ERROR
     */
    int      loadProfileEncodeFile(bool expand);

    // test only for QA
    void     outputIncDocInfo();


private:
    static ProfileManager *_managerObj;    //单实例的ProfileManager对象指针

protected:
    ProfileField       *_profileFieldArr[MAX_PROFILE_FIELD_NUM]; //ProfileField字段存储的指针数组
    uint32_t            _fieldNum;                               //ProfileField字段数量
    ProfileResource     _profileResource;                        //ProfileResource资源结构体对象
    ProfileDocAccessor  _docAccessor;                            //ProfileDocAccessor对象实例的指针
    char                _profilePath [PATH_MAX];                 //profile索引数据所在路径
    const ProfileField *_docHeaderArr[MAX_PROFILE_FIELD_NUM];    //建库过程文本索引字段名称序列对应的ProfileField结构体指针
    uint32_t            _headerNum;                              //文本索引头信息中的字段个数
    char                _dictPath    [PATH_MAX];                 //docId dict所在目录的路径
    DocIdManager       *_pDictManager;                           //docId Manager的单实例指针
    int                 _nidPos;                                 //nid在文本索引结构中的字段位置

    uint32_t            _buildDocNum;                            //加载索引中的doc数量
    int                 _docCntFd;                               //记录增量后最新doc个数的文件句柄
    bool                _sync;                                   // 增量数据持久化flag
};

}

#endif //KINGSO_INDEXLIB_PROFILEMANAGER_H_

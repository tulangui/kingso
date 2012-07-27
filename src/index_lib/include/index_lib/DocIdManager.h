/******************************************************************
 * DocIdManager.h
 *
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


#ifndef DOCIDMANAGER_H_
#define DOCIDMANAGER_H_


#include "commdef/commdef.h"
#include "IndexLib.h"
#include "ProfileDocAccessor.h"
#include "ProfileManager.h"
#include "ProfileStruct.h"
#include <vector>

using namespace std;


#define   COMMON_NULL        0xFFFFFFFE               // 不可用的next


namespace index_lib
{

/**
 * 64个元素， 每个元素只有一个位=1 其余位=0
 */
const uint64_t bit_mask_tab[64]={
    1L<<0,  1L<<1,  1L<<2,  1L<<3,  1L<<4,  1L<<5,  1L<<6,  1L<<7,
    1L<<8,  1L<<9,  1L<<10, 1L<<11, 1L<<12, 1L<<13, 1L<<14, 1L<<15,
    1L<<16, 1L<<17, 1L<<18, 1L<<19, 1L<<20, 1L<<21, 1L<<22, 1L<<23,
    1L<<24, 1L<<25, 1L<<26, 1L<<27, 1L<<28, 1L<<29, 1L<<30, 1L<<31,
    1L<<32, 1L<<33, 1L<<34, 1L<<35, 1L<<36, 1L<<37, 1L<<38, 1L<<39,
    1L<<40, 1L<<41, 1L<<42, 1L<<43, 1L<<44, 1L<<45, 1L<<46, 1L<<47,
    1L<<48, 1L<<49, 1L<<50, 1L<<51, 1L<<52, 1L<<53, 1L<<54, 1L<<55,
    1L<<56, 1L<<57, 1L<<58, 1L<<59, 1L<<60, 1L<<61, 1L<<62, 1L<<63
};


/**
 * 64个元素， 每个元素只有一个位=0 其余位=1
 */
const uint64_t bit_mask_tab2[64]={
    ~(1L<<0),  ~(1L<<1),  ~(1L<<2),  ~(1L<<3),  ~(1L<<4),  ~(1L<<5),  ~(1L<<6),  ~(1L<<7),
    ~(1L<<8),  ~(1L<<9),  ~(1L<<10), ~(1L<<11), ~(1L<<12), ~(1L<<13), ~(1L<<14), ~(1L<<15),
    ~(1L<<16), ~(1L<<17), ~(1L<<18), ~(1L<<19), ~(1L<<20), ~(1L<<21), ~(1L<<22), ~(1L<<23),
    ~(1L<<24), ~(1L<<25), ~(1L<<26), ~(1L<<27), ~(1L<<28), ~(1L<<29), ~(1L<<30), ~(1L<<31),
    ~(1L<<32), ~(1L<<33), ~(1L<<34), ~(1L<<35), ~(1L<<36), ~(1L<<37), ~(1L<<38), ~(1L<<39),
    ~(1L<<40), ~(1L<<41), ~(1L<<42), ~(1L<<43), ~(1L<<44), ~(1L<<45), ~(1L<<46), ~(1L<<47),
    ~(1L<<48), ~(1L<<49), ~(1L<<50), ~(1L<<51), ~(1L<<52), ~(1L<<53), ~(1L<<54), ~(1L<<55),
    ~(1L<<56), ~(1L<<57), ~(1L<<58), ~(1L<<59), ~(1L<<60), ~(1L<<61), ~(1L<<62), ~(1L<<63)
};



/**
 * docid 字典节点
 */
#pragma pack (1)
typedef struct
{
//  uint64_t       nid;          // 宝贝的nid
    uint32_t       next;         // 同一个hash值的下一个节点
} DD_NODE;
#pragma pack ()

struct DocIdSyncInfo
{
    public:
        char * pMemAddr;         // value address in memory
        size_t len;              // value length
        size_t pos;              // value pos in file
        int    fd;               // file discriptor

    public:
        explicit DocIdSyncInfo():pMemAddr(NULL),len(0),pos(0),fd(-1) { }
};

class DocIdSyncManager
{
    public:
        DocIdSyncManager(const char * filePath, uint32_t init = 30);
        ~DocIdSyncManager();

        /*
         * @param  delMap true,持久化到delMap文件；false,持久化到docid.dict
         */
        int putSyncInfo(char * pMemAddr, size_t len, size_t pos, bool delMap);
        void syncToDisk();
        void reset();

    protected:
        bool expand(uint32_t step = 10);

    private:
        vector <DocIdSyncInfo *>   _syncInfoVector;   // SyncInfo
        uint32_t                   _usedNum;          // SyncInfo vector中的个数
        int                        _delMap_fd;        // delMap 对应的fd
        int                        _dict_fd;          // dict 对应的fd
};

class DocIdManager
{

public:

    /* 封装轻量级的对外服务 */
    class DeleteMap
    {
    public:
        /**
         * 检查一个docid是否 已经被删除
         *
         * @param docId
         *
         * @return  true:删除   false:未删除
         */
        inline bool isDel(uint32_t docId)
        {
            return instance->isDocIdDel( docId );
        };
    };


    /**
     * 获取单实例对象指针, 实现类的单例
     *
     * @return  NULL: error
     */
    static DocIdManager * getInstance();


    /**
     * 释放类的单例
     */
    static void freeInstance();

    /**
     * 设置增量数据持久化标签
     * @param  sync  true: 本地持久化；false: 本地不持久化
     */
    void setDataSync ( bool sync )   {  _sync = sync; }

    /**
     * @return 持久化标签
     */
    bool getSyncFlag () { return _sync; }


    /**
     * 添加一个对应关系到字典中， 如果nid已经存在，则返回错误
     *
     * @param   nid
     * @param   updateCount      是否更新delete计数
     *
     * @return  成功: 新的docId； 失败 : INVALID_DOCID
     */
    uint32_t addNid(int64_t nid, bool updateCount = true);


    /**
     * 从字典中删除一个对应关系
     * 并将对应的docid在 delete map 上表示为删除
     *
     * @param nid
     */
    inline void delNid(int64_t nid)
    {
        uint32_t docId = getDocId( nid );

        if ( INVALID_DOCID ==  docId ) return;

        delDocId(docId);
    };



    /**
     * 根据docId 返回 对应的 nid
     *
     * @param   docId
     *
     * @return  成功: nid； 失败 : INVALID_NID
     */
    inline int64_t getNid(uint32_t docId)
    {
        if ( unlikely( docId >= _blockPos ) )         return INVALID_NID;
        if ( unlikely( true == isDocIdDel(docId) ) )  return INVALID_NID;
        if ( unlikely( docAccessor == NULL ) )        return INVALID_NID;

        return docAccessor->getInt64( docId , nidField );
    }


    /**
     * 根据 已经被删除的 docId 返回 对应的 nid
     *
     * @param   docId
     *
     * @return  成功: nid； 失败 : INVALID_NID
     */
    inline int64_t getDelNid(uint32_t docId)
    {
        if ( unlikely( docId >= _blockPos ) )         return INVALID_NID;
        if ( unlikely( false == isDocIdDel(docId) ) ) return INVALID_NID;
        if ( unlikely( docAccessor == NULL ) )        return INVALID_NID;

        return docAccessor->getInt64( docId , nidField );
    }


    /**
     * 根据nid  查询已有的docid
     *
     * @param   nid
     *
     * @return  存在: 对应的docid; 不存在: INVALID_DOCID
     */
    inline uint32_t getDocId(int64_t nid)
    {
        if ( unlikely( _hashTab == NULL ) )    return INVALID_DOCID;
        if ( unlikely( _block   == NULL ) )    return INVALID_DOCID;

        uint32_t hashVal = ( (uint64_t) nid ) % _hashSize;

        if ( COMMON_NULL == _hashTab[ hashVal ] )          // 不存在
        {
            return INVALID_DOCID;
        }

        uint32_t  docId = _hashTab[ hashVal ];

        while ( _block[ docId ].next != COMMON_NULL )      // 循环找链表中的相同nid
        {
            if ( getNid( docId ) == nid ) return docId;

            docId = _block[ docId ].next;                  // next 标示的是 下一个节点的docId
        }

        if ( getNid( docId ) == nid ) return docId;

        return INVALID_DOCID;                              // 位置就是 docId
    }



    /**
     * 根据nid  查询已有的, 但已经被标识为删除的 docid, 如果有2个相同, 取后加入的
     *
     * @param   nid
     *
     * @return  存在: 对应的docid; 不存在: INVALID_DOCID
     */
    inline uint32_t getDelDocId(int64_t nid)
    {
        if ( unlikely( _hashTab == NULL ) )    return INVALID_DOCID;
        if ( unlikely( _block   == NULL ) )    return INVALID_DOCID;

        uint32_t hashVal = ( (uint64_t) nid ) % _hashSize;

        if ( COMMON_NULL == _hashTab[ hashVal ] )          // 不存在
        {
            return INVALID_DOCID;
        }

        uint32_t  docId    = _hashTab[ hashVal ];
        uint32_t  retDocId = INVALID_DOCID;

        while ( _block[ docId ].next != COMMON_NULL )      // 循环找链表中的相同nid
        {
            if ( getDelNid( docId ) == nid )  retDocId = docId;

            docId = _block[ docId ].next;                  // next 标示的是 下一个节点的docId
        }

        if ( getDelNid( docId ) == nid ) retDocId = docId;

        return retDocId;                                   // 位置就是 docId
    }


    /**
     * 删除docid 在 delete_map 标示删除
     *
     * @param docId
     */
    inline void delDocId(uint32_t docId)
    {
        while ( ( _delMapSize << 6 ) <= docId )            // 已经分配的空间不够
        {
            if ( false == delMapExpand() )  return;
        }

        if ( unlikely( _delMap == NULL ) )   return;

        if( unlikely( _oldDelMap ) )
        {
            if( time( NULL ) > _delMapFreeTime )
            {
                SAFE_FREE( _oldDelMap );                           // 释放旧内存
            }
        }

        uint32_t segPos   = docId >> 6;
        uint32_t inSegPos = docId & 0x3F;
        if ( ( _delMap [ segPos ] & bit_mask_tab[ inSegPos ] ) == 0) {
            _delMap[ segPos ] |= bit_mask_tab[ inSegPos ];
            _delCount++;
            if (syncMgr) {
                uint32_t endian_pos = inSegPos / 8;
                // x86小端存储专用计算方式
                size_t filePos = sizeof(_delMapSize) + sizeof(_delCount) + segPos * sizeof(uint64_t) + endian_pos;
                syncMgr->putSyncInfo((char *)(_delMap + segPos) + endian_pos, sizeof(uint8_t), filePos, true);
                filePos = sizeof(_delMapSize);
                syncMgr->putSyncInfo((char *)&_delCount, sizeof(_delCount), filePos, true);
                syncMgr->syncToDisk();
            }
        }
    }


    /**
     * 删除docid 在 delete_map 标示删除，且不更新delCount值
     *
     * @param docId
     */
    inline void setDelDocId(uint32_t docId)
    {
        while ( ( _delMapSize << 6 ) <= docId )            // 已经分配的空间不够
        {
            if ( false == delMapExpand() )  return;
        }

        if ( unlikely( _delMap == NULL ) )   return;
        uint32_t segPos   = docId >> 6;
        uint32_t inSegPos = docId & 0x3F;
        if ( ( _delMap [ segPos ] & bit_mask_tab[ inSegPos ] ) == 0) {
            _delMap[ segPos ] |= bit_mask_tab[ inSegPos ];
            if (syncMgr) {
                uint32_t endian_pos = inSegPos / 8;
                // x86小端存储专用计算方式
                size_t filePos = sizeof(_delMapSize) + sizeof(_delCount) + segPos * sizeof(uint64_t) + endian_pos;
                syncMgr->putSyncInfo((char *)(_delMap + segPos) + endian_pos, sizeof(uint8_t), filePos, true);
                syncMgr->syncToDisk();
            }
        }
    }

    /**
     * 在 delete_map 标示docid 未删除
     *
     * @param docId
     */
    inline void unDelDocId(uint32_t docId)
    {
        if ( unlikely( _delMap == NULL ) )   return;

        if ( ( _delMapSize << 6 ) <= docId )   return;

        uint32_t segPos   = docId >> 6;
        uint32_t inSegPos = docId & 0x3F;
        if ( ( _delMap[ segPos ] & bit_mask_tab[ inSegPos ] ) != 0) {
            _delMap[ segPos ] &= bit_mask_tab2[ inSegPos ];
            _delCount--;
            if (syncMgr) {
                uint32_t endian_pos = inSegPos / 8;
                // x86小端存储专用计算方式
                size_t filePos = sizeof(_delMapSize) + sizeof(_delCount) + segPos * sizeof(uint64_t) + endian_pos;
                syncMgr->putSyncInfo((char *)(_delMap + segPos) + endian_pos, sizeof(uint8_t), filePos, true);
                filePos = sizeof(_delMapSize);
                syncMgr->putSyncInfo((char *)&_delCount, sizeof(_delCount), filePos, true);
                syncMgr->syncToDisk();
            }
        }
    }

    /**
     * 在 delete_map 标识docid 未删除，且不更新delCount值
     *
     * @param docId
     */
    inline void setUnDelDocId(uint32_t docId)
    {
        if ( unlikely( _delMap == NULL ) )   return;

        if ( ( _delMapSize << 6 ) <= docId )   return;

        uint32_t segPos   = docId >> 6;
        uint32_t inSegPos = docId & 0x3F;
        if ( ( _delMap[ segPos ] & bit_mask_tab[ inSegPos ] ) != 0) {
            _delMap[ segPos ] &= bit_mask_tab2[ inSegPos ];
            if (syncMgr) {
                uint32_t endian_pos = inSegPos / 8;
                // x86小端存储专用计算方式
                size_t filePos = sizeof(_delMapSize) + sizeof(_delCount) + segPos * sizeof(uint64_t) + endian_pos;
                syncMgr->putSyncInfo((char *)(_delMap + segPos) + endian_pos, sizeof(uint8_t), filePos, true);
                syncMgr->syncToDisk();
            }
        }
    }


    /**
     * 检查一个docid是否 已经被删除
     *
     * @param docId
     *
     * @return  true:删除   false:未删除
     */
    inline bool isDocIdDel(uint32_t docId)
    {
        if ( unlikely(( _delMapSize << 6 ) <= docId) )  return true;

        uint32_t segPos   = docId >> 6;
        uint32_t inSegPos = docId & 0x3F;
        if ( 0 != (_delMap[ segPos ] & bit_mask_tab[ inSegPos ]) )
        {
            return true;
        }

        return false;
    }


    /**
     * 检查一个docid是否 已经被删除,
     * 并且这个docId对应的nid 已经有了对应生成的更大的docId
     * 这时才是真正无用的docId，可以从倒排链条里面清除掉
     *
     * @param docId
     *
     * @return  true:删除   false:未删除
     */
    inline bool isDocIdDelUseLess(uint32_t docId)
    {
        if (unlikely( docId > _blockPos ))   return true;

        if ( false == isDocIdDel( docId ) )  return  false;

        if ( _block[ docId ].next == COMMON_NULL )         // 已经被删除了， 查一下 有无更大的docId属于同一nid
            return false;

        int64_t nid = docAccessor->getInt64( docId , nidField );

        do                                                 // 检测 下一个docId 属于同一个nid, 往下循环找到同一个nid
        {
            docId = _block[ docId ].next;                  // 下一个docId

            if ( docAccessor->getInt64( docId , nidField ) == nid )
                return true;                               // 找到同一个nid了，传入的docId可清除

        } while ( _block[ docId ].next != COMMON_NULL );

        return false;
    };


    /**
     * 获取docId的总数（其实就是最大docId + 1）， 包括删除的docId
     *
     * @return
     */
    uint32_t  getDocIdCount()  {  return _blockPos;  };


    /**
     * 获取全量doc数
     *
     * @return
     */
    uint32_t  getFullDocNum();


    /**
     * 获取 有效  doc 的总数， 不包括删除的doc
     *
     * @return
     */
    uint32_t  getDocCount()  {  return _blockPos - _delCount - _failCount;  };



    /**
     * 获取被删除的docId总数（业务意义上的） ， 不包括内部因为更新失败而导致的伤处
     *
     * @return
     */
    uint32_t  getDelCount()    {   return _delCount; };



    /**
     * 获取deleteMap的对象实例
     *
     * @return  NULL:失败
     */
    DeleteMap * getDeleteMap()
    {
        if ( NULL == instanceDM )
        {
            instanceDM = new DeleteMap();
        }

        return instanceDM;
    };



    /**
     * 载入索引数据文件。  包括nid->docid的字典和 deletemap
     *
     * @param  filePath 持久化数据存放的路径
     *
     * @return  false: 载入失败;  true: 载入化成功
     */
    bool  load(const char * filePath);


    /**
     * 输出索引数据文件。  包括nid->docid的字典和 deletemap
     *
     * @param  filePath 持久化数据存放的路径
     *
     * @return  false: 持久化失败;  true: 持久化成功
     */
    bool  dump(const char * filePath);


    /**
     * 检查是否已经成功的加载字典
     *
     * @return true: 加载成功;  false: 未加载
     */
    bool  checkLoad() { return _loadFlag; }


    /**
     * 增加一次更新失败的累积
     */
    void addFail() { _failCount++; dumpFailDocNum(); };


    /**
     * 重置内部数据， 用于测试
     */
    void reset();

    /**
     * 清空持久化管理器中的待更新数据
     */
    void clearSyncInfo() { if (syncMgr) {syncMgr->reset();} }

    /**
     * 获取路径
     *
     * @return
     */
    const char * getPath();

private:
    // 构造函数、析构函数
    DocIdManager();
    virtual ~DocIdManager();


    /**
     * 分配相关资源
     *
     * @return
     */
    bool create();



    /**
     * 扩展字典表
     *
     * @return
     */
    bool blockExpand();


    /**
     * 扩展deletemap
     *
     * @return
     */
    bool delMapExpand();


    /**
     * 输出nid->docid的字典
     *
     * @param  filePath 持久化数据存放的路径
     *
     * @return  false: 持久化失败;  true: 持久化成功
     */
    bool  dumpDict(const char * filePath);


    /**
     * 输出deletemap
     *
     * @param  filePath 持久化数据存放的路径
     *
     * @return  false: 持久化失败;  true: 持久化成功
     */
    bool  dumpDelMap(const char * filePath);


    /**
     * 载入   nid->docid的字典
     *
     * @param  filePath 持久化数据存放的路径
     *
     * @return  false: 载入失败;  true: 载入化成功
     */
    bool  loadDict(const char * filePath);


    /**
     * 载入deletemap
     *
     * @param  filePath 持久化数据存放的路径
     *
     * @return  false: 载入失败;  true: 载入化成功
     */
    bool  loadDelMap(const char * filePath);


    /**
     * 销毁运行中分配的资源
     *
     * @return
     */
    bool destroy();


    /**
     * 加载全量doc数
     *
     * @return
     */
    uint32_t loadFullDocNum(const char * filePath);


    /**
     * 持久化全量doc数
     *
     * @return
     */
    void dumpFullDocNum(const char * filePath);


    /**
     * 加载更新失败doc数
     *
     * @return
     */
    uint32_t loadFailDocNum(const char * filePath);


    /**
     * 持久化更新失败doc数
     *
     * @return
     */
    void dumpFailDocNum();


private:
    static DocIdManager       *  instance;       // 类的惟一实例
    static DeleteMap          *  instanceDM;     //
    static ProfileManager     *  profileMgr;     // 属性管理器
    static ProfileField       *  nidField;       // 字段描述符
    static ProfileDocAccessor *  docAccessor;    // 读取工具类
    static DocIdSyncManager   *  syncMgr;        // 持久化工具类


private:
    uint64_t  *  _delMap;                        // delete map 内存数组
    uint64_t  *  _oldDelMap;                     // 旧的delMap数组，延时释放
    time_t       _delMapFreeTime;                // 旧的delMap数组可以延时释放的时间(当前时间+延时时间)

    uint32_t     _delMapSize;                    // delete map uint64单元个数
    uint32_t     _delCount;                      // delete map 被删除的总数
    uint32_t     _failCount;                     // 更新失败的总数， _delCount包括了 正常的需要删除的doc数
                                                 // 和因为更新失败导致删除的doc数, 2者在业务意义上是不用的.
                                                 // 需要区分开来

    uint32_t     _hashSize;                      // 用来取模的值, 一个质数
    uint32_t  *  _hashTab;

    uint32_t     _blockPos;                      // 当前可用的下一个节点位置, 也就是新的docId. 顺序递增 要加锁
    uint32_t     _blockSize;                     // 总字典节点的大小
    bool         _loadFlag;                      // 数据加载成功标记

    DD_NODE  *   _block;                         // 字典节点数组
    DD_NODE  *   _oldBlock;                      // 旧的节点数组，延迟释放
    time_t       _canFreeTime;                   // 旧的节点可以释放的时间(当前时间+延迟时间)

    char         _path[PATH_MAX];                // Docid Dict和Delmap存储的路径

    uint32_t     _fullDocNum;                    // 全量数据对应的doc数
    bool         _sync;                          // 增量数据持久化flag
};





}

#endif /* DOCIDMANAGER_H_ */

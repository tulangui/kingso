/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: MetaDataManager.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SHM_METADATAMANAGER_H_
#define SHM_METADATAMANAGER_H_

#include "ShareMemSegment.h"
#include "DelayFreeQueue.h"

typedef uint32_t u_int;

namespace index_mem
{

struct FixMemMeta
{
    bool  _isAlloc;  //内存状态
    int   _left;     //剩余空间
    int   _unit;     //定长大小
    u_int _level;    //所属内存段
};

struct MemPoolMeta
{
    //管理的内存区间，最小内存到最大内存，不包括头int
    int _maxmem;
    int _minmem;
    //目前所能分配的最大内存，超过部分直接malloc
    int _allocmem;
    //级数递增的比例
    float _rate;
    //目前最大的级数
    u_int _level;
    u_int _delaytime;
};

struct MemSpaceMeta
{
    int    _maxbits;
    u_int  _nowptr;
    int    _units[0];
};

class MetaDataManager
{ 
public:
    MetaDataManager():_size(0), _addr(NULL), _maxLevelNum(0), _queueSize(0),_maxbnum(0){}
    ~MetaDataManager(){}

    /**
     * @param  path       增量内存池对应目录
     * @param  maxlevel   最大level值
     * @param  qsize      延时释放队列的大小
     * @param  maxbnum    最大block个数
     *
     * @return            0,OK;-1,ERROR
     */
    int create(const char * path, int maxLevelNum, int qsize, int maxbnum);

    int loadData(const char * path);

    int loadMem(const char * path);

    int dump(const char * path);

    int detach();

    int destroy();

    struct MemPoolMeta    * getMemPoolMeta();
    struct FixMemMeta     * getFixMemMeta();
    struct queue_t        * getQueueAddress();
    struct MemSpaceMeta   * getMemSpaceMeta();
    u_int                 * getFreeListArray();

    size_t        getMetaSpaceSize();
    void        * getMetaBaseAddr();
    int           getMaxLevelNum();
    int           getMaxBlockNum();
    int           getQueueSize();

private:
    size_t _size;
    void * _addr;
    int    _maxLevelNum;
    int    _queueSize;
    int    _maxbnum;

    ShareMemSegment _shmSeg;
};

}

#endif //SHM_METADATAMANAGER_H_

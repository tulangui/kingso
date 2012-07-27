/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ShareMemPool.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: ShareMemPool $
 ********************************************************************/

#ifndef KINGSO_SHM_SHAREMEMPOOL_H_
#define KINGSO_SHM_SHAREMEMPOOL_H_

#include "ShareMemSegment.h"
#include "MetaDataManager.h"
#include "LogicMemSpace.h"
#include "DelayFreeQueue.h"
#include "Freelist.h"

typedef uint32_t u_int;

namespace index_mem
{

class FixMem
{
    LogicMemSpace     * _memSpace;
    struct FixMemMeta * _fixMeta;

public:
    int create(LogicMemSpace *memmg, struct FixMemMeta * fixMeta, int unit);
    int load(LogicMemSpace *memmg, struct FixMemMeta * fixMeta);
    int reset();
    u_int alloc();
};


/**
 * 基于共享内存的内存池结构
 */
class ShareMemPool
{ 
	static const int MAXLEVEL  = 20;
	static const int DEFMAXMEM = 20;//1<<20int
	static const int DEFMINMEM = 2;	//1<<2int

	LogicMemSpace   _memSpace;
    MetaDataManager _metaMgr;

	//内存管理池
	FixMem               _fixmem[MAXLEVEL];
	DelayFreeQueue       _delayqueue;
    struct MemPoolMeta * _poolMeta;
    u_int              * _freelstKeys;

public:

    ShareMemPool():_poolMeta(NULL),_freelstKeys(NULL) {}
    ~ShareMemPool() {}

	/**
	 * @brief 创建内存池
	 *
	 * @param [in] minmem   : int	最小的内存分配单元
	 * @param [in] maxmem   : int	管理最大的连续内存
	 * @param [in] rate   : float	根据最小单元每级递增的比例，不超过最大管理的连续内存
	 * 									最多10级
	 * @param [in] qsize	:	延迟释放队列大小
	 * @param [in] delaytime : 延迟释放时间
	 * @return  int 	成功返回0，失败－1
	**/
	int create(const char * path, int minmem=DEFMINMEM, int maxmem=DEFMAXMEM, float rate=2.0f, int qsize=10000, int delaytime=10);

    /**
     * @brief 从硬盘加载内存池
     *
     * @param [in] path    : const char * 内存池对应的本地存储路径 
     * @return  int     成功返回0，失败-1
     */
    int loadMem(const char * path);

    /**
     * @brief 从硬盘加载内存池
     *
     * @param [in] path    : const char * 内存池对应的本地存储路径 
     * @return  int     成功返回0，失败-1
     */
    int loadData(const char * path);

    /**
     * @brief 将内存池数据dump到硬盘
     *
     * @param [in] path   : const char * 内存池持久化的本地路径
     * @return  int     成功返回0，失败-1
     */
    int dump(const char * path);

	/**
	 * @brief detach share memory of mempool
	 *
	 * @return  void 
	**/
	void detach();

	/**
	 * @brief 删除mempool
	 *
	 * @return  void 
	**/
	void destroy();

	/**
	 * @brief 分配内存
	 *
	 * @param [in/out] size   : int 需要分配的内存大小
	 * @return  u_int 返回可用的内存指针，失败返回-1
	**/
	u_int alloc(int size);

	/**
	 * @brief 释放内存
	 *
	 * @return  void 
	**/
	void dealloc(u_int);

	void reset();

	int getidx(int size);

	inline int size(u_int key) {
		return _memSpace.size(key);
	}
	inline void * addr(u_int key) {
		return _memSpace.addr(key);
	}

    void freeDelayMem();
    void resetDelayMem();

private:
	void real_dealloc(u_int key);
};

}
#endif //KINGSO_SHM_SHAREMEMPOOL_H_

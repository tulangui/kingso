#include "ShareMemPool.h"
#include <time.h>

namespace index_mem
{

#define _debug(fmt, arg...) 
//	fprintf(stderr, "[%s:%d]"fmt"\n", __FILE__, __LINE__, ##arg);

u_int
FixMem::alloc()
{
    if (_fixMeta == NULL) {
        return (u_int)(-1);
    }

	_debug("FixMem left[%d] unit[%d] _level[%d]", _fixMeta->_left, _fixMeta->_unit, _fixMeta->_level);
	if (_fixMeta->_left < _fixMeta->_unit) {
		if (_memSpace->alloc(_fixMeta->_level, _fixMeta->_unit) == NULL) {
            _fixMeta->_isAlloc = false;
			return (u_int)(-1);
		}
        _fixMeta->_isAlloc = true;
		_fixMeta->_left    = _memSpace->_maxmemalloc;
	}
	_fixMeta->_left -= _fixMeta->_unit;
	return (((u_int)_fixMeta->_level) << (_memSpace->_meta->_maxbits)) + (u_int)(_fixMeta->_left);
}

int
FixMem::create(LogicMemSpace *memmg, struct FixMemMeta * fixMeta, int unit)
{
    if (memmg == NULL || fixMeta == NULL) {
        return -1;
    }

	_memSpace = memmg;
    _fixMeta  = fixMeta;

	_fixMeta->_isAlloc  = false;
	_fixMeta->_left     = 0;
	_fixMeta->_unit     = unit;
	_fixMeta->_level    = 0;
	return 0;
}

int
FixMem::load(LogicMemSpace *memmg, struct FixMemMeta * fixMeta)
{
    if (memmg == NULL || fixMeta == NULL) {
        return -1;
    }

	_memSpace = memmg;
    _fixMeta  = fixMeta;
	return 0;
}

int
FixMem::reset()
{
	_fixMeta->_isAlloc = false;
	_fixMeta->_left    = 0;
	_fixMeta->_level   = 0;
	return 0;
}

int
ShareMemPool::create(const char * path, int minmem, int maxmem, float rate, int qsize, int delaytime)
{
	if (rate <= 1.01f) {
		rate = 2.0f;
	}
	if (minmem < DEFMINMEM) {
		minmem = DEFMINMEM;
	}
	if (maxmem < DEFMAXMEM || maxmem >= 32) {
		maxmem = DEFMAXMEM;
	}
	if (minmem > maxmem) {
		_debug("min[%d] > max[%d]", minmem, maxmem);
		return -1;
	}
	if (qsize < 10000) {
		qsize = 10000;
	}
	if (delaytime < 0) {
		delaytime = 0;
	}

    int maxbnum = 1 << (32 - maxmem);
    if (_metaMgr.create(path, MAXLEVEL, qsize, maxbnum) == -1) {
        return -1;
    }

    struct MemSpaceMeta * spaceMeta   = _metaMgr.getMemSpaceMeta(); 
    struct FixMemMeta   * fixMemArray = _metaMgr.getFixMemMeta();  
    struct queue_t      * queueSpace  = _metaMgr.getQueueAddress();
    
    _poolMeta    = _metaMgr.getMemPoolMeta();
    _freelstKeys = _metaMgr.getFreeListArray();

	if (_memSpace.create(spaceMeta, path, maxmem) == -1) {
        return -1;
    }

	_poolMeta->_minmem   = 1<<minmem;
	_poolMeta->_maxmem   = 1<<maxmem;
	_poolMeta->_allocmem = 1<<maxmem;
	_poolMeta->_rate     = rate;
	_poolMeta->_level    = 0;


	int mem = _poolMeta->_minmem;
	for (int i=0; i<MAXLEVEL; ++i) {
		int rsize = mem;
		if (rsize <= _poolMeta->_maxmem) {
			if (_fixmem[i].create(&_memSpace, (fixMemArray + i), rsize) != 0) {
				_debug("create fimmem[%d] size[%d] err", i, rsize);
				return -1;
			}
			_debug("create bnum[%d] rsize[%d] level[%d] meminfo", _poolMeta->_maxmem/mem, rsize, _poolMeta->_level);
			++ _poolMeta->_level;
		} else {
			break;
		}
		_poolMeta->_allocmem = rsize;
		mem = (int)(mem * rate);
	}

	for (int i=0; i<MAXLEVEL; ++i) {
        _freelstKeys[i] = (u_int)(-1);
	}

	_debug("minmem=%d maxmem=%d _rate=%.2f level=%d", _poolMeta->_minmem, _poolMeta->_maxmem, _poolMeta->_rate, _poolMeta->_level);

	_poolMeta->_delaytime = delaytime;
	return _delayqueue.create(queueSpace, qsize);
}

void
ShareMemPool::destroy()
{
	_delayqueue.destroy();
	_memSpace.destroy();
    _metaMgr.destroy();
}

void
ShareMemPool::detach()
{
    _memSpace.detach();
    _metaMgr.detach();
}

void
ShareMemPool::reset()
{
	for (u_int i=0; i<_poolMeta->_level; ++i) {
		_fixmem[i].reset();
        _freelstKeys[i] = (u_int)(-1);
	}
    _delayqueue.clear();
	_memSpace.reset();
}

int
ShareMemPool::getidx(int rsize)
{
	int base = _poolMeta->_minmem;
	int ptr = 0;

	_debug("alloc base[%d] ptr[%d]", base, ptr);
	//根据create的计算这里的ptr不可能溢出
	while (rsize > base) {
		++ ptr;
		base = (int)(base * _poolMeta->_rate);
		_debug("alloc base[%d] ptr[%d]", base, ptr);
	}
	return ptr;
}

u_int
ShareMemPool::alloc(int size)
{
	int rsize = (size+3)/sizeof(int);

	_debug("alloc rsize[%d] allocmem[%d] base[%d]", rsize, _poolMeta->_allocmem, _poolMeta->_minmem);
	if (rsize > _poolMeta->_allocmem) {
		return INVALID_ADDR;
	}

	int ptr = getidx(rsize);

	if (_freelstKeys[ptr] != ((u_int)(-1))) {
		freelist_t *node = NULL;
		freelist_pop(&_memSpace, _freelstKeys[ptr], &node);
		return node->key;
	}
	return _fixmem[ptr].alloc();
}

void
ShareMemPool::dealloc(u_int key)
{
	delay_t node;
	u_int nowtime = (u_int)time(0);

	do {
		if (_delayqueue.pop_front(node) == 0) {
			if (node.reltime + _poolMeta->_delaytime <= nowtime) {
				real_dealloc(node.addres);
			} else {
				_delayqueue.push_front(node);
				break;
			}
		} else {
			break;
		}
	} while (1);

	node.addres  = key;
	node.reltime = nowtime;
	if (_delayqueue.push_back(node) != 0) {
		if (_delayqueue.pop_front(node) == 0) {
			real_dealloc(node.addres);
			node.addres = key;
			node.reltime = nowtime;
			_delayqueue.push_back(node);
		} else {
			real_dealloc(key);
		}
	}
}

void
ShareMemPool::real_dealloc(u_int key)
{
	freelist_t *node = (freelist_t *)_memSpace.addr(key);
	if (node != NULL) {
		node->key = key;
		int ptr = getidx(_memSpace.size(key));
		freelist_insert(&_memSpace, _freelstKeys[ptr], node);
	}
}


int
ShareMemPool::loadMem(const char * path)
{
    if (_metaMgr.loadMem(path) == -1) {
        return -1;
    }

    struct MemSpaceMeta * spaceMeta   = _metaMgr.getMemSpaceMeta(); 
    struct FixMemMeta   * fixMemArray = _metaMgr.getFixMemMeta();  
    struct queue_t      * queueSpace  = _metaMgr.getQueueAddress();
    
    _poolMeta    = _metaMgr.getMemPoolMeta();
    _freelstKeys = _metaMgr.getFreeListArray();

	if (_memSpace.loadMem(spaceMeta, path) == -1) {
        return -1;
    }

	for (int i=0; i<MAXLEVEL; ++i) {
        if (_fixmem[i].load(&_memSpace, (fixMemArray + i)) != 0) {
            return -1;
        }
	}

	_debug("minmem=%d maxmem=%d _rate=%.2f level=%d", _poolMeta->_minmem, _poolMeta->_maxmem, _poolMeta->_rate, _poolMeta->_level);
	if (_delayqueue.load(queueSpace) == -1) {
        return -1;
    }
    resetDelayMem();
    return 0;
}

int
ShareMemPool::loadData(const char * path)
{
    if (_metaMgr.loadData(path) == -1) {
        return -1;
    }

    struct MemSpaceMeta * spaceMeta   = _metaMgr.getMemSpaceMeta(); 
    struct FixMemMeta   * fixMemArray = _metaMgr.getFixMemMeta();  
    struct queue_t      * queueSpace  = _metaMgr.getQueueAddress();
    
    _poolMeta    = _metaMgr.getMemPoolMeta();
    _freelstKeys = _metaMgr.getFreeListArray();

	if (_memSpace.loadData(spaceMeta, path) == -1) {
        return -1;
    }

	for (int i=0; i<MAXLEVEL; ++i) {
        if (_fixmem[i].load(&_memSpace, (fixMemArray + i)) != 0) {
            return -1;
        }
	}

	_debug("minmem=%d maxmem=%d _rate=%.2f level=%d", _poolMeta->_minmem, _poolMeta->_maxmem, _poolMeta->_rate, _poolMeta->_level);
	if (_delayqueue.load(queueSpace) == -1) {
        return -1;
    }
    freeDelayMem();
    return 0;
}

int
ShareMemPool::dump(const char * path)
{
    if (path == NULL) {
        return -1;
    }

    if (_memSpace.dump(path) != 0 || _metaMgr.dump(path) != 0) {
        return -1;
    }
    return 0;
}

void 
ShareMemPool::freeDelayMem()
{
	delay_t node;
	do {
		if (_delayqueue.pop_front(node) == 0) {
            real_dealloc(node.addres);
		} else {
			break;
		}
	} while (1);
}

void 
ShareMemPool::resetDelayMem()
{
	delay_t node;
    _delayqueue.pop_back(node);     //将最近一个舍弃掉，不释放(防止一级目标token一级索引未持久化为新的偏移地址)
    freeDelayMem();
}

}

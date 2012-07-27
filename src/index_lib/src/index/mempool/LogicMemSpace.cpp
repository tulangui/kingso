#include "LogicMemSpace.h"

#include <stdio.h>
#include <stdlib.h>

namespace index_mem
{

#define _debug(fmt, arg...)  //fprintf(stderr, "[%s:%d]"fmt"\n", __FILE__, __LINE__, ##arg);
static const char * DUMPMEMFILE = "mempool.seg";

int
LogicMemSpace::create(struct MemSpaceMeta * meta, const char * path, u_int maxbits)
{
    if (meta == NULL || path == NULL) {
        return -1;
    }

    strcpy(_memPath, path);
    _meta = meta;

	if (maxbits < 20) return -1;
	_meta->_maxbits = maxbits;
	_maxmemalloc = (1 << maxbits) ;
	_maxbnum = 1 << (32 - maxbits);
	_lvlmask = (((u_int)(-1)) >> maxbits) << maxbits;
	_sizmask = ~_lvlmask;

	_mptr = (blockmg_t **)calloc(_maxbnum, sizeof(blockmg_t *));
	if (_mptr == NULL) {
		return -1;
	}

	_debug("alloc blockmg list[%d]", _maxbnum);
	_meta->_nowptr = 0;
	return 0;
}

int
LogicMemSpace::loadMem(struct MemSpaceMeta * meta, const char * path)
{
    if (meta == NULL || path == NULL) {
        return -1;
    }

    snprintf(_memPath, 512, "%s", path);
    _meta = meta;

	int maxbits = _meta->_maxbits;
	_maxmemalloc = (1 << maxbits) ;
	_maxbnum = 1 << (32 - maxbits);
	_lvlmask = (((u_int)(-1)) >> maxbits) << maxbits;
	_sizmask = ~_lvlmask;

	_mptr = (blockmg_t **)calloc(_maxbnum, sizeof(blockmg_t *));
	if (_mptr == NULL) {
		return -1;
	}

    for(u_int pos = 0; pos < _meta->_nowptr; pos++) {
		_mptr[pos] = (blockmg_t *)malloc(sizeof(blockmg_t));
		if (_mptr[pos] == NULL) {
			return -1;
		}

        ShareMemSegment * pSeg = new ShareMemSegment();
        if (pSeg == NULL || pSeg->loadShareMem(_memPath, pos + 1) != 0) {
            return -1;
        }

        if (pSeg->size() < (sizeof(int) * _maxmemalloc)) {
            return -1;
        }

        _mptr[pos]->seg  = pSeg;
        _mptr[pos]->unit = _meta->_units + pos;
        _mptr[pos]->data = (int *)pSeg->addr();
	}
    return 0;
}

int
LogicMemSpace::loadData(struct MemSpaceMeta * meta, const char * path)
{
    if (meta == NULL || path == NULL) {
        return -1;
    }

    snprintf(_memPath, 512, "%s", path);
    _meta = meta;

	int maxbits = _meta->_maxbits;
	_maxmemalloc = (1 << maxbits) ;
	_maxbnum = 1 << (32 - maxbits);
	_lvlmask = (((u_int)(-1)) >> maxbits) << maxbits;
	_sizmask = ~_lvlmask;

	_mptr = (blockmg_t **)calloc(_maxbnum, sizeof(blockmg_t *));
	if (_mptr == NULL) {
		return -1;
	}

    for(u_int pos = 0; pos < _meta->_nowptr; pos++) {
		_mptr[pos] = (blockmg_t *)malloc(sizeof(blockmg_t));
		if (_mptr[pos] == NULL) {
			return -1;
		}

        ShareMemSegment * pSeg = new ShareMemSegment();
        if (pSeg == NULL) {
            return -1;
        }

        if (pSeg->createShareMem(_memPath, pos + 1, sizeof(int)*_maxmemalloc) != 0) {
            pSeg->loadShareMem(_memPath, pos + 1);
            pSeg->freeShareMem();
            if (pSeg->createShareMem(_memPath, pos + 1, sizeof(int)*_maxmemalloc) != 0) {
                return -1;
            }
        }

        _mptr[pos]->seg  = pSeg;
        _mptr[pos]->unit = _meta->_units + pos;
        _mptr[pos]->data = (int *)pSeg->addr();

        char fileName[512];
        snprintf(fileName, 512, "%s/%s_%u", _memPath, DUMPMEMFILE, pos+1);

        FILE * fp = NULL;
        fp = fopen(fileName, "r");
        if (fp == NULL) {
            return -1;
        }

        if (fread(_mptr[pos]->data, sizeof(int), _maxmemalloc, fp) != _maxmemalloc) {
            fclose(fp);
            return -1;
        }
        fclose(fp);
	}
    return 0;
}

void *
LogicMemSpace::alloc(u_int &ptr, int unit)
{
    if (_meta == NULL || _mptr == NULL) {
        return NULL;
    }

	if (_mptr[_meta->_nowptr] == NULL) {
		if (_meta->_nowptr >= _maxbnum) {
			_debug("invalid ptr[%d] > maxbum[%d]", _meta->_nowptr, _maxbnum);
			return NULL;
		}
		_mptr[_meta->_nowptr] = (blockmg_t *)malloc(sizeof(blockmg_t));
		if (_mptr[_meta->_nowptr] == NULL) {
			return NULL;
		}

        ShareMemSegment * pSeg = new ShareMemSegment();
        if (pSeg == NULL)
        {
            return NULL;
        }

        if (pSeg->createShareMem(_memPath, _meta->_nowptr + 1, sizeof(int)*_maxmemalloc) != 0) {
            pSeg->loadShareMem(_memPath, _meta->_nowptr + 1);
            pSeg->freeShareMem();
            if (pSeg->createShareMem(_memPath, _meta->_nowptr + 1, sizeof(int)*_maxmemalloc) != 0) {
                return NULL;
            }
        }

        _mptr[_meta->_nowptr]->seg  = pSeg;
        _mptr[_meta->_nowptr]->unit = &(_meta->_units[_meta->_nowptr]);
        _mptr[_meta->_nowptr]->data = (int *)pSeg->addr();
	}
	ptr = (_meta->_nowptr)++;
	*(_mptr[ptr]->unit) = unit;
	return _mptr[ptr]->data;
}

void *
LogicMemSpace::addr(u_int key)
{
	if (key == INVALID_ADDR || _mptr == NULL || _meta == NULL) {
		_debug("invalid key[%x]", key);
		return NULL;
	}
	return _mptr[(key&_lvlmask)>>(_meta->_maxbits)]->data + (key&_sizmask);
}

int
LogicMemSpace::size(u_int key)
{
	if (key == INVALID_ADDR || _mptr == NULL || _meta == NULL) {
		_debug("invalid key[%x]", key);
		return 0;
	}
	return *(_mptr[(key&_lvlmask)>>(_meta->_maxbits)]->unit);
}

void
LogicMemSpace::destroy()
{
	for (u_int i=0; i<_maxbnum; ++i) {
        if (_mptr[i] != NULL) {
            ShareMemSegment * pSeg = _mptr[i]->seg;
            if (pSeg) {
                pSeg->freeShareMem();
                delete pSeg;
                _mptr[i]->seg = NULL;
            }
        }
		free (_mptr[i]);
	}
	free (_mptr);
	_mptr = NULL;
}

void
LogicMemSpace::detach()
{
	for (u_int i=0; i<_maxbnum; ++i) {
        if (_mptr[i] != NULL) {
            ShareMemSegment * pSeg = _mptr[i]->seg;
            if (pSeg) {
                pSeg->detachShareMem();
                delete pSeg;
                _mptr[i]->seg = NULL;
            }
        }
		free (_mptr[i]);
	}
	free (_mptr);
	_mptr = NULL;
}

void
LogicMemSpace::reset()
{
    destroy();
    create(_meta, _memPath, _meta->_maxbits);
}

int 
LogicMemSpace::dump(const char * path)
{
    if (_meta == NULL || _mptr == NULL) {
        return -1;
    }

    ShareMemSegment * pSeg = NULL;
    for (u_int pos = 0; pos < _meta->_nowptr; pos++) {
        char fileName[512];
        snprintf(fileName, 512, "%s/%s_%u", path, DUMPMEMFILE, pos+1);

        FILE * fp = NULL;
        fp = fopen(fileName, "w");
        if (fp == NULL) {
            return -1;
        }

        if (_mptr[pos] == NULL || _mptr[pos]->seg == NULL) {
            fclose(fp);
            return -1;
        }

        pSeg = _mptr[pos]->seg;
        if (fwrite(pSeg->addr(), sizeof(int), _maxmemalloc, fp) != _maxmemalloc) {
            fclose(fp);
            return -1;
        }
        fclose(fp);
    }
    return 0;
}

}
/* vim: set ts=4 sw=4 sts=4 tw=100 */

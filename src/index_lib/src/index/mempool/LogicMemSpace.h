#ifndef  __MEMMG_H_
#define  __MEMMG_H_

#include <stdint.h>
#include "MetaDataManager.h"
#include "ShareMemSegment.h"

typedef uint32_t u_int;

//#include <ul_def.h>

static const u_int INVALID_ADDR = (u_int)(-1);

namespace index_mem
{

    struct blockmg_t
    {
        int             * unit;
        ShareMemSegment * seg;
        int             * data;
    };

	class LogicMemSpace
	{
		public:
			u_int _maxmemalloc;
			u_int _maxbnum;
			u_int _lvlmask;
			u_int _sizmask;

			blockmg_t           **_mptr;
            struct MemSpaceMeta * _meta;
            char  _memPath[512];

		public:
            LogicMemSpace():_maxmemalloc(0),_maxbnum(0),_lvlmask(0),_sizmask(0),_mptr(NULL),_meta(NULL) {}


			int create(struct MemSpaceMeta * meta, const char * path, u_int maxbits = 20);
            int loadMem(struct MemSpaceMeta * meta, const char * path);
            int loadData(struct MemSpaceMeta * meta, const char * path);
            int dump(const char * path);

			void *alloc(u_int &ptr, int unit);
			void *addr(u_int key);
			/*返回key对应的内存空间的大小单位是sizeof(int), 因此如果要buffer长度需要*sizeof(int)*/
			int size(u_int key);

            void detach();
			void destroy();
			void reset();
	};
};
#endif  //__MEMMG_H_

/* vim: set ts=4 sw=4 sts=4 tw=100 */

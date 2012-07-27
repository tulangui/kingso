#ifndef _BITMAP_INDEX_FILTER_H_
#define _BITMAP_INDEX_FILTER_H_

#include "index_lib/DocIdManager.h"
#include "FilterInterface.h"

using namespace index_lib;

namespace search {

class BitmapIndexFilter : public FilterInterface
{
public:
    
    BitmapIndexFilter(const uint64_t *p_bitmap, const bool ret)
    {
        _p_bitmap = p_bitmap; 
        _ret      = ret;
    }

    virtual ~BitmapIndexFilter()
    {
    }

    /**
     * bitmap过滤处理函数
     * @param doc_id docid值
     * @return true（不过滤），false（过滤）
     */
    bool process(uint32_t doc_id)
	{
		return (_p_bitmap[doc_id / 64] & bit_mask_tab[doc_id % 64])  
			   ? !_ret 
			   : _ret;
	}

private:
	const uint64_t *_p_bitmap;        /* bitmap指针 */
    bool            _ret;             /* 非逻辑过滤 */

};

}

#endif

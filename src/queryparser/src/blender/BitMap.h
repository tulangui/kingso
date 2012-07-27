/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: BitMap.h 2577 2011-03-09 01:50:55Z yanhui.tk $
 *
 * $Brief: bitmap结构 每一个bit保存一个整数的存在或不存在 $
 ********************************************************************/

#ifndef BLENDER_BITMAP_H_
#define BLENDER_BITMAP_H_

#include "util/common.h"
#include <stdint.h>

namespace queryparser{

    class BitMap
    { 
        public:
            BitMap();

            ~BitMap();

            int32_t init(int32_t byte_count = 2);

            bool find(const char *item, int32_t len);
            bool find(int64_t item);

            int32_t add(char *item, int32_t len);
            int32_t add(int64_t item);

        private:
            char    *_bits;
            int32_t  _byte_count;
    };

    inline BitMap::BitMap()
        : _bits(NULL), _byte_count(0)
    {

    }

    inline BitMap::~BitMap()
    {
        if(_bits)
        {
            delete [] _bits;
            _bits = NULL;
            _byte_count = 0;
        }
    }

    inline int32_t BitMap::init(int32_t byte_count)
    {
        if(byte_count <= 0)
        {
            return -1;
        }
        _byte_count = byte_count;
        int32_t item_count = (1<<(byte_count*8))/8;
        _bits = new char [item_count];
        if(unlikely(_bits == NULL))
        {
            TERR("alloc memory for bitmap failed.");
            return -1;
        }
        bzero(_bits, item_count);
        return 0;
    }

    inline bool BitMap::find(const char *item, int32_t len)
    {
        char tp_word[4] = {0, 0, 0, 0};
        wchar_t ws_word[4] = {0, 0, 0, 0};

        if(len > _byte_count + 1) //???!!!
        {
            return false;
        }
        memcpy(tp_word, item, len);
        mbstowcs(ws_word, tp_word, 3);
        if(ws_word[1]!=0)
        {
            return false;
        }
        return find(int32_t(ws_word[0]));
    }

    inline bool BitMap::find(int64_t item)
    { 	
        if(unlikely(item < 0 || item >= (1<<(_byte_count*8))))
        {
            return false;
        }
        int32_t pos = item / 8;
        int32_t offset = item % 8;

        return (_bits[pos] & (1<<offset));
    }
    
    inline int32_t BitMap::add(char *item, int32_t len)
    {
		return add(strtol(item, NULL, 10));
    }

    inline int32_t BitMap::add(int64_t item)
    {
        if(unlikely(item < 0 || item >= (1<<(_byte_count*8))))
        {
            return -1;
        }
        int32_t pos = item / 8;
        int32_t offset = item % 8;

        _bits[pos] |= (1 << offset);
        
        return 0;
    }
}

#endif

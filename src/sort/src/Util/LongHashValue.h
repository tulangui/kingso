#ifndef __UTIL_LONG_HASH_VALUE_H
#define __UTIL_LONG_HASH_VALUE_H

//#include <tr1/memory>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <assert.h>

#include "sort/common/Common.h"
#include "util/Log.h"


template<int count = 2>
class LongHashValue
{
public:
 
    LongHashValue(uint64_t integer)
    {
        for(int i = 0; i < count - 1 ; i++)
        {
            value[i] = 0;
        }
        
        value[count - 1] = integer;
    }
   
    LongHashValue()
    {
        for(int i = 0; i < count ; i++)
        {
            value[i] = 0;
        }
    }

    int Size()
    {
        return sizeof(value);
    }

    int Count()
    {
        return count;
    }

    bool operator ==(const LongHashValue<count>& other) const
    {
        for(int i = 0; i < count; i++)
        {
            if(other.value[i] != value[i])
            {
                return false;
            }
        }
        return true;
    }
    
    bool operator !=(const LongHashValue<count>& other) const
    {
        for(int i = 0; i < count; i++)
        {
            if(other.value[i] != value[i])
            {
                return true;
            }
        }
        return false;
    }

    bool operator < (const LongHashValue<count>& other) const
    {
        for(int i = 0; i < count; i++)
        {
            if(other.value[i] < value[i])
            {
                return true;
            }
        }
        return false;
    }
    bool operator > (const LongHashValue<count>& other) const {
        return other < *this;
    }

    void Serialize(std::string& str) const
    {
        str.resize(sizeof(uint64_t) * count, '\0');
        
        uint64_t tmpPrimaryKey ;
        int len = sizeof(tmpPrimaryKey);
        for(int seg = 0; seg < count; seg++)
        {
            tmpPrimaryKey = value[seg];
            for (int i = seg * len + len - 1; i >= seg * len; --i) 
            {
                str[i] = (char)(tmpPrimaryKey & 0xFF);
                tmpPrimaryKey >>= 8;
            }
        }
    }

    void Deserialize(const std::string& str)
    {
        assert(str.length() == count * sizeof(uint64_t));
        int len = sizeof(uint64_t);
        uint64_t tmpPrimaryKey;
        for(int seg = 0; seg < count; seg++)
        {
            tmpPrimaryKey = (uint64_t)0;
            for(int i = seg * len; i < (seg+1) * len; i++)
            {
                tmpPrimaryKey <<= 8;
                tmpPrimaryKey |= (unsigned char)str[i];
                
            }
            value[seg] = tmpPrimaryKey;
        }
    }

    std::string ToString()
    {
        std::stringstream ss;
        ss << *this;
        return ss.str();
    }

public:
    uint64_t value[count];
};

typedef LongHashValue<2> uint128_t;
typedef LongHashValue<4> uint256_t;

std::ostream& operator <<(std::ostream& stream, uint128_t v);
std::ostream& operator <<(std::ostream& stream, uint256_t v);


#endif //__UTILLIB_LONG_HASH_VALUE_H

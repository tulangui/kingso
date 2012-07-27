#include <assert.h>
#include <string.h>

inline uint32_t securityCopyTo(char *&dst,uint32_t &len1,
        const void *src, const uint32_t len2)
{
    if(NULL != dst) {
        assert(src && (len1 >= len2));
        memcpy(dst,src,len2);
        dst += len2;
        len1 -= len2;
    }
    return len2;
};

inline uint32_t securityCopyFrom(void *dst,const uint32_t len1,
        char *&src, uint32_t &len2)
{
    if(NULL != src) {
        assert(dst && (len2 >= len1));
        memcpy( dst,src,len1);
        src += len1;
        len2 -= len1;
    }
    return len1;
};


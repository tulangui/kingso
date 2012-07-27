#include "compress.h"
#include "util/Log.h"

uLong compressSize(uLong sourceLen)
{
    return (sizeof(uLong)*2+sourceLen+(sourceLen>1280?sourceLen/10:128));
}

uLong uncompressSize(const Bytef* source, uLong sourceLen)
{
    if(!source || sourceLen<=sizeof(uLong)*2){
        return 0;
    }
    else{
        return ((uLong*)source)[1];
    }
}

int zcompress(Bytef* dest, uLong* destLen, const Bytef* source, uLong sourceLen)
{
    uLong realLen = 0;
    Bytef* realBuf = 0;
    if(!dest || !destLen || *destLen<=sizeof(uLong)*2 || !source || sourceLen<=0){
        TWARN("zcompress argument error, return!");
        return -1;
    }
    realLen = *destLen-sizeof(uLong)*2;
    realBuf = dest+sizeof(uLong)*2;
    if(compress2(realBuf, &realLen, source, sourceLen, Z_BEST_SPEED)){
        TWARN("compress2 erorr, return![realLen=%lu][sourceLen=%lu]", realLen, sourceLen);
        return -1;
    }
    else{
        ((uLong*)dest)[0] = realLen;
        ((uLong*)dest)[1] = sourceLen;
        *destLen = realLen+sizeof(uLong)*2;
        return 0;
    }
}

int zdecompress(Bytef* dest, uLong* destLen, const Bytef* source, uLong sourceLen)
{
    uLong realLen = 0;
    const Bytef* realDat = 0;
    if(!dest || !destLen || !source || sourceLen<=sizeof(uLong)*2){
        TWARN("zdecompress argument error, return!");
        return -1;
    }
    if(*destLen<((uLong*)source)[1]){
        TWARN("destLen=%lu less than uncompressed data len=%lu", *destLen, ((uLong*)source)[1]);
        return -1;
    }
    realLen = *((uLong*)source);
    realDat = source+sizeof(uLong)*2;
    if(uncompress(dest, destLen, realDat, realLen)){
        TWARN("uncompress error, return![destLen=%lu][sourceLen=%lu]", *destLen, realLen);
        return -1;
    }
    if(*destLen!=((uLong*)source)[1]){
        TWARN("destLen=%lu not match uncompressed data len=%lu", *destLen, ((uLong*)source)[1]);
        return -1;
    }
    return 0;
}

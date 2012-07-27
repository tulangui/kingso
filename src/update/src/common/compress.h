/*********************************************************************
 * $Author: pianqian.gc $
 *
 * $LastChangedBy: pianqian.gc $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: compress.h 2577 2011-03-09 01:50:55Z pianqian.gc $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef _COMPRESS_H_
#define _COMPRESS_H_

#include <zlib.h>

#ifdef __cplusplus
extern "C" {
#endif

uLong compressSize(uLong sourceLen);

uLong uncompressSize(const Bytef* source, uLong sourceLen);

int zcompress(Bytef* dest, uLong* destLen, const Bytef* source, uLong sourceLen);

int zdecompress(Bytef* dest, uLong* destLen, const Bytef* source, uLong sourceLen);

#ifdef __cplusplus
}
#endif

#endif //_COMPRESS_H_

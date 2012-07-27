#include "framework/Compressor.h"
#include "util/compress.h"
#include "util/errcode.h"
#include <stdlib.h>
#include <zlib.h>
#include <string.h>
#include <new>

FRAMEWORK_BEGIN;

/* RFC 1952 Section 2.3 defines the gzip header:
 *
 * +---+---+---+---+---+---+---+---+---+---+
 * |ID1|ID2|CM |FLG|     MTIME     |XFL|OS |
 * +---+---+---+---+---+---+---+---+---+---+
 */
static const char gzip_header[10] =
{ '\037', '\213', Z_DEFLATED, 0,
    0, 0, 0, 0, /* mtime */
    0, 0x03 /* Unix OS_CODE */
};

ZCompressor CompressorFactory::g_zc;
GZCompressor CompressorFactory::g_gzc;
HTTPGZCompressor CompressorFactory::g_httpgzc;

void * Compressor::allocMemory(uint32_t size) {
    if (size == 0) {
        return NULL;
    }
    if (_heap) {
        return NEW_VEC(_heap, char, size);
    }
    return malloc(size);
}

void Compressor::freeMemory(void * ptr) {
        if (ptr && !_heap) {
            ::free(ptr);
        }
}

int32_t ZCompressor::compress(const char *uncomprData, uint32_t uncomprSize,
		char * &comprData, uint32_t &comprSize) {
	int32_t val = 0;
	if (!uncomprData) {
		uncomprSize = 0;
	}
	if (uncomprSize == 0) {
		comprData = (char *)allocMemory(sizeof(uint32_t) * 2);
		if (unlikely(!comprData)) {
            return KS_ENOMEM;
        }
		*(uint32_t*)comprData = 0;
		*(uint32_t*)(comprData + sizeof(uint32_t)) = 0;
		comprSize = sizeof(uint32_t) * 2;
		return KS_SUCCESS;
	}
	val = uncomprSize + (uncomprSize > 1280 ? uncomprSize / 10 : 128);
	comprData = (char *)allocMemory(sizeof(uint32_t) * 2 + uncomprSize + 128);
	if (unlikely(!comprData)) {
        return KS_ENOMEM;
    }
	if (compress_data((char*)uncomprData, uncomprSize, 
				comprData + sizeof(uint32_t) * 2, &val) != 0) 
    {
		freeMemory(comprData);
		return KS_EFAILED;
	}
	comprSize = val + sizeof(uint32_t) * 2;
	*(uint32_t*)comprData = val;
	*(uint32_t*)(comprData + sizeof(uint32_t)) = uncomprSize;
	return KS_SUCCESS;
}

int32_t ZCompressor::uncompress(const char * comprData, uint32_t comprSize,
		char * & uncomprData, uint32_t & uncomprSize) {
	int32_t val = 0;
	if (!comprData || comprSize < sizeof(uint32_t) * 2) {
        return KS_EFAILED;
    }
	if (comprSize != *(uint32_t*)comprData + sizeof(uint32_t) * 2) {
		return KS_EFAILED;
	}
	uncomprSize = *(uint32_t*)(comprData + sizeof(uint32_t));
	if(uncomprSize == 0) {
		if(comprSize > sizeof(uint32_t)) {
			return KS_EFAILED;
		}
		uncomprData = NULL;
		uncomprSize = 0;
		return KS_SUCCESS;
	}
	uncomprData = (char *)allocMemory(uncomprSize + 1);
	if (unlikely(!uncomprData)) {
        return KS_ENOMEM;
    }
	val = uncomprSize + 1;
	if (uncompress_data((char*)comprData + sizeof(uint32_t) * 2, 
				comprSize - sizeof(uint32_t) * 2,
				uncomprData, &val) != 0) 
    {
		freeMemory(uncomprData);
		return KS_EFAILED;
	}
	if ((uint32_t)val != uncomprSize) {
		freeMemory(uncomprData);
		return KS_EFAILED;
	}
    uncomprData[uncomprSize] = '\0';
	return KS_SUCCESS;
}

int32_t GZCompressor::compress(const char *uncomprData, uint32_t uncomprSize,
                char * &comprData, uint32_t &comprSize) {
        z_stream c_stream;
        char outBuffer[DEFLATE_CHUNK];
        int32_t ret = 0;
        int32_t have = 0;
        int32_t offset = 0;
        c_stream.zalloc = Z_NULL;
        c_stream.zfree = Z_NULL;
        c_stream.opaque = Z_NULL;
        ret = deflateInit2(&c_stream, -1, Z_DEFLATED, -15, 9, Z_DEFAULT_STRATEGY);
        if (ret != Z_OK) {
            return KS_EFAILED;
        }
        c_stream.avail_in = uncomprSize;
        c_stream.next_in = (Bytef *)uncomprData;
        do {
                c_stream.avail_out = DEFLATE_CHUNK;
                c_stream.next_out = (Bytef *)outBuffer;
                ret = deflate(&c_stream, Z_FINISH);
                if (ret == Z_STREAM_ERROR) {
                        deflateEnd(&c_stream);
                        return KS_EFAILED;
                }
                have = DEFLATE_CHUNK - c_stream.avail_out;
                if (comprSize < offset + have) {
                        deflateEnd(&c_stream);
                        return KS_EFAILED;
                }
                memcpy(comprData + offset, outBuffer, have);
                offset += have;
        } while (c_stream.avail_out == 0);
        if (c_stream.avail_in != 0) {
            deflateEnd(&c_stream);
            return KS_EFAILED;
        }
        comprSize = offset;
        deflateEnd(&c_stream);
        return KS_SUCCESS;
}

int32_t GZCompressor::uncompress(const char *comprData, uint32_t comprSize,
                char * &uncomprData, uint32_t &uncomprSize) {
        int err = 0;
        z_stream dStream = {0}; /* decompression stream */
        static char dummyHead[2] =
        {
                0x8 + 0x7 * 0x10,
                (((0x8 + 0x7 * 0x10) * 0x100 + 30) / 31 * 31) & 0xFF,
        };
        dStream.zalloc = (alloc_func)0;
        dStream.zfree = (free_func)0;
        dStream.opaque = (voidpf)0;
        dStream.next_in  = (Bytef *)comprData;
        dStream.avail_in = 0;
        dStream.next_out = (Bytef *)uncomprData;
        if (inflateInit2(&dStream, -MAX_WBITS) != Z_OK) {
            return KS_EFAILED;
        }
        //if(inflateInit2(&dStream, 47) != Z_OK) return -1;
        while (dStream.total_out < uncomprSize && 
                dStream.total_in < comprSize) 
        {
                dStream.avail_in = dStream.avail_out = 1; /* force small buffers */
                if ((err = inflate(&dStream, Z_NO_FLUSH)) == Z_STREAM_END) {
                    break;
                }
                if (err != Z_OK )
                {
                        if (err == Z_DATA_ERROR)
                        {
                                dStream.next_in = (Bytef*) dummyHead;
                                dStream.avail_in = sizeof(dummyHead);
                                if ((err = inflate(&dStream, Z_NO_FLUSH)) != Z_OK)
                                {
                                        return KS_EFAILED;
                                }
                        }
                        else {
                            return KS_EFAILED;
                        }
                }
        }
        if (inflateEnd(&dStream) != Z_OK) {
            return KS_EFAILED;
        }
        uncomprSize = dStream.total_out;
        return KS_SUCCESS;
}

int32_t HTTPGZCompressor::compress(const char *uncomprData, 
        uint32_t uncomprSize,
        char * &comprData, 
        uint32_t &comprSize) 
{
    int icur = 0;
    uint32_t outlen = 0;
    if (uncomprSize == 0) {
        comprData = (char *)allocMemory(sizeof(uint32_t) * 2);
        if (unlikely(!comprData)) {
            return KS_ENOMEM;
        }
        *(uint32_t*)comprData = 0;
        *(uint32_t*)(comprData + sizeof(uint32_t)) = 0;
        comprSize = sizeof(uint32_t) * 2;
        return KS_SUCCESS;
    }
    comprData = (char *)allocMemory(sizeof(uint32_t) * 2 + uncomprSize + 128);
    if (unlikely(!comprData)) {
        return KS_ENOMEM;
    }
    memcpy(comprData + icur, gzip_header, sizeof(gzip_header));
    icur += sizeof(gzip_header);
    outlen = comprSize - icur;
    char* gzip_body = comprData + icur;
    if (GZCompressor::compress(uncomprData, uncomprSize, gzip_body, outlen) 
            != KS_SUCCESS)
    {
        return KS_EFAILED;
    }
    icur += outlen;
    int crc = 0;
    crc = crc32(crc, (const Bytef *)uncomprData, uncomprSize);
    *(unsigned int *)(comprData + icur) = crc;
    icur += 4;
    *(unsigned int *)(comprData + icur) = uncomprSize;
    icur += 4;
    comprSize = icur;
    return KS_SUCCESS;
}


Compressor * CompressorFactory::make(compress_type_t type, MemPool *heap)
{
    if (type == ct_zlib) {
        if (!heap) {
            return &g_zc;
        }
        return new (std::nothrow) ZCompressor(heap);
    }
    if (type == ct_gzip) {
        if (!heap) {
            return &g_gzc;
        }
        return new (std::nothrow) GZCompressor(heap);
    }
    if (type == ct_httpgzip) {
        if (!heap) {
            return &g_httpgzc;
        }
        return new (std::nothrow) HTTPGZCompressor(heap);
    }
    return NULL;
}

void CompressorFactory::recycle(Compressor * p) {
    if (p && p != &g_zc && p != &g_gzc && p != &g_httpgzc) {
        delete p;
    }
}

FRAMEWORK_END;


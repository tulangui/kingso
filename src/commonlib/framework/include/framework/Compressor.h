/** \file
 *******************************************************************
 * Author: taiyi.zjh
 *
 * $LastChangedBy: taiyi.zjh $
 *
 * $Revision: 167 $
 *
 * $LastChangedDate: 2011-03-25 11:13:57 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: Compressor.h 167 2011-03-25 03:13:57Z taiyi.zjh $
 *
 * $Brief: command which control server status (start stop restart)$
 *
 *******************************************************************
 */
#ifndef _COMPRESSOR_H_
#define _COMPRESSOR_H_
#include "util/common.h"
#include "util/MemPool.h"
#include "util/namespace.h"
#include "framework/namespace.h"

FRAMEWORK_BEGIN;

//压缩类型
typedef enum {
    ct_zlib,
    ct_gzip,
    ct_httpgzip,
    ct_none
} compress_type_t;

class Compressor {
    public:
        /**
         * @brief Compressor constructor function
         * @param heap memory pool which used in compress
         * @return 
         */
        Compressor(MemPool *heap = NULL) : _heap(heap) { }
        /**
         * @brief Compressor destructor function
         * @param
         * @return
         */
        virtual ~Compressor() { }
    public:
        /**
         * @brief allocate memory function
         * @param size allocate size
         * @return pointer to the memory
         */
        void * allocMemory(uint32_t size);
        /**
         * @brief free memory function
         * @param ptr pointer of the memory
         * @return 
         */
        void freeMemory(void *ptr);
        /**
         * @brief compress string
         * @param uncomprData source string
         * @param uncomprSize source string size
         * @param comprData destination string
         * @param comprSize destination string size
         * @return destination string size
         */
        virtual int32_t compress(const char *uncomprData, uint32_t uncomprSize,
                char * &comprData, uint32_t &comprSize) = 0;
        /**
         * @brief uncompress string
         * @param comprData destination string
         * @param comprSize destination string size
         * @param uncomprData source string
         * @param uncomprSize source string size
         * @return destination string size
         */
        virtual int32_t uncompress(const char *comprData, uint32_t comprSize,
                char * &uncomprData, uint32_t &uncomprSize) = 0;
    private:
        MemPool *_heap;
};

class ZCompressor : public Compressor {
    public:
        ZCompressor(MemPool *heap = NULL):Compressor(heap) { }
        ~ZCompressor() { }
    public:
        virtual int32_t compress(const char *uncomprData, uint32_t uncomprSize,
                char * &comprData, uint32_t &comprSize);
        virtual int32_t uncompress(const char *comprData, uint32_t comprSize,
                char * &uncomprData, uint32_t &uncomprSize);
};

class GZCompressor : public Compressor {
    public:
        GZCompressor(MemPool *heap = NULL):Compressor(heap) { }
        ~GZCompressor() { }
    public:
        virtual int32_t compress(const char *uncomprData, uint32_t uncomprSize,
                char * &comprData, uint32_t &comprSize);
        virtual int32_t uncompress(const char *comprData, uint32_t comprSize,
                char * &uncomprData, uint32_t &uncomprSize);
};

class HTTPGZCompressor : public GZCompressor {
    public:
        HTTPGZCompressor(MemPool *heap = NULL):GZCompressor(heap) { }
        ~HTTPGZCompressor() { }
    public:
        virtual int32_t compress(const char *uncomprData, uint32_t uncomprSize,
                char * &comprData, uint32_t &comprSize);
        virtual int32_t uncompress(const char *comprData, uint32_t comprSize,
                char * &uncomprData, uint32_t &uncomprSize) { }
};


class CompressorFactory {
    public:
        /**
         * @brief CompressorFactory constructor function
         * @param
         * @return
         */
        CompressorFactory() { }
        /**
         * @brief CompressorFactory destructor function
         * @param
         * @return
         */
        ~CompressorFactory() { }
    public:
        /**
         * @brief Generate Compressor object
         * @param type Compressor type
         * @param heap Mempool
         * @return Compressor object
         */
        static Compressor * make(compress_type_t type, MemPool *heap=NULL);
        /**
         * @brief recycle Compressor object
         * @param p pointer to Compressor object
         * @return
         */
        static void recycle(Compressor * p);
    private:
        static ZCompressor g_zc;
        static GZCompressor g_gzc;
        static HTTPGZCompressor g_httpgzc;
};

FRAMEWORK_END;

#endif //_FRAMEWORK_COMPRESSOR_H_


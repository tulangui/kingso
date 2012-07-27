#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include "Common.h"
#include "IndexFieldInc.h"
#include "IndexTermActualize.h"

#include "util/MD5.h"
#include "util/idx_sign.h"

#include "index_lib/IndexLib.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/IndexIncManager.h"

#include "mempool/ShareMemPool.h"

INDEX_LIB_BEGIN

index_mem::ShareMemPool g_mempool;

IndexIncManager IndexIncManager::_indexIncInstance;

IndexIncManager::IndexIncManager()
{
    _initFlag       = false;
    _sync           = false;
    _maxIncNum      = 0; 
}

IndexIncManager::~IndexIncManager()
{
}

IndexTerm * IndexIncManager::getTerm(MemPool *pMemPool, const char * fieldName, const char * term)
{
    if (NULL == fieldName || NULL == term) {
        return NULL;
    }

    uint64_t termSign;
    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));

    if (fieldSign == _nidSign) {
        const char* pre = term;
        char* cur = NULL;
        termSign = (uint64_t)strtoll(pre, &cur, 10);
    } else {
        termSign = idx_sign64(term, strlen(term));
    }
    return getTerm(pMemPool, fieldSign, termSign);
}

IndexTerm * IndexIncManager::getTerm(MemPool *pMemPool, uint64_t fieldNameSign, uint64_t termSign)
{
    if (fieldNameSign == _nidSign) {
        IndexTermInfo* pTermInfo = NEW(pMemPool, IndexTermInfo);
        if(NULL == pTermInfo) {
            return NULL;
        }

        DocIdManager* pDocIdx = DocIdManager::getInstance();
        uint32_t doc = pDocIdx->getDocId(termSign);
        if (doc < _FullDocNum || doc == INVALID_DOCID) {
            pTermInfo->docNum = 0;
            return NEW(pMemPool, IndexTerm);
        } else {
            uint32_t* pDocList = NEW_ARRAY(pMemPool, uint32_t, 2);
            IndexUnZipNullOccTerm* pTerm = NEW(pMemPool, IndexUnZipNullOccTerm);
            if (NULL == pTerm || NULL == pDocList) {
                return NULL;
            }
            pDocList[0] = doc;
            pDocList[1] = INVALID_DOCID;
            pTerm->setInvertList((char*)pDocList);
            if (pTerm->init(1, pDocIdx->getDocIdCount(), 0) < 0) {
                return NULL;
            }
            return pTerm;
        }
    }

    // find at fieldMap
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldNameSign);
    if (_fieldMap.end() == it) {
        return NULL;
    }
    IndexFieldInc* pReader = (IndexFieldInc*)it->value;
    IndexTerm* pTerm = pReader->getTerm(pMemPool, termSign);
    if(NULL == pTerm) {
        return NEW(pMemPool, IndexTerm);
    }
    return pTerm;
}

int IndexIncManager::addTerm(const char * fieldName, uint64_t termSign, uint32_t docId, uint8_t firstOcc)
{
    if (NULL == fieldName) return -1;

    uint64_t  fieldSign = idx_sign64( fieldName, strlen(fieldName) );

    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find( fieldSign );

    if (_fieldMap.end() == it) return -1;

    IndexFieldInc * pReader = (IndexFieldInc *) it->value;

    return pReader->addTerm(termSign, docId, firstOcc);
}

bool IndexIncManager::hasField(const char * fieldName)
{
    if (NULL == fieldName) {
        return false;
    }
    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    if (_nidSign == fieldSign) {
        return true;
    }

    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if (_fieldMap.end() == it) {
        return false;
    }
    return true;
}

int IndexIncManager::dump(bool flag)
{
    void* value = NULL;
    uint64_t key = 0;
    IndexFieldInc* pInc = NULL;

    uint32_t success = 0;
    _fieldMap.itReset();
    for(uint32_t i = 0; i < _fieldMap.size(); i++) {
        if(false == _fieldMap.itNext(key, value)) {
            continue;
        }
        pInc = (IndexFieldInc*)value;
        if(pInc->dump(flag) >= 0) {
            success++;
        }
    }
    if(success == _fieldMap.size()) {
        return 0;
    }
    return -1;
}

int IndexIncManager::open(const char * path)
{
    if (NULL == path || strlen(path) >= PATH_MAX) {
        return KS_EINVAL;
    }
    strcpy(_idxPath, path);

    char fileName[PATH_MAX];
    snprintf(fileName, PATH_MAX, "%s/%s", _idxPath, INDEX_GLOBAL_INFO);

    struct stat st;
    if (stat(fileName, &st) || st.st_size <= 0) {
        return KS_ENOENT;
    }
    char* pGlobalInfoBuf = new (std::nothrow) char[st.st_size + 1];
    if (NULL == pGlobalInfoBuf) {
        return KS_ENOMEM;
    }

    FILE* fp = fopen(fileName, "rb");
    if (NULL == fp || fread(pGlobalInfoBuf, st.st_size, 1, fp) != 1) {
        if (fp) fclose(fp);
        delete[] pGlobalInfoBuf;
        return KS_EIO;
    }
    fclose(fp);
    pGlobalInfoBuf[st.st_size] = 0;

    // 验证索引是否完整
    unsigned char md5Sign[16];
    util::MD5_CTX context;
    util::MD5Init(&context);
    util::MD5Update(&context, (const unsigned char*)pGlobalInfoBuf, st.st_size - 16);
    util::MD5Final(md5Sign, &context);
    if (memcmp(md5Sign, pGlobalInfoBuf + st.st_size - 16, 16)) {
        delete[] pGlobalInfoBuf;
        return KS_ENOENT;
    }

    char MemPoolName[PATH_MAX];
    snprintf(MemPoolName, PATH_MAX, "%s/%s", _idxPath, INDEX_MEMPOOL_DIR);

    DIR * dir = opendir(MemPoolName);
    if (dir != NULL) {
        closedir(dir);
    }
    else {
        // create dir
        mkdir(MemPoolName, 0755);
    }

    // 如果配置为非持久化方式，清除老内存池数据
    if ( !_sync ) {
        index_mem::ShareMemPool old_mempool;
        if ( old_mempool.loadMem(MemPoolName) == 0 ) {
            old_mempool.destroy();
            TLOG("sync flag is false, release old share memory!");
        }
    }

    bool inc_load = false;
    if (g_mempool.loadMem(MemPoolName) == 0) {
        inc_load = true;
        TLOG("load mempool from share memory ok!");
    }
    else if(g_mempool.loadData(MemPoolName) == 0) {
        inc_load = true;
        TLOG("load mempool from disk ok!");
    }
    else if(g_mempool.create(MemPoolName, 7, 24, 2.0f, 10000, 20) == 0) {
        TLOG("create mempool ok");
    }
    else {
        TERR("create mempool failed");
        return KS_ENOENT;
    }

    char* p = pGlobalInfoBuf;
    int32_t fieldNum = *(int32_t*)p;   // fieldNum
    p += sizeof(int32_t);

    _maxDocNum = *(int32_t*)p;        // maxDocNum
    p += sizeof(int32_t);
  	_fieldNum = fieldNum;

    int32_t maxIncSkipCnt     = MAX_INC_SKIP_CNT;
    int32_t maxIncBitmapSize  = MAX_INC_BITMAP_SIZE;
    if(_maxIncNum > 0) {
        maxIncSkipCnt     = ((_maxIncNum + (1<<20) - 1) >> 20) << 4;
        maxIncBitmapSize  = (((_maxIncNum + (1<<20) - 1) >> 20) << 17) - 8;
    }
    TLOG("incskipcnt=%d, bitmapsize=%d, maxdocNum=%d", maxIncSkipCnt, maxIncBitmapSize, maxIncBitmapSize*8);

	// 需要保证docidManager必须保证先起来
    DocIdManager* pDocIdx = DocIdManager::getInstance();
    _FullDocNum = pDocIdx->getFullDocNum();
    for (int32_t i = 0; i < fieldNum; i++) {
        char* fieldName = p;
        while (*p) {
            p++;
        }
        uint64_t fieldSign = idx_sign64(fieldName, p - fieldName);
        p++;      // 0x0
        uint32_t maxOccNum = *(int32_t*)p;  // maxOccNum
        p += sizeof(int32_t);

        IndexFieldInc* pField = new (std::nothrow) IndexFieldInc(maxOccNum, _FullDocNum, g_mempool);
        if (NULL == pField) {
            delete[] pGlobalInfoBuf;
            return KS_ENOMEM;
        }
        if (strlen(fieldName) >= MAX_FIELD_NAME_LEN) {
            delete[] pGlobalInfoBuf;
            return KS_EFAILED;
        }
        if (pField->open(path, fieldName, inc_load, _sync, maxIncSkipCnt, maxIncBitmapSize)) {
            delete[] pGlobalInfoBuf;
            return KS_EFAILED;
        }
        void* value = (void*)pField;
        if(false == _fieldMap.insert(fieldSign, value)) {
            delete[] pGlobalInfoBuf;
            return KS_EFAILED;
        }
    }

    delete[] pGlobalInfoBuf;
    _nidSign = idx_sign64(NID_FIELD_NAME, strlen(NID_FIELD_NAME));
    _initFlag = true;

    return 0;
}


int IndexIncManager::close()
{
    void* value = NULL;
    uint64_t key = 0;
    IndexFieldInc* pInc = NULL;

    char MemPoolName[PATH_MAX];
    snprintf(MemPoolName, PATH_MAX, "%s/%s", _idxPath, INDEX_MEMPOOL_DIR);

    // 导出文本索引(索引对比)
    if(dump(_export) < 0) {
        TERR("dump failed");
        return -1;
    }

    if ( _sync ) {
        g_mempool.detach();
    } 
    else {
        g_mempool.destroy();
    }

    _fieldMap.itReset();
    for(uint32_t i = 0; i < _fieldMap.size(); i++) {
        if(false == _fieldMap.itNext(key, value)) {
            return -1;
        }
        pInc = (IndexFieldInc*)value;
        pInc->close();
        delete pInc;
    }
    _fieldMap.clear();

    return 0;
}



/**
 * 遍历字段的所有term签名
 *
 * @param fieldSign   字段名称
 * @param termSign    返回的当前term的签名
 *
 * @return 0: 成功 ；   -1： 失败
 */
int IndexIncManager::getFirstTerm(uint64_t fieldSign, uint64_t &termSign)
{
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find( fieldSign );

    if ( it == _fieldMap.end() )  return -1;

    IndexFieldInc * pInc = (IndexFieldInc *)it->value;

    return pInc->getFirstTerm( termSign );
}

int IndexIncManager::getNextTerm(uint64_t fieldSign, uint64_t &termSign)
{
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find( fieldSign );

    if ( it == _fieldMap.end() )  return -1;

    IndexFieldInc * pInc = (IndexFieldInc *)it->value;

    return pInc->getNextTerm( termSign );
}

int IndexIncManager::getFirstTerm(const char * fieldName, uint64_t &termSign)
{
    if (unlikely( NULL == fieldName )) return -1;

    uint64_t fieldSign = idx_sign64( fieldName, strlen(fieldName) );

    return getFirstTerm( fieldSign, termSign );
}

int IndexIncManager::getNextTerm(const char * fieldName, uint64_t &termSign)
{
    if (unlikely( NULL == fieldName )) return -1;

    uint64_t fieldSign = idx_sign64( fieldName, strlen(fieldName) );

    return getNextTerm( fieldSign, termSign );
}


INDEX_LIB_END


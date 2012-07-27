#include <sys/stat.h>

#include "Common.h"
#include "IndexFieldReader.h"
#include "IndexTermActualize.h"

#include "util/MD5.h"
#include "util/errcode.h"
#include "util/idx_sign.h"
#include "index_lib/IndexReader.h"
#include "index_lib/DocIdManager.h"
#include "index_lib/IndexInfoManager.h"


INDEX_LIB_BEGIN


IndexReader IndexReader::_indexReaderInstance;

IndexReader::IndexReader()
{
    _nidSign = 0;
}

IndexReader::~IndexReader()  { }

int IndexReader::open(const char * path, bool bPreLoad)
{
    if ( NULL == path || strlen(path) >= PATH_MAX)  return KS_EINVAL;

    IndexInfoManager info;
    if ( info.load( path ) < 0 )
    {
        TERR( "open index info file failed:%s", path );
        return KS_ENOENT;
    }

    if ( (_maxDocNum = info.getDocNum()) < 0)  return KS_EFAILED;
    strcpy( _path, path );

    uint32_t           pos       = 0;
    uint32_t           maxOccNum = 0;
    char             * fieldName = NULL;
    IndexFieldReader * pReader   = NULL;
    uint64_t           fieldSign = 0;

    while ( info.getNextField( pos, fieldName, maxOccNum ) >= 0 )
    {
        fieldSign = idx_sign64( fieldName, strlen(fieldName) );
        pReader   = new IndexFieldReader( maxOccNum, _maxDocNum );

        if ( NULL == pReader ) return KS_ENOMEM;
        if ( 0 != pReader->open( path, fieldName, bPreLoad ) )
        {
            TERR( "open index failed, path:%s, field:%s", path, fieldName );
            return KS_EFAILED;
        }

        if ( false == _fieldMap.insert( fieldSign, (void *)pReader) )
            return KS_EFAILED;
    }

    _nidSign = idx_sign64( NID_FIELD_NAME, strlen(NID_FIELD_NAME) );

    DocIdManager::getInstance();                 // 需要保证docidManager必须保证先起来
    return 0;
}



void IndexReader::close()
{
    uint64_t key = 0;
    void* value = NULL;
    IndexFieldReader* pReader = NULL;

    _fieldMap.itReset();
    for(uint32_t i = 0; i < _fieldMap.size(); i++) {
        if(false == _fieldMap.itNext(key, value)) {
            continue;
        }
        pReader = (IndexFieldReader*)value;
        pReader->close();
        delete pReader;
    }

    _fieldMap.clear();
}




/**
 * 取得高频词的截断索引
 *
 * @param pMemPool
 * @param fieldName       倒排字段名
 * @param term            原始的term
 * @param psFieldName     ps排序字段的名字
 * @param sortType        排序类型， 0：正排   1:倒排
 * @return
 */
    IndexTerm *
IndexReader::getTerm(MemPool     * pMemPool,
        const char  * fieldName,
        const char  * term,
        const char  * psFieldName,
        uint32_t      sortType)
{
    if (unlikely( NULL == pMemPool ))     return NULL;
    if (unlikely( NULL == fieldName ))    return NULL;
    if (unlikely( NULL == term ))         return NULL;
    if (unlikely( NULL == psFieldName ))  return NULL;

    uint64_t  termSign  = 0;
    uint64_t  fieldSign = idx_sign64( fieldName, strlen(fieldName) );

    if ( fieldSign == _nidSign )
    {
        termSign = strtoull( term, NULL, 10 );
    }
    else
    {
        termSign = HFterm_sign64( psFieldName, sortType, term );
    }

    return getTerm( pMemPool, fieldSign, termSign );
}




IndexTerm * IndexReader::getTerm(MemPool* pMemPool, const char * fieldName, const char * term)
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

IndexTerm * IndexReader::getTerm(MemPool* pMemPool, uint64_t fieldNameSign, uint64_t termSign)
{
    if (fieldNameSign == _nidSign) {
        IndexTermInfo* pTermInfo = NEW(pMemPool, IndexTermInfo);
        if(NULL == pTermInfo) {
            return NULL;
        }

        DocIdManager* pDocIdx = DocIdManager::getInstance();
        uint32_t doc = pDocIdx->getDocId(termSign);
        if (doc >= _maxDocNum) {
            pTermInfo->docNum = 0;
            IndexTerm* pTerm = NEW(pMemPool, IndexTerm);
            if(NULL == pTerm) {
                return NULL;
            }
            return pTerm;
        } else {
            pTermInfo->docNum = 1;
            uint32_t* pDocList = NEW_ARRAY(pMemPool, uint32_t, 2);
            IndexUnZipNullOccTerm* pTerm = NEW(pMemPool, IndexUnZipNullOccTerm);
            if (NULL == pTerm || NULL == pDocList) {
                return NULL;
            }
            pDocList[0] = doc;
            pDocList[1] = INVALID_DOCID;
            pTerm->setInvertList((char*)pDocList);
            if (pTerm->init(1, _maxDocNum , 0) < 0) {
                return NULL;
            }
            return pTerm;
        }
    }

    // false find at fieldMap
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldNameSign);
    if (_fieldMap.end() == it) {
        return NULL;
    }
    IndexFieldReader* pReader = (IndexFieldReader*)it->value;
    IndexTerm* pTerm = pReader->getTerm(pMemPool, termSign);
    if(NULL == pTerm) {
        return NEW(pMemPool, IndexTerm);
    }
    return pTerm;
}

bool IndexReader::hasField(const char * fieldName)
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

bool IndexReader::hasField(uint64_t fieldNameSign)
{
    if (_nidSign == fieldNameSign) {
        return true;
    }

    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldNameSign);
    if (_fieldMap.end() == it) {
        return false;
    }
    return true;
}

int IndexReader::getIndexInfo(char* info, int maxlen)
{
    uint64_t key = 0;
    void* value = NULL;
    IndexFieldReader* pReader = NULL;

    int len = maxlen;
    _fieldMap.itReset();
    char* p = info;

    for(uint32_t i = 0; i < _fieldMap.size(); i++) {
        _fieldMap.itNext(key, value);
        pReader = (IndexFieldReader*)value;
        int ret = pReader->getIndexInfo(p, len);
        if(ret < 0) {
            return -1;
        }
        len -= ret;
        p += ret;
    }

    return p - info;
}


/**
 * 遍历字段的所有term签名
 *
 * @param fieldSign   字段名称
 * @param termSign    返回的当前term的签名
 *
 * @return 0: 成功 ；   -1： 失败
 */
int IndexReader::getFirstTerm(uint64_t fieldSign, uint64_t &termSign)
{
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find( fieldSign );

    if ( it == _fieldMap.end() )  return -1;

    IndexFieldReader * pReader = (IndexFieldReader *)it->value;

    return pReader->getFirstTerm( termSign );
}

int IndexReader::getNextTerm(uint64_t fieldSign, uint64_t &termSign)
{
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find( fieldSign );

    if ( it == _fieldMap.end() )  return -1;

    IndexFieldReader * pReader = (IndexFieldReader *)it->value;

    return pReader->getNextTerm( termSign );
}

int IndexReader::getFirstTerm(const char * fieldName, uint64_t &termSign)
{
    if (unlikely( NULL == fieldName )) return -1;

    uint64_t fieldSign = idx_sign64( fieldName, strlen(fieldName) );

    return getFirstTerm( fieldSign, termSign );
}

int IndexReader::getNextTerm(const char * fieldName, uint64_t &termSign)
{
    if (unlikely( NULL == fieldName )) return -1;

    uint64_t fieldSign = idx_sign64( fieldName, strlen(fieldName) );

    return getNextTerm( fieldSign, termSign );
}

INDEX_LIB_END


#include <string>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Common.h"
#include "IndexFieldBuilder.h"
#include "index_struct.h"
#include "index_lib/IndexInfoManager.h"
#include "index_lib/IndexBuilder.h"

#include "util/MD5.h"
#include "util/errcode.h"
#include "util/idx_sign.h"

INDEX_LIB_BEGIN


IndexBuilder IndexBuilder::_indexBuilderInstance;

IndexBuilder::IndexBuilder(int32_t maxDocNum)
{
    _openFlag = false;
    _maxDocNum = maxDocNum;
}
IndexBuilder::IndexBuilder()
{
    _openFlag = false;
    _maxDocNum = 0;
}


IndexBuilder::~IndexBuilder()
{
    void* value = NULL;
    uint64_t key = 0;
    IndexFieldBuilder* pBuilder = NULL;

    _fieldMap.itReset();
    for(uint32_t i = 0; i < _fieldMap.size(); i++) {
        if(false == _fieldMap.itNext(key, value)) {
            continue;
        }
        pBuilder = (IndexFieldBuilder*)value;
        delete pBuilder;
    }
}


int IndexBuilder::open( const char * path )
{
    if ( NULL == path || strlen(path) >= PATH_MAX )
        return -1;

    void      * value = NULL;
    uint64_t    key   = 0;

    _openFlag = true;
    strcpy(_idxPath, path);

    _fieldMap.itReset();
    for ( uint32_t i = 0; i < _fieldMap.size(); i++ )
    {
        if ( false == _fieldMap.itNext(key, value) )
            return -1;

        IndexFieldBuilder * pBdr = (IndexFieldBuilder *)value;

        if ( pBdr->open( path ) < 0 )
            return -1;
    }

    return 0;
}

int IndexBuilder::reopen( const char * path )
{
    if ( NULL == path || strlen(path) >= PATH_MAX)  return -1;

    IndexInfoManager info;
    if ( info.load( path ) < 0 )
    {
        TERR( "open index info file failed:%s", path );
        return -1;
    }

    if ( (_maxDocNum = info.getDocNum()) < 0 ) return -1;
    strcpy(_idxPath, path);

    uint32_t   pos       = 0;
    uint32_t   maxOccNum = 0;
    char     * fieldName = NULL;

    while ( info.getNextField( pos, fieldName, maxOccNum ) >= 0 )
    {
        if ( addField( fieldName, maxOccNum ) < 0)
        {
            TERR( "open index failed, path:%s, field:%s", path, fieldName );
            return KS_ENOENT;
        }
    }

    void* value = NULL;
    uint64_t key = 0;
    IndexFieldBuilder* pBuilder = NULL;

    _openFlag = true;
    _fieldMap.itReset();
    for(uint32_t i = 0; i < _fieldMap.size(); i++) {
        if(false == _fieldMap.itNext(key, value)) {
            return -1;
        }
        pBuilder = (IndexFieldBuilder*)value;
        if(pBuilder->reopen(path) < 0) {
            return -1;
        }
    }
    return 0;
}

int IndexBuilder::close()
{
    void* value = NULL;
    uint64_t key = 0;
    IndexFieldBuilder* pBuilder = NULL;

    IndexInfoManager info;
    _fieldMap.itReset();

    for(uint32_t i = 0; i < _fieldMap.size(); i++) {
        if(false == _fieldMap.itNext(key, value)) {
            return -1;
        }
        pBuilder = (IndexFieldBuilder*)value;
        pBuilder->close();

        if(info.addField(pBuilder->getFieldName(), pBuilder->getMaxOccNum()) < 0) {
            return -1;
        }
    }
    // 设置当前doc数量
    info.setDocNum(_maxDocNum);
    info.save(_idxPath);

    return 0;
}

int IndexBuilder::addField(const char * fieldName, int maxOccNum)
{
    uint64_t fieldSign = 0;
    int ret = checkField(fieldName, fieldSign);
    if (ret < 0) {
        return ret;
    }

    IndexFieldBuilder* pFieldBuilder = new IndexFieldBuilder(fieldName, maxOccNum, _maxDocNum, NULL);
    if (NULL == pFieldBuilder) {
        return -4;
    }
    void* value = (void*)pFieldBuilder;
    if (_fieldMap.insert(fieldSign, value) == false) {
        return -5;
    }
    return 0;
}

int IndexBuilder::addField(const char * fieldName, int maxOccNum, const char * encodeFile)
{
    uint64_t fieldSign = 0;
    int ret = checkField(fieldName, fieldSign);
    if (ret < 0) {
        return ret;
    }
    if (access(encodeFile, R_OK)) {
        return -6;
    }

    IndexFieldBuilder* pFieldBuilder = new IndexFieldBuilder(fieldName, maxOccNum, _maxDocNum, encodeFile);
    if (NULL == pFieldBuilder) {
        return -4;
    }
    void* value = (void*)pFieldBuilder;
    if (_fieldMap.insert(fieldSign, value) == false) {
        return -5;
    }
    return 0;
}

int IndexBuilder::addTerm(const char * fieldName, const char * line, int len)
{
    if (NULL == fieldName) {
        return KS_EINVAL;
    }

    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if(it == _fieldMap.end()) {
        return KS_ENOENT;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)it->value;
    return pBuilder->addTerm(line, len);
}

int IndexBuilder::addTerm(const char * fieldName, const uint64_t termSign,
        const uint32_t* docList, uint32_t docNum)
{
    if (NULL == fieldName) {
        return KS_EINVAL;
    }

    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if(it == _fieldMap.end()) {
        return KS_ENOENT;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)it->value;
    return pBuilder->addTerm(termSign, docList, docNum);
}

int IndexBuilder::addTerm(const char * fieldName, const uint64_t termSign,
        const DocListUnit* docList, uint32_t docNum)
{
    if (NULL == fieldName) {
        return KS_EINVAL;
    }

    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if(it == _fieldMap.end()) {
        return KS_ENOENT;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)it->value;
    return pBuilder->addTerm(termSign, docList, docNum);
}

int IndexBuilder::dump()
{
    void* value = NULL;
    uint64_t key = 0;
    IndexFieldBuilder* pBuilder = NULL;

    _fieldMap.itReset();
    for (uint32_t i = 0; i < _fieldMap.size(); i++) {
        if (false == _fieldMap.itNext(key, value)) {
            return -1;
        }
        pBuilder = (IndexFieldBuilder*)value;
        if (pBuilder->dump() < 0) {
            return -1;
        }
    }
    return 0;
}

int IndexBuilder::dumpField(const char * fieldName)
{
    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if(it == _fieldMap.end()) {
        return KS_ENOENT;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)it->value;
    return pBuilder->dump();
}

int IndexBuilder::checkField(const char* fieldName, uint64_t& fieldSign)
{
    if (true == _openFlag) {
        return -1;
    }
    int fieldLen = strlen(fieldName);
    if (fieldLen >= MAX_FIELD_NAME_LEN) {
        return -1;
    }
    fieldSign = idx_sign64(fieldName, (unsigned int )fieldLen);
    if (_fieldMap.find(fieldSign) != _fieldMap.end()) {
        return -2;
    }
    if (_fieldMap.size() >= MAX_INDEX_FIELD_NUM) {
        return -3;
    }
    return 0;
}

const char* IndexBuilder::getNextField(int32_t& travelPos, int32_t& maxOccNum)
{
    uint64_t key = 0;
    void* value = NULL;

    if(travelPos < 0) {
        travelPos = 0;
        _fieldMap.itReset();
    }

    if(false == _fieldMap.itNext(key, value)) {
        return NULL;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)value;
    maxOccNum = pBuilder->getMaxOccNum();
    return pBuilder->getFieldName();
}


const IndexTermInfo* IndexBuilder::getFirstTermInfo(const char* fieldName, uint64_t & termSign)
{
    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if(it == _fieldMap.end()) {
        return NULL;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)it->value;
    return pBuilder->getFirstTerm(termSign);
}

const IndexTermInfo* IndexBuilder::getNextTermInfo(const char* fieldName, uint64_t & termSign)
{
    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if(it == _fieldMap.end()) {
        return NULL;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)it->value;
    return pBuilder->getNextTerm(termSign);
}

IndexTerm * IndexBuilder::getTerm(const char* fieldName, uint64_t & termSign)
{
    uint64_t fieldSign = idx_sign64(fieldName, strlen(fieldName));
    util::HashMap<uint64_t, void*>::iterator it = _fieldMap.find(fieldSign);
    if(it == _fieldMap.end()) {
        return NULL;
    }

    IndexFieldBuilder* pBuilder = (IndexFieldBuilder*)it->value;
    return pBuilder->getTerm(termSign);
}

INDEX_LIB_END



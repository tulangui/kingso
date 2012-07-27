#include "framework/Query.h"
#include <ctype.h>

FRAMEWORK_BEGIN;

Query::Query() 
    : _origData(NULL), 
    _origSize(0),
    _pureData(NULL), 
    _pureSize(0),
    _subClusters(NULL),
    _compr(ct_none), 
    _useCache(true),
    _nCount(-1),
    _nStart(0),
    _uniqField(NULL),
    _uniqInfo(NULL),
    _separate(false),
    _searchlists(false)
{ }

Query::~Query() 
{
    clear();
}

void Query::clear() 
{
    if (_origData) {
        ::free(_origData);
        _origData = NULL;
    }
    _origSize = 0;
    if (_pureData) {
        ::free(_pureData);
        _pureData = NULL;
    }
    _pureSize = 0;
    if (_subClusters) {
        ::free(_subClusters);
        _subClusters = NULL;
    }
    if(_uniqField){
        ::free(_uniqField);
        _uniqField = NULL;
    }
    if(_uniqInfo){
        ::free(_uniqInfo);
        _uniqInfo = NULL;
    } 
    _compr = ct_none;
}

int32_t Query::setOrigQuery(const char *data, uint32_t size) 
{
    clear();
    if (!data) {
        size = 0;
    }
    _origData = (char*)malloc(size + 1);
    if (unlikely(!_origData)) {
        return ENOMEM;
    }
    if (size > 0) {
        memcpy(_origData, data, size);
    }
    _origData[size] = '\0';
    _origSize = size;
    return KS_SUCCESS;
}

int32_t Query::resetOrigQuery(const char *data, uint32_t size)
{
    char *origData;
    if (!data) {
        size = 0;
    }
    origData = (char *)malloc(size + 1);
    if (unlikely(!origData)) {
        return ENOMEM;
    }
    if (size > 0) {
        memcpy(origData, data, size);
    }
    origData[size] = '\0';
    if (_origData) {
        ::free(_origData);
    }
    _origData = origData;
    _origSize = size;
    return KS_SUCCESS;
}

char * Query::getParam(const char *q, uint32_t size, 
        const char *name) 
{
    uint32_t nameLen = 0;
    uint32_t len = 0;
    uint32_t i = 0;
    uint32_t end = 0;
    char *out = NULL;
    if (!name || !q) {
        return NULL;
    }
    nameLen = strlen(name);
    if (nameLen == 0) {
        return NULL;
    }
    if (size == 0) {
        return NULL;
    }
    for(i = 0; i < size; i++) {
        if (q[i] == '?') {
            break;
        }
    }
    if (i == size) {
        i = 0;
    }
    else {
        i++;  //skip '?'
    }
    while (true) {
        if (size < i + nameLen + 1) {
            break;
        }
        if (strncasecmp(q+i, name, nameLen) == 0 &&
                q[i + nameLen] == '=')
        {
            len = 0;
            while(i + nameLen + 1 + len < size) {
                if ((q[i + nameLen + 1 + len] == '&' 
                        || q[i + nameLen + 1 + len] == '|')
                        && q[i + nameLen + len] != '\\') 
                {
                    break;
                }
                len++;
            }
            if (len > 0) {
                out = (char*)malloc(len + 1);
                if (unlikely(!out)) {
                    return NULL;
                }
                memcpy(out, q + i + nameLen + 1, len);
                out[len] = '\0';
            }
            return out;
        }
        for (; i < size; i++) {
            if (q[i] == '&') {
                break;
            }
        }
        i++; //skip '&'
    }
    return NULL;
}

int32_t Query::getParam(char *q, uint32_t &size, 
        const char *name, 
        char * &out) 
{
    uint32_t nameLen = 0;
    uint32_t len = 0;
    uint32_t i = 0;
    uint32_t end = 0;
    if (!name || !q) {
        return KS_EFAILED;
    }
    nameLen = strlen(name);
    if (nameLen == 0) {
        return KS_EFAILED;
    }
    out = NULL;
    if (size == 0) {
        return KS_SUCCESS;
    }
    for (i = 0; i < size; i++) {
        if (q[i] == '?') {
            break;
        }
    }
    if (i == size) {
        i = 0;
    }
    else {
        i++;  //skip '?'
    }
    while (true) {
        if (size < i + nameLen + 1) {
            break;
        }
        if (strncasecmp(q+i, name, nameLen) == 0 &&
                q[i + nameLen] == '=')
        {
            len = 0;
            while (i + nameLen + 1 + len < size) {
                if ((q[i+nameLen+1+len] == '&' || q[i+nameLen+1+len] == '|') 
                        && q[i + nameLen + len] != '\\') 
                {
                    break;
                }
                len++;
            }
            if (len > 0) {
                out = (char*)malloc(len + 1);
                if (unlikely(!out)) {
                    return ENOMEM;
                }
                memcpy(out, q + i + nameLen + 1, len);
                out[len] = '\0';
            }
            end = i + nameLen + 1 + len;
            if (end + 1 < size) {
                if ((i==0) || (*(q+i-1) == '?')) {
                    memmove(q + i, q + end + 1, size - end - 1);
                }
                else {
                    memmove(q + i - 1, q + end, size - end);
                }
                size -= nameLen + len + 2;
            } 
            else {
                size = i;
                while (size > 0 && q[size - 1] == '&') {
                    size--;
                }
            }
            q[size] = '\0';
            return KS_SUCCESS;
        }
        for (; i < size; i++) {
            if (q[i] == '&') {
                break;
            }
        }
        i++; //skip '&'
    }
    return KS_SUCCESS;
}

int32_t Query::getStatisticParam(char *q, uint32_t &size, 
        const char *name, 
        char * &out) {
    uint32_t namelen = 0;
    uint32_t len = 0;
    uint32_t i = 0;
    uint32_t end = 0;
    if (!name || !q) {
        return KS_EFAILED;
    }
    namelen = strlen(name);
    if (namelen == 0) {
        return KS_EFAILED;
    }
    out = NULL;
    if (size == 0) {
        return KS_EFAILED;
    }
    for (i = 0; i < size; i++) {
        if(q[i] == '?') break;
    }
    if (i == size) {
        i = 0;
    }
    else {
        i++;  //skip '?'
    }
    while (true) {
        if (size < i + namelen + 1) break;
        if (strncasecmp(q+i, name, namelen) == 0 &&
                q[i + namelen] == '=')
        {
            len = 0;
            while(i + namelen + 1 + len < size) {
                if(q[i + namelen + 1 + len] == '&' && 
                        q[i + namelen + len] != '\\') break;
                len++;
            }
            if(len > 0) {
                out = (char*)malloc(len + 1);
                if(unlikely(!out)) return KS_ENOMEM;
                memcpy(out, q + i + namelen + 1, len);
                out[len] = '\0';
            }
            end = i + namelen + 1 + len;
            if(end + 1 < size) {
                if(*(q+i-1) == '?')  
                    memmove(q + i, q + end + 1, size - end - 1);
                else
                    memmove(q + i - 1, q + end, size - end);
                size -= namelen + len + 2;
            } else {
                size = i;
                while(size > 0 && q[size - 1] == '&') size--;
            }
            q[size] = '\0';
            return KS_SUCCESS;
        }
        for(; i < size; i++) {
            if(q[i] == '&') break;
        }
        i++; //skip '&'
    }
    return KS_SUCCESS;
}

int32_t Query::purge(const char *prefix) 
{
    char *buf = NULL;
    char *p = NULL;
    uint32_t size = 0;
    uint32_t i = 0;
    uint32_t start = 0;
    char prev = '\0';
    int32_t ret = 0;
    if (!_origData || _origSize == 0) {
        return KS_EFAILED;
    }
    buf = (char *)malloc(_origSize + 1);
    if (unlikely(!buf)) {
        return ENOMEM;
    }
    i = 0;
    //remove all space of string head
    while (i < _origSize && isspace(_origData[i])) {
        i++;
    }
    for (size = 0; i < _origSize; i++) {
        if (_origData[i] == '/' && prev == '/') {
            continue;
        }
        prev = _origData[i];
        buf[size++] = prev;
    }
    //remove all space of string tail
    while (size > 0 && isspace(buf[size - 1])) {
        size--;
    }
    if (size == 0) {
        ::free(buf);
        return KS_EFAILED;
    }
    start = 0;
    if (prefix) {
        start = strlen(prefix);
        if (start > 0 
                && (start>size || memcmp(buf, prefix, start) != 0)) 
        {
            ::free(buf);
            return KS_EFAILED;
        }
        size -= start;
    }
    ret = getParam(buf + start, size, "subclusters", _subClusters);
    if (ret != KS_SUCCESS) {
        ::free(buf);
        return ret;
    }
    ret = getParam(buf + start, size, "separate", p);
    if (ret != KS_SUCCESS) {
        if (_subClusters) {
            ::free(_subClusters);
            _subClusters = NULL;
        }
        ::free(buf);
        return ret;
    }
    if (p) {
        _separate = false;
        if (strcmp(p, "true") == 0 || strcmp(p, "yes") == 0) {
            _separate = true;
        }
        ::free(p);
    }
    ret = getParam(buf + start, size, "searcherlists", p);
    if (ret != KS_SUCCESS) {
        if (_subClusters) {
            ::free(_subClusters);
            _subClusters = NULL;
        }
        ::free(buf);
        return ret;
    }
    if (p) {
        _searchlists = false;
        if (strcmp(p, "true") == 0 || strcmp(p, "yes") == 0) {
            _searchlists = true;
        }
        ::free(p);
    }
    ret = getParam(buf + start, size, "compr", p);
    if (ret != KS_SUCCESS) {
        if (_subClusters) {
            ::free(_subClusters);
            _subClusters = NULL;
        }
        ::free(buf);
        return ret;
    }
    _compr = ct_none;
    if (p) {
        if (strcmp(p, "httpgzip") == 0) {
            _compr = ct_httpgzip;
        }
        ::free(p);
    }
    ret = getParam(buf + start, size, "useCache", p);
    if (ret != KS_SUCCESS) {
        if (_subClusters) {
            ::free(_subClusters);
            _subClusters = NULL;
        }
        return ret;
    }
    _useCache = true;
    if (p) {
        if (strcmp(p, "false") == 0 || strcmp(p, "no") == 0) {
            _useCache = false;
        }
        ::free(p);
    }
    _pureData = (char*)malloc(size + 1);
    if (unlikely(!_pureData)) {
        if (_subClusters) {
            ::free(_subClusters);
            _subClusters = NULL;
        }
        ::free(buf);
        return ENOMEM;
    }
    if (size > 0) memcpy(_pureData, buf + start, size);
    _pureData[size] = '\0';
    _pureSize = size;
    ::free(buf);
    return KS_SUCCESS;
}


int Query::getCount() 
{
    if (-1 == _nCount) {
        char *sCount = getParam(_pureData, _pureSize, "n");
        if (NULL == sCount) {
            return -1;
        }
        else {
            _nCount= atoi(sCount);
            ::free(sCount);
        }
    }
    return _nCount;
}

int Query::getStart() 
{
    if (0 == _nStart) {
        char *sStart = getParam(_pureData, _pureSize, "s");
        if (NULL == sStart) {
            return 0;
        }
        else {
            _nStart = atoi(sStart);
            ::free(sStart);
        }
    }
    return _nStart;
}


FRAMEWORK_END;


#include "framework/Cache.h"
#include "framework/Compressor.h"
#define KEY_COMP_LIMIT 50

FRAMEWORK_BEGIN;

CacheHandle::CacheHandle() 
    : _max_key_len(0), _max_val_len(0),
    _expire_time(0),
    _conn_timeout(MEMCACHED_DEFAULT_TIMEOUT), _send_timeout(0), _recv_timeout(0),
    _put(0), _putfail(0), _lookup(0), _hit(0),  _nMinDocsFound(-1) {
    }

bool CacheHandle::get(struct iovec & key, struct iovec & val) {
    _lookup++;
    if((_max_key_len > 0 && key.iov_len > _max_key_len) ||
            !doGet(key, val)) {
        return false;
    }
    _hit++;
    return true;
}

bool CacheHandle::put(struct iovec & key, struct iovec & val) {
    _put++;
    if((_max_key_len > 0 && key.iov_len > _max_key_len) ||
            (_max_val_len > 0 && val.iov_len > _max_val_len) ||
            !doPut(key, val)) {
        _putfail++;
        return false;
    }
    return true;
}

FRAMEWORK_END;

/**********************************MemcacheClient********************/
#include "mxml.h"
#include "util/Log.h"
#include "libmemcached/memcached.h"
#include "util/ScopedLock.h"
#include <stdio.h>
#include <math.h>

FRAMEWORK_BEGIN;

int stringsplit(const char *ptr, const char delime, std::vector<std::string> &str_vec)
{
    if (ptr == NULL)
    {
        return 1;
    }
    str_vec.clear();
    const char *headptr = ptr;
    const char *tailptr = ptr;
    while (1)
    {
        tailptr = strchr(headptr, delime);
        if (tailptr == NULL)
        {
            if (headptr != NULL)
            {
                str_vec.push_back(std::string(headptr, strlen(headptr)));
            }
            break;
        }
        str_vec.push_back(std::string(headptr, tailptr-headptr));
        headptr = tailptr + 1;
    }
    return 0;
}

void MemcacheClient::setIgnore(const char *ignore_list)
{
    if (!ignore_list || !ignore_list[0])
        return;
    stringsplit(ignore_list, ' ', _ignore_vec);
    return;
}


int32_t MemcacheClient::initialize(const char * path) {
    FILE * fp;
    mxml_node_t * tree;
    mxml_node_t * node;
    mxml_node_t * child;
    const char * attr;
    const char * ip;
    int nval;
    int port;
    int servercount;
    memcached_return rc;
    memcached_st * mc;
    fp = ::fopen(path, "r");
    if(unlikely(!fp)) {
        TERR("file does not exist or Permission denied: %s",
                SAFE_STRING(path));
        return KS_EFAILED;
    }
    tree = mxmlLoadFile(NULL, fp, MXML_NO_CALLBACK);
    ::fclose(fp);
    if(!tree) {
        TERR("parse xml config file error: %s", SAFE_STRING(path));
        return KS_EFAILED;
    }
    node = mxmlFindElement(tree, tree, "cache", NULL, NULL, MXML_DESCEND);
    if(!node) {
        mxmlDelete(tree);
        TERR("memcached xml error");
        return KS_EFAILED;
    }
    attr = mxmlElementGetAttr(node, "max_key_length");
    if(attr != NULL) {
        nval = atoi(attr);
        if(nval <= 0) nval = MEMCACHED_MAX_KEY;
    } else {
        nval = MEMCACHED_MAX_KEY;
    }
    setKeyLimit(nval);
    TLOG("memcached set: max_key_length = %u", nval);
    attr = mxmlElementGetAttr(node, "max_value_length");
    nval = 0;
    if(attr != NULL) {
        nval = atoi(attr);
    }
    if(nval > 0) {
        setValLimit(nval);
        TLOG("memcached set: max_value_length = %u", nval);
    }

    attr = mxmlElementGetAttr(node, "connect_timeout");
    nval = 0;
    if(attr != NULL) {
        nval = atoi(attr);
    }
    if(nval > 0) {
        setConnTimeout(nval);
        TLOG("memcached set: connect_timeout = %u", nval);
    }

    attr = mxmlElementGetAttr(node, "snd_timeout");
    nval = 0;
    if(attr != NULL) {
        nval = atoi(attr);
    }
    if(nval > 0) {
        setSendTimeout(nval);
        TLOG("memcached set: snd_timeout = %u", nval);
    }

    attr = mxmlElementGetAttr(node, "rcv_timeout");
    nval = 0;
    if(attr != NULL) {
        nval = atoi(attr);
    }
    if(nval > 0) {
        setRecvTimeout(nval);
        TLOG("memcached set: rcv_timeout = %u", nval);
    }

    attr = mxmlElementGetAttr(node, "key_compress");
    if(attr != NULL) {
        setKeyCompr(attr);
        TLOG("memcached set: key_compress = %s", attr);
    }

    attr = mxmlElementGetAttr(node, "val_compress");
    if(attr != NULL) {
        setValCompr(attr);
        TLOG("memcached set: val_compress = %s", attr);
    }

    attr = mxmlElementGetAttr(node, "ignore");
    if(attr != NULL) {
        setIgnore(attr);
        TLOG("memcached set: ignore = %s", attr);
    }

    attr = mxmlElementGetAttr(node, "expire_time");
    _expire_time = 0;
    if(attr != NULL) {
        nval = atoi(attr);
        if(nval > 0 ) _expire_time = nval;
    }
    TLOG("memcached set: expire_time = %u", _expire_time);


    attr = mxmlElementGetAttr(node, "min_docsFound");
    _nMinDocsFound = -1;
    if(attr != NULL) {
        nval = atoi(attr);
        if(nval > 0 ) _nMinDocsFound = nval;
    }
    TLOG("min docs found set: minDocsFound = %d", _nMinDocsFound);


    mc = memcached_create(NULL);
    if(mc == NULL) {
        mxmlDelete(tree);
        TERR("memcached_create(...) error!");
        return KS_EFAILED;
    }
    memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_CONNECT_TIMEOUT, getConnTimeout());
    memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_SND_TIMEOUT, getSendTimeout());
    memcached_behavior_set(mc, MEMCACHED_BEHAVIOR_RCV_TIMEOUT, getRecvTimeout());
    servercount = 0;
    for(child = mxmlFindElement(node, node, 
                "server", "ip", NULL, MXML_DESCEND);
            child != NULL; 
            child = mxmlFindElement(child, node, 
                "server", "ip", NULL, MXML_DESCEND)) {
        ip = mxmlElementGetAttr(child, "ip");
        attr = mxmlElementGetAttr(child, "port");
        port = 0;
        if(attr != NULL) port = atoi(attr);
        if(port <= 0) {
            TWARN("memcached: server %s use default port %d!", 
                    ip, MEMCACHED_DEFAULT_PORT);
            port = MEMCACHED_DEFAULT_PORT;
        }
        rc = memcached_server_add(mc, (char*)ip, port);
        if(rc != MEMCACHED_SUCCESS) {
            TERR("memcached_server_add(...) error: %s:%d", ip, port);
            continue;
        }
        TLOG("memcached set: server %s:%d", ip, port);
        servercount++;
    }
    mxmlDelete(tree);
    if(servercount == 0) {
        TERR("memcached: no valid cache server");
        memcached_free(mc);
        return KS_EFAILED;
    }
    _handle = mc;
    TLOG("memcached client set succeed! %d servers. (libmemcached-%s)",
            servercount, LIBMEMCACHED_VERSION_STRING);
    return KS_SUCCESS;
}

MemcacheClient::~MemcacheClient() {
    memcached_st * mc;
    MUTEX_LOCK(_lock);
    while(_handles.size() > 0) {
        mc = (memcached_st*)_handles.top();
        _handles.pop();
        if(mc) memcached_free(mc);
    }
    MUTEX_UNLOCK(_lock);
    mc = (memcached_st*)_handle;
    if(mc) memcached_free(mc);
    if (_key_compress) free(_key_compress);
    if (_val_compress) free(_val_compress);

}

void MemcacheClient::processUrl(char *url)
{
    if (!url || !url[0]) return;
    int32_t urlsize = strlen(url)+1;
    char *tmpurl = strstr(url, "?");
    if (tmpurl == NULL)
        return;
    tmpurl++;
    int32_t offset = tmpurl - url;
    int32_t len = 0;
    int32_t i,j;
    std::vector<std::string> tmpvec;
    stringsplit(tmpurl, '&', tmpvec);
    if (tmpvec.size() <= 0)
        return;
    for (i=0; i<tmpvec.size(); i++)
    {
        for (j=0; j<_ignore_vec.size(); j++)
        {
            if (strncmp(tmpvec[i].c_str(), _ignore_vec[j].c_str(), 
                        _ignore_vec[j].size())==0)
            {
                tmpvec[i] = "";
                break;
            }
        }
    }
    sort(tmpvec.begin(), tmpvec.end());
    bool isbegin = true;
    for (i=0; i<tmpvec.size(); i++)
    {
        if (tmpvec[i].size() > 0)
        {
            if (isbegin)
                len = snprintf(url+offset, urlsize-offset, "%s", tmpvec[i].c_str());
            else
                len = snprintf(url+offset, urlsize-offset, "&%s", tmpvec[i].c_str());
            offset += len;
            isbegin = false;
        }
    }
    return;
}

bool MemcacheClient::doGet(const struct iovec & key, 
        struct iovec & val) {
    memcached_st * mc = NULL;
    memcached_return rc;
    uint32_t flag;
    size_t vallen;
    if(unlikely(!_handle)) return false;
    if(key.iov_base == NULL || key.iov_len == 0) return false;

    struct iovec curkey;
    curkey.iov_base = new char[key.iov_len+1];
    memcpy(curkey.iov_base, key.iov_base, key.iov_len);
    ((char *)curkey.iov_base)[key.iov_len] = '\0';
    curkey.iov_len = key.iov_len;

    if (_ignore_vec.size() > 0)
    {
        processUrl((char *)curkey.iov_base);
        curkey.iov_len = strlen((char *)curkey.iov_base);
    }
    if ((_key_compress != NULL)&&(curkey.iov_len>KEY_COMP_LIMIT))
    {
        if (strcasecmp(_key_compress, "md5") == 0)
        {
            char *ctmp = util::MD5String((const char *)curkey.iov_base);
            delete [] (char *)curkey.iov_base;
            curkey.iov_base = ctmp;
            if (!ctmp) return false;
            curkey.iov_len = 33;
        }
    }
    MUTEX_LOCK(_lock);
    if(_handles.size() > 0) {
        mc = (memcached_st*)_handles.top();
        _handles.pop();
    } else {
        mc = NULL;
    }
    MUTEX_UNLOCK(_lock);
    if(!mc) {
        mc = memcached_clone(NULL, (memcached_st*)_handle);
    }
    if(!mc) 
    {
        delete [] (char *)(curkey.iov_base);
        return false;
    }
    val.iov_base = memcached_get(mc, 
            (const char*)curkey.iov_base, curkey.iov_len, 
            &vallen, &flag, &rc);
    delete [] (char *)curkey.iov_base;
    MUTEX_LOCK(_lock);
    _handles.push(mc);
    MUTEX_UNLOCK(_lock);
    if(rc != MEMCACHED_SUCCESS) {
        if(val.iov_base != NULL) {
            ::free(val.iov_base);
            val.iov_base = NULL;
        }
        return false;
    } 
    if(vallen == 0) {
        if(val.iov_base != NULL) { 
            ::free(val.iov_base);
            val.iov_base = NULL;
        }
        return false;
    }
    val.iov_len = vallen;
    if (_val_compress != NULL)
    {
        //uncompress
        char *uncmpr = NULL;
        uint32_t uncmprsize = 0;
        FRAMEWORK::Compressor *compr;
        FRAMEWORK::compress_type_t cmpr_t = FRAMEWORK::ct_none;
        if (strcasecmp(_val_compress,"z")==0)
        {
            cmpr_t = FRAMEWORK::ct_zlib;
        }
        compr = FRAMEWORK::CompressorFactory::make(cmpr_t);
        if (compr != NULL)
        {
            int32_t com_ret = compr->uncompress((const char *)val.iov_base, vallen, uncmpr, uncmprsize);
            FRAMEWORK::CompressorFactory::recycle(compr);
            ::free(val.iov_base);
            if ((com_ret!=KS_SUCCESS)||(uncmpr==NULL)||(uncmprsize==0))
            {
                ::free(uncmpr);
                val.iov_len = 0;
                return false;
            }
            val.iov_base = uncmpr;
            val.iov_len = uncmprsize;
        }
        //uncompress end
    }
    return true;
}

bool MemcacheClient::doPut(const struct iovec & key, 
        const struct iovec & val) {
    memcached_st * mc = NULL;
    memcached_return rc;
    time_t expiration = 0;
    bool bValNeedFree = false;
    if(unlikely(!_handle)) return false;
    if(key.iov_base == NULL || key.iov_len == 0 || 
            val.iov_base == NULL || val.iov_len == 0) return false;
    struct iovec curkey,curval;
    curkey.iov_base = new char[key.iov_len+1];
    memcpy(curkey.iov_base, key.iov_base, key.iov_len);
    ((char *)curkey.iov_base)[key.iov_len] = '\0';
    curkey.iov_len = key.iov_len;

    curval.iov_base = val.iov_base;
    curval.iov_len = val.iov_len;

    if (_ignore_vec.size() > 0)
    {
        processUrl((char *)curkey.iov_base);
        curkey.iov_len = strlen((char *)curkey.iov_base);
    }
    if ((_key_compress!=NULL)&&(curkey.iov_len>KEY_COMP_LIMIT))
    {
        if (strcasecmp(_key_compress, "md5") == 0)
        {
            char *ctmp = util::MD5String((const char *)curkey.iov_base);
            delete [] (char *)curkey.iov_base;
            curkey.iov_base = ctmp;
            if (!ctmp) return false;
            curkey.iov_len = 33;
        }
    }
    if (_val_compress != NULL)
    {
        //compress
        FRAMEWORK::Compressor *compr;
        FRAMEWORK::compress_type_t cmpr_t = FRAMEWORK::ct_none;
        if (strcasecmp(_val_compress, "z") == 0)
        {
            cmpr_t = FRAMEWORK::ct_zlib;
        }
        compr = FRAMEWORK::CompressorFactory::make(cmpr_t);
        if (compr != NULL)
        {
            char *cmpr = NULL;
            uint32_t cmprsize = 0;
            int32_t com_ret = compr->compress((const char *)val.iov_base, val.iov_len, cmpr, cmprsize);
            FRAMEWORK::CompressorFactory::recycle(compr);
            if ((com_ret!=KS_SUCCESS)||(cmpr==NULL)||(cmprsize==0))
            {
                ::free(cmpr);
                cmpr = NULL;
                delete [] (char *)curkey.iov_base;
                return false;
            }
            curval.iov_base = cmpr;
            curval.iov_len = cmprsize;
            bValNeedFree = true;
        }
        //compress end
    }
    MUTEX_LOCK(_lock);
    if(_handles.size() > 0) {
        mc = (memcached_st*)_handles.top();
        _handles.pop();
    } else {
        mc = NULL;
    }
    MUTEX_UNLOCK(_lock);
    if(!mc) {
        mc = memcached_clone(NULL, (memcached_st*)_handle);
    }
    if(!mc) 
    {	
        if (bValNeedFree) ::free((char *)curval.iov_base);
        delete [] (char *)(curkey.iov_base);
        return false;
    }
    if(_expire_time > 0) expiration = time(NULL) + _expire_time;
    rc = memcached_set(mc, 
            (const char *)curkey.iov_base, curkey.iov_len,
            (const char *)curval.iov_base, curval.iov_len, 
            expiration, (uint32_t)0);
    if (bValNeedFree) ::free(curval.iov_base);
    delete [] (char *)(curkey.iov_base);

    // add for bug #83158
    if (rc != MEMCACHED_SUCCESS) {
        memcached_quit(mc);
    }

    MUTEX_LOCK(_lock);
    _handles.push(mc);
    MUTEX_UNLOCK(_lock);

    if(rc != MEMCACHED_SUCCESS) {
        TNOTE("memcached_set %s error!",memcached_strerror(mc,rc));
        return false;
    }
    return true;
}

FRAMEWORK_END;


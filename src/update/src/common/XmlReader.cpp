#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "XmlReader.h"
#include "util/MD5.h"
#include "util/HashMap.hpp"
#include "util/ThreadLock.h"
#include "ks_build/ks_build.h"

namespace update {

XmlReader::XmlReader(ksb_conf_t * conf)
{
    _fp   = NULL;
    _gzfp = 0;
    _conf = conf;
    _need_utf8 = false;
    _line = NULL;
    _changeBuf = NULL;
    _docCount = 0;
    _conv_t = NULL;
    _maxLen = DEFAULT_MESSAGE_SIZE<<1;
}

XmlReader::~XmlReader()
{
    if(_fp) fclose(_fp);
    if(_gzfp) gzclose(_gzfp);
    if(_line) delete _line;
    if(_changeBuf) delete _changeBuf;
}

int XmlReader::init(const char* filename, bool needUtf8)
{
    _need_utf8 = needUtf8;
    if(_conv_t == NULL) { // 第一次
        _conv_t = code_conv_open(CC_UTF8, CC_GBK, CC_FLAG_IGNORE);
        if(NULL == _conv_t) {
            return -1;
        }
        if(initFieldTable(_conf, _field_table) < 0) {
            code_conv_close(_conv_t);
            _conv_t = NULL;
            return -1;
        }
        _line = new char[_maxLen];
        _changeBuf = new char[_maxLen];
        if(NULL == _line || NULL == _changeBuf) {
            if(_line) delete _line; _line = NULL;
            if(_changeBuf) delete _changeBuf; _changeBuf = NULL;
            code_conv_close(_conv_t); _conv_t = NULL;
            return -1;
        }
    }

    if(_fp) fclose(_fp);
    if(_gzfp) gzclose(_gzfp);

    _fp = NULL;
    _gzfp = NULL;
    _docCount = 0;

    if(filename) {
        char* name = strrchr(filename, '/');
        if(NULL == name) {
            name = (char*)filename;
        } else {
            name++;
        }
        int len = strlen(name);
        if(len > 3 && strcmp(name + len - 3, ".gz") == 0) {
            if((_gzfp = gzopen(filename, "rb")) == NULL) {
                return -1;
            }
        } else {
            if((_fp = fopen(filename, "rb")) == NULL) {
                return -1;
            }
        }
    }

    return 0;
}
    
DocMessage* XmlReader::next()
{
    if(NULL == _fp && NULL == _gzfp) {
        return NULL;
    }

    bool bBegin = false;
    bool bHasNid = false;
    bool bFind = false;
    char* ret = NULL;

    while (1) {
        if(_fp) {
            ret = fgets(_line, _maxLen, _fp);
        } else {
            ret = gzgets(_gzfp, _line, _maxLen);
        }
        if(ret == NULL) break;
        if (_line[0] == '\n') continue;

        if (unlikely(strstr(_line, "<doc>") == _line)) {
            _msg.Clear();
            bBegin = true;
            bHasNid = false;
        }
        else if (unlikely(bBegin && strstr(_line, "</doc>") == _line)) {
            bBegin = false;
            if (bHasNid) {
                bFind  = true;
                _msg.set_action(ACTION_ADD);
                _msg.set_seq(0);
                _docCount++;
                break; 
            }
        }
        else if (likely(bBegin)) {
            char *p = strstr(_line, "=");
            if (unlikely(p == NULL)) {
                continue;
            }
            char *szValue = p + 1;
            *p = '\0';
            int len = strlen(szValue);
            if (likely(len > 0)) {
                szValue[len - 1] = 0;
                if (len > 1 && szValue[len - 2] == '\01') {
                    szValue[len - 2] = '\0';
                }
            }
            if (unlikely(strcmp(_line, "nid") == 0) && strlen(szValue) > 0) {
                long nid = strtoul(szValue, NULL, 10);
                _msg.set_nid(nid);
                bHasNid = true;
            }
            DocMessage_Field *pField = _msg.add_fields();
            pField->set_field_name(_line);
            pField->set_field_value(szValue);
        }
    }
    
    if(bFind == false) {
        return NULL;
    }
    if(_need_utf8 == false) {
        return &_msg;
    }

    // 进行编码转换
    _msgNew.Clear();
    _msgNew.set_nid(_msg.nid());
    for(int i = 0; i < _msg.fields_size(); i++) {
        const DocMessage_Field& field = _msg.fields(i);
        const std::string& value = field.field_value();
        if(value.empty()) continue;

        ssize_t len = code_conv(_conv_t, _changeBuf, _maxLen, value.c_str());
        if(len <= 0) {
            continue;
        }
        _changeBuf[len] = 0;
        DocMessage_Field *pField = _msgNew.add_fields();
        pField->set_field_name(field.field_name());
        pField->set_field_value(_changeBuf);
    }

    return &_msgNew;
}
   
int XmlReader::sign(DocMessage* msg, unsigned char digest[16])
{
    int array[_field_table.size()];
    memset(array, 0xFF, _field_table.size() * sizeof(int)); 

    for(int i = 0; i < msg->fields_size(); i++) {
        const ::DocMessage_Field& field = msg->fields(i);
        const std::string& name = field.field_name();
        util::HashMap<char*, int>::iterator it = _field_table.find((char*)name.c_str());
        if(it == _field_table.end()) {
            continue;
        }
        array[it->value] = i;
    }

    char* p = _changeBuf;
    for(uint32_t i = 0; i < _field_table.size(); i++) {
        if(array[i] < 0) {
            p += sprintf(p, "%s=&", _fields_name[i]);
            continue;
        }

        const ::DocMessage_Field& field = msg->fields(array[i]);
        const std::string& name = field.field_name();
        const std::string& value = field.field_value();

        memcpy(p, name.c_str(), name.length());
        p += name.length();
        
        *p++ = '=';
        
        memcpy(p, value.c_str(), value.length());
        p += value.length();
        p--;
        while(isspace(*p)) p--;
        p++;
        *p++ = '&';
    }
    if(unlikely(p - _changeBuf > _maxLen)) {
        TERR("over max len=%ld(%d), averything append", p - _changeBuf, _maxLen);
        return -1;
    }
    _changeBuf[p - _changeBuf] = 0;

    util::MD5_CTX context;
    util::MD5Init(&context);
    util::MD5Update(&context, (const unsigned char*)_changeBuf, p - _changeBuf);
    util::MD5Final(digest, &context);

    return 0;
}

int XmlReader::initFieldTable(ksb_conf_t * conf, util::HashMap<char*, int>& field)
{
    int no = 0;
    for(uint32_t i = 0; i < conf->index_count; i++) {
        if(field.find(conf->index[i].name) != field.end()) {
            continue;
        }
        if(!field.insert(conf->index[i].name, no)) {
            field.clear();
            TERR("init index field table error, %s", conf->index[i].name);
            return -1;
        }
        _fields_name[no++] = conf->index[i].name;
        if(no >= KSB_FIELD_MAX*3) {
            TERR("over max field num=%d(%d)", no, KSB_FIELD_MAX*3);
            return -1;
        }
    }

    for(uint32_t i = 0; i < conf->profile_count; i++) {
        if(field.find(conf->profile[i].name) != field.end()) {
            continue;
        }
        if(!field.insert(conf->profile[i].name, no)) {
            field.clear();
            TERR("init profile field table error, %s", conf->profile[i].name);
            return -1;
        }
        _fields_name[no++] = conf->profile[i].name;
        if(no >= KSB_FIELD_MAX*3) {
            TERR("over max field num=%d(%d)", no, KSB_FIELD_MAX*3);
            return -1;
        }
    }
/*    
    for(int i = 0; i < conf->detail_count; i++) {
        if(field.find(conf->detail[i].name) != field.end()) {
            continue;
        }
        if(!field.insert(conf->detail[i].name, no)) {
            field.clear();
            TERR("init detail field table error, %s", conf->detail[i].name);
            return -1;
        }
        _fields_name[no++] = conf->detail[i].name;
        if(no >= KSB_FIELD_MAX*3) {
            TERR("over max field num=%d(%d)", no, KSB_FIELD_MAX*3);
            return -1;
        }
    }
*/
    return no;
}

}

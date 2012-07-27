#include "Param.h"
#include "util/Log.h"
#include "util/strfunc.h"
#include "util/StringUtility.h"

/* 经纬度参数定义 */
#define LATLNG_PARAM "latlng"
#define LATLNG_DELIM ':'

namespace queryparser {
/* 无参构造函数 */
Param::Param(MemPool *mempool)
{
    _iter = _dict.begin();	
    mDestroy = false;
	mQueryString[0] = 0;
	mQueryLength = 0;
	mHlkeyString[0] = 0;
	mHlkeyLength = 0;
    _dict.clear();
    _mempool = mempool;
}

/* 析构函数 */
Param::~Param()
{
    _dict.clear();
}


/* 注册querystring */
void Param::setQueryString(const char* queryString, int queryLength)
{
	if(queryString && queryLength>0){
		mQueryLength = queryLength>MAX_QYERY_LEN?MAX_QYERY_LEN:queryLength;
		memcpy(mQueryString, queryString, mQueryLength);
		mQueryString[mQueryLength] = 0;
	}
	else{
		mQueryString[0] = 0;
		mQueryLength = 0;
	}
}

/* 注册hlkeystring */
void Param::setHlkeyString(const char* hlkeyString, int hlkeyLength, bool encode)
{
	if(hlkeyString && hlkeyLength>0){
		if(encode){
			if(hlkeyLength*3>=MAX_QYERY_LEN){
				mHlkeyString[0] = 0;
			}
			else{
				util::urlEncode2(mHlkeyString, MAX_QYERY_LEN, hlkeyString);
			}
			mHlkeyLength = strlen(mHlkeyString);
		}
		else{
			mHlkeyLength = hlkeyLength>MAX_QYERY_LEN?MAX_QYERY_LEN:hlkeyLength;
			memcpy(mHlkeyString, hlkeyString, mHlkeyLength);
			mHlkeyString[mHlkeyLength] = 0;
		}
	}
	else{
		mHlkeyString[0] = 0;
		mHlkeyLength = 0;
	}
}

/* 检查某个关键字是否存在 */
bool Param::hasParam(const char* key) const
{
	if(!key){
		return false;
	}
	
	// 先检查是不是要查找querystring
	if(strcmp(key, QUERY_FLAG)==0){
		return true;
	}
	
	// 先检查是不是要查找hlkeystring
	if(strcmp(key, HLKEY_FLAG)==0){
		return true;
	}
    char real_name[MAX_FIELD_NAME_LEN+1] = {0};
    snprintf(real_name, MAX_FIELD_NAME_LEN, "%s", key);
	str_lowercase(real_name);

    util::HashMap<const char*, char *>::const_iterator it; 
    it = _dict.find(real_name);
     
    return it != _dict.end();
}

/* 获取某个关键字对应的数据 */
const char* Param::getParam(const char* key) const 
{
	char* value = 0;
	int vlen = 0;
	if(!key){
		return 0;
	}
	
	char real_name[MAX_FIELD_NAME_LEN+1] = {0};
    snprintf(real_name, MAX_FIELD_NAME_LEN, "%s", key);
	str_lowercase(real_name);
	// 先检查是不是要查找querystring
	if(strcmp(key, QUERY_FLAG)==0){
		return mQueryString;
	}
	
	// 先检查是不是要查找hlkeystring
	if(strcmp(key, HLKEY_FLAG)==0){
		return mHlkeyString;
	}
    
    util::HashMap<const char*, char *>::const_iterator it; 
    it = _dict.find(real_name);
    if(it != _dict.end()) {
        return it->value;
    }
    return NULL;
}

/*
 * 获取客户端经纬度位置关键字对应的数据
 *
 * @param lat [out] 纬度
 * @param lng [out] 经度
 *
 * @return true, 字段存在且解析有效;
 *         false, 字段不存在，或格式无效
 */
bool Param::getLatLngParam(float & lat, float & lng) const
{
    const char * value = getParam(LATLNG_PARAM);
    if (unlikely(value == NULL)) {
        return false;
    }

    // 分隔经纬度参数
    const char * pDelim = strchr(value, LATLNG_DELIM);
    if (unlikely( pDelim == NULL || pDelim == value || *(pDelim + 1) == '\0')) {
        return false;
    }   

    char * pEnd = NULL;
    lat = strtof(value, &pEnd);
    if (unlikely(pEnd != pDelim)) {
        return false;
    }

    lng = strtof(pDelim + 1, &pEnd);
    if (unlikely(*pEnd != '\0')) {
        return false;
    }

    if (unlikely(lat > 90 || lat < -90)) {
        return false;
    }   

    if (unlikely(lng > 180 || lng < -180)) {
        return false;
    }
    return true;
}

/* 修改/添加某个关键字 */
bool Param::setParam(char* key, char* value)
{
	if(!key || !value){
		return false;
	}
    char *hash_key = NEW_VEC(_mempool, char, strlen(key)+1);
    snprintf(hash_key, strlen(key)+1, "%s", key);
	str_lowercase(hash_key);
	// 先检查是不是要修改querystring
	if(strcmp(key, QUERY_FLAG)==0){
		mQueryLength = strlen(value);
		if(mQueryLength>MAX_QYERY_LEN){
			mQueryLength = MAX_QYERY_LEN;
		}
		memcpy(mQueryString, value, mQueryLength);
		mQueryString[mQueryLength] = 0;
		return true;
	}
	
	// 先检查是不是要修改hlkeystring
	if(strcmp(key, HLKEY_FLAG)==0){
		mHlkeyLength = strlen(value);
		if(mHlkeyLength>MAX_QYERY_LEN){
			mHlkeyLength = MAX_QYERY_LEN;
		}
		memcpy(mHlkeyString, value, mHlkeyLength);
		mHlkeyString[mHlkeyLength] = 0;
		return true;
	}
    
    util::HashMap<const char*, char *>::iterator it; 
    it = _dict.find(hash_key);
    if(it != _dict.end()) {
        char *hash_value = NEW_VEC(_mempool, char, strlen(value)+1);
        snprintf(hash_value, strlen(value)+1, "%s", value);
        it->value = hash_value; 
    } else {
        char *hash_value = NEW_VEC(_mempool, char, strlen(value)+1);
        snprintf(hash_value, strlen(value)+1, "%s", value);
        _dict.insert(hash_key, hash_value); 
    }

	return true;
}

/* 获取节点个数 */
const int Param::getNum() const
{
    return _dict.size();
}

/* 使游标指向第一个数据 */
void Param::first()
{
    _iter =  _dict.begin();
}

/* 获取下一组数据 */
int Param::next(char* &key, int *key_len, char* &value, int *value_len)
{
	char* tmpName = 0;
	int tmpNlen = 0;
	char* tmpValue = 0;
	int tmpVlen = 0;

    if(_iter == _dict.end()){
        return -1;
    }

    key = (char *)_iter->key;
    *key_len = _iter->key?strlen(_iter->key) : 0;
    value = (char *)_iter->value;
    *value_len = _iter->value?strlen(_iter->value) : 0; 
    _iter++;
    return 1;
}

/* 打印key-value对字典，调试时使用 */
void Param::print()
{
	char* key = 0;
	char* value = 0;
	int idx = 0;
    int key_len = 0;
    int value_len = 0;
	first();
	
	while(next(key, &key_len, value, &value_len)==0){
		TDEBUG("Param node[%d] key=%s value=%s", idx, key, value);
		idx ++;
	}
}
}

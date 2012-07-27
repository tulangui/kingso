#include "StringUtil.h"
#include "string.h"
#include "util/Log.h"

namespace queryparser {
extern int _parse_keyvalue_string(char* string, int slen,
	char** name, int* nlen, char** value, int* vlen, char** type, int* tlen)
{
	char* begin = string;
	char* cur = 0;
	int clen = slen;
	
	// 先将name,value,type清零
	*name = *value = *type = 0;
	*nlen = *vlen = *tlen = 0;
	
	// 找到"::"，如果存在，前面的字符串就是type
	cur = strstr(begin, "::");
	if(cur){
		*cur = 0;
		*type = begin;
		*tlen = cur-begin;
		begin = cur+2;
		clen -= *tlen+2;
	}
	
	// 找到':'，如果存在，前面的字符串就是name，后面的就是value
	cur = strchr(begin, ':');
	if(cur){
		*cur = 0;
		*name = begin;
		*nlen = cur-begin;
		*value = cur+1;
		*vlen = clen-*nlen-1;
		return 0;
	}
	else{
		TNOTE("_parse_keyvalue_string no name error, return!");
		return -1;
	}
}



}

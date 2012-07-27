#include "queryparser/QPResult.h"
#include <stdlib.h>
#include "queryparser/FilterList.h"
#include "Param.h"


namespace queryparser {
/* 无参构造函数 */
QPResult::QPResult(MemPool* memPool):mMemPool(memPool)
{
	init();
}

/* 析构函数 */
QPResult::~QPResult()
{
}

/* 初始化该类的函数 */
void QPResult::init()
{
	do{
		// 创建各种实体
		_qp_tree = qp_syntax_tree_create(mMemPool);
		if(!_qp_tree){
			break;
		}
	    _filter_list = NEW(mMemPool,  FilterList)();
        if(!_filter_list) {
            break;
        }
		_param = NEW(mMemPool, Param)(mMemPool);
		if(!_param){
			break;
		}
		return;
	}while(0);
	
	// 错误处理
	if(_qp_tree){
	}
    if(_filter_list) {
        _filter_list = NULL;
    }
    if(_param){
		_param = NULL;
	}
}

qp_syntax_tree_t *QPResult::getSyntaxTree()
{
    return _qp_tree;
}

/* 获取querystring解析成key-value的结果 */
Param* QPResult::getParam()
{
	return _param;
}

/* 获取内存池 */
MemPool* QPResult::getMemPool()
{
	return mMemPool;
}

/* 计算query序列化结果大小 */
int QPResult::getSerialSize()
{
	char* name = 0;
	int nlen = 0;
	char* value = 0;
	int vlen = 0;
	int size = 0;
	
	const char* query = 0;
	const char* hlkey = 0;
	
	if(!_param){
		return 0;
	}
	
	query = _param->getParam(QUERY_FLAG);
	hlkey = _param->getParam(HLKEY_FLAG);
    _param->first();	
    while(_param->next(name, &nlen, value, &vlen)>0){
		size += (sizeof(int)*2+nlen+1+vlen);
	}
	size += (sizeof(int)*2+strlen(QUERY_FLAG)+1+strlen(query)+1);
	size += (sizeof(int)*2+strlen(HLKEY_FLAG)+1+strlen(hlkey)+1);
	size += sizeof(int)*2;
	
	return size;
}
		
/* 序列化query */
int QPResult::serialize(char* buffer, int size)
{
	char* name = 0;
	int nlen = 0;
	char* value = 0;
	int vlen = 0;
	
	int need = sizeof(int)*2;
	int count = 0;
	char* curp = buffer+sizeof(int)*2;
	int* intp = (int*)buffer;
	
	const char* query = 0;
	const char* hlkey = 0;
	
	if(need>size){
        return -1;
	}
	
	if(!buffer || size<=0){
		return -1;
	}
	
	if(!_param){
		return -1;
	}
	
	query = _param->getParam(QUERY_FLAG);
	hlkey = _param->getParam(HLKEY_FLAG);

    _param->first();
	while(_param->next(name, &nlen, value, &vlen)>0){
        need += (sizeof(int)*2+nlen+1+vlen);
		count ++;
		if(need>size){
			return -1;
		}
		*((int*)curp) = nlen;
		curp += sizeof(int);
		*((int*)curp) = vlen;
		curp += sizeof(int);
		if(nlen>0){
			memcpy(curp, name, nlen);
			curp += nlen;
			*curp = 0;
			curp ++;
		}
		if(vlen>0){
			memcpy(curp, value, vlen);
			curp += vlen;
		}
	}
	
	/* set query */{
		name = QUERY_FLAG;
		nlen = strlen(QUERY_FLAG);
		value = (char*)query;
		vlen = strlen(query)+1;
		need += (sizeof(int)*2+nlen+1+vlen);
		count ++;
		if(need>size){
			return -1;
		}
		*((int*)curp) = nlen;
		curp += sizeof(int);
		*((int*)curp) = vlen;
		curp += sizeof(int);
		if(nlen>0){
			memcpy(curp, name, nlen);
			curp += nlen;
			*curp = 0;
			curp ++;
		}
		if(vlen>0){
			memcpy(curp, value, vlen);
			curp += vlen;
		}
	}
	
	/* set hlkey */{
		name = HLKEY_FLAG;
		nlen = strlen(HLKEY_FLAG);
		value = (char*)hlkey;
		vlen = strlen(hlkey)+1;
		need += (sizeof(int)*2+nlen+1+vlen);
		count ++;
		if(need>size){
			return -1;
		}
		*((int*)curp) = nlen;
		curp += sizeof(int);
		*((int*)curp) = vlen;
		curp += sizeof(int);
		if(nlen>0){
			memcpy(curp, name, nlen);
			curp += nlen;
			*curp = 0;
			curp ++;
		}
		if(vlen>0){
			memcpy(curp, value, vlen);
			curp += vlen;
		}
	}
	
	intp[0] = need;
	intp[1] = count;
	return need;
}
	
/* 反序列化 */
int QPResult::deserialize(char* buffer, int size, MemPool* memPool)
{
	char* name = 0;
	int nlen = 0;
	void* value = 0;
	int vlen = 0;
	
	char* curp = buffer+sizeof(int)*2;
	int* intp = (int*)buffer;
	
	int i = 0;
	
	if(!buffer || size<=0){
        return -1;
	}
	if(intp[0]>size){
		return -1;
	}
	if(intp[1]<2){
		return -1;
	}
	
	if(!_param){
		return -1;
	}
	for(i=0; i<intp[1]-2; i++){
		nlen = *((int*)curp);
		curp += sizeof(int);
		vlen = *((int*)curp);
		curp += sizeof(int);
		if(nlen>0){
			name = curp;
			curp += nlen+1;
		}
		else{
			name = 0;
		}
		if(vlen>0){
			value = curp;
			curp += vlen;
		}
		else{
			value = 0;
		}
        char *hash_key = NEW_VEC(memPool, char, nlen+1);
        char *hash_value = NEW_VEC(memPool, char, vlen+1);
        snprintf(hash_key, nlen+1, "%s", name);
        snprintf(hash_value, vlen+1, "%s", value);
        if(_param->setParam(hash_key, hash_value)<0){
            return -1;
		}
	}
	
	/* get query */{
		nlen = *((int*)curp);
		curp += sizeof(int);
		vlen = *((int*)curp);
		curp += sizeof(int);
		if(nlen>0){
			name = curp;
			curp += nlen+1;
		}
		else{
			name = 0;
		}
		if(vlen>0){
			value = curp;
			curp += vlen;
		}
		else{
			value = 0;
		}
		_param->setQueryString((char*)value, vlen-1);
	}
	
	/* get hlkey */{
		nlen = *((int*)curp);
		curp += sizeof(int);
		vlen = *((int*)curp);
		curp += sizeof(int);
		if(nlen>0){
			name = curp;
			curp += nlen+1;
		}
		else{
			name = 0;
		}
		if(vlen>0){
			value = curp;
			curp += vlen;
		}
		else{
			value = 0;
		}
		_param->setHlkeyString((char*)value, vlen-1);
	}
	
	return 0;
}

FilterList *QPResult::getFilterList()
{
    return _filter_list;
}
/* 打印query解析结果，调试时使用 */
void QPResult::print()
{
	// 调用每个成员的print函数
	if(_qp_tree){
        qp_syntax_tree_print(_qp_tree->root);
    }
    if(_filter_list) {
        _filter_list->print();
    }
    if(_param){
		_param->print();
	}
}
}

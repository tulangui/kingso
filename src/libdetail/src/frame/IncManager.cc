#include "libdetail/IncManager.h"
#include "libdetail/detail.h"
#include <string.h>
#include "util/Log.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


namespace detail{

IncManager* IncManager::mInstance = 0;

IncManager::IncManager()
{
	mInstance=0;
	mMsgCount=0;
	mMsgcntFile=0;
	mStrBuffer=0;
	mStrBuflen=0;
	mFieldPointer=0;
	mFieldPoinlen=0;
	
	if(mMsgcntFile==0){
		const char* msgcnt_path = get_di_msgcnt_file();
		if(access(msgcnt_path, F_OK)){
			int fd = creat(msgcnt_path, 0666);
			if(fd<0){
				TWARN("creat msgcnt file error, return!");
				return;
			}
			close(fd);
		}
		mMsgcntFile = fopen(msgcnt_path, "r+");
		if(mMsgcntFile==0){
			TWARN("fopen msgcnt file error, return!");
			return;
		}
		mMsgCount = 0;
		fseek(mMsgcntFile, 0, SEEK_SET);
		fscanf(mMsgcntFile, "%u", &mMsgCount);
	}
	
	TNOTE("persisted %d message", mMsgCount);
}

/* 获取单实例对象指针 */
IncManager* IncManager::getInstance()
{
	if(!mInstance){
		mInstance = new IncManager();
	}

	return mInstance;
}

/* 添加一个doc到detail中 */
bool IncManager::add(const DocMessage* msg)
{
	const char** di_field_name = get_di_field_name();
	const int* di_field_nlen = get_di_field_nlen();
	const int di_field_cnt = get_di_field_count();
	
	int64_t nid = 0;
	const char* content = 0;
	int32_t len = 0;
	
	int needLen = 0;
	int i = 0;
	int j = 0;
	char* cur = 0;
	
	if(!msg){
		TWARN("IncManager::add argument error, return!");
		return false;
	}
	
	// 获取field相关信息
	if(!di_field_name || !di_field_nlen || di_field_cnt==0){
		TWARN("get field infomation error, return!");
		return false;
	}
	
	if(mMsgcntFile==0){
		const char* msgcnt_path = get_di_msgcnt_file();
		if(access(msgcnt_path, F_OK)){
			int fd = creat(msgcnt_path, 0666);
			if(fd<0){
				TWARN("creat msgcnt file error, return!");
				return false;
			}
			close(fd);
		}
		mMsgcntFile = fopen(msgcnt_path, "r+");
		if(mMsgcntFile==0){
			TWARN("fopen msgcnt file error, return!");
			return false;
		}
		mMsgCount = 0;
		fseek(mMsgcntFile, 0, SEEK_SET);
		fscanf(mMsgcntFile, "%u", &mMsgCount);
	}
	
	// 为field到value的指针数组分配空间
	if(mFieldPoinlen<di_field_cnt){
		int* temp = (int*)malloc(di_field_cnt*sizeof(int));
		if(!temp){
			TWARN("malloc mFieldPointer error, return!");
			return false;
		}
		if(mFieldPointer){
			free(mFieldPointer);
		}
		mFieldPointer = temp;
		mFieldPoinlen = di_field_cnt;
	}
	
	// 将数据与field对应起来，并计算数据总长度
	for(j=0; j<di_field_cnt; j++){
		for(i=0; i<msg->fields_size(); i++){
			if(di_field_nlen[j]==msg->fields(i).field_name().length() &&
			   strncmp(di_field_name[j], msg->fields(i).field_name().c_str(), di_field_nlen[j])==0){
				mFieldPointer[j] = i;
				needLen += msg->fields(i).field_value().length()+1;
				break;
			}
		}
		if(i==msg->fields_size()){
			TWARN("no found fied error, return![field:%s]", di_field_name[j]);
			return false;
		}
	}
	//needLen ++;
	
	// 为数据分配缓存空间
	if(mStrBuflen<needLen){
		char* temp = (char*)malloc(needLen);
		if(!temp){
			TWARN("malloc mStrBuffer error, return!");
			return false;
		}
		if(mStrBuffer){
			free(mStrBuffer);
		}
		mStrBuffer = temp;
		mStrBuflen = needLen;
	}
	
	// 制作数据
	cur = mStrBuffer;
	for(i=0; i<mFieldPoinlen; i++){
		memcpy(cur, msg->fields(mFieldPointer[i]).field_value().c_str(),
			msg->fields(mFieldPointer[i]).field_value().length());
		cur += msg->fields(mFieldPointer[i]).field_value().length();
		*cur = '\001';
		cur ++;
	}
	*(cur-1) = 0;
	
	// 添加数据到detail
	nid = msg->nid();
	content = mStrBuffer;
	len = needLen-1;
	
	if(di_detail_add(nid, content, len)<0){
		TWARN("di_detail_add error, return!");
		return false;
	}
	
	mMsgCount ++;
	fseek(mMsgcntFile, 0, SEEK_SET);
	if(fprintf(mMsgcntFile, "%u", mMsgCount)<0){
		TWARN("fprintf mMsgCount error, return!");
		fclose(mMsgcntFile);
		mMsgcntFile = 0;
	}
	fflush(mMsgcntFile);
	
	return true;
}

/* 删除一个doc */
bool IncManager::del(const DocMessage* msg)
{
	if(!msg){
		TWARN("IncManager::del argument error, return!");
		return false;
	}
	
	if(mMsgcntFile==0){
		const char* msgcnt_path = get_di_msgcnt_file();
		if(access(msgcnt_path, F_OK)){
			int fd = creat(msgcnt_path, 0666);
			if(fd<0){
				TWARN("creat msgcnt file error, return!");
				return false;
			}
			close(fd);
		}
		mMsgcntFile = fopen(msgcnt_path, "r+");
		if(mMsgcntFile==0){
			TWARN("fopen msgcnt file error, return!");
			return false;
		}
		mMsgCount = 0;
		fseek(mMsgcntFile, 0, SEEK_SET);
		fscanf(mMsgcntFile, "%u", &mMsgCount);
	}
	
	mMsgCount ++;
	fseek(mMsgcntFile, 0, SEEK_SET);
	if(fprintf(mMsgcntFile, "%u", mMsgCount)<0){
		TWARN("fprintf mMsgCount error, return!");
		fclose(mMsgcntFile);
		mMsgcntFile = 0;
	}
	fflush(mMsgcntFile);
	
	return true;
}

/* 修改doc对应的detail信息 */
bool IncManager::update(const DocMessage* msg)
{
	return add(msg);
}

}

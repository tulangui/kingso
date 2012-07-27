#ifndef BASE_FILE_H
#define BASE_FILE_H

#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

namespace store {

#define FILE_OPEN_READ		1
#define FILE_OPEN_WRITE		2
#define FILE_OPEN_APPEND	4
#define FILE_OPEN_UPDATE    5

//当前支持的文件类型
#define FILE_TYPE_MEMREAD   1
#define FILE_TYPE_SHM       2
#define FILE_TYPE_MMAP      3
#define FILE_TYPE_FS        4
#define FILE_TYPE_BFS		5

class CBaseFile
{
public:
	CBaseFile(void);
	virtual ~CBaseFile(void) = 0;
	// 打开文件
	virtual bool open(const char *szFileName, int flag, size_t length = 0) = 0;
	// 设置文件位置
	virtual bool setPosition(size_t pos, int whence = SEEK_SET) = 0;
	// 得到文件当前位置
	virtual size_t getPosition() = 0;
	// 读文件, 返回是读到字节数
	virtual size_t read(void *src, size_t len) = 0;
	// 写文件, 返回是写的字节数
	virtual size_t write(void *src, size_t len) = 0;
	// 得到文件大小
	virtual size_t getSize() = 0;
	// flush
	virtual bool flush() = 0;
	// 关闭文件
	virtual bool close() = 0;
	/* 把偏移量转换为地址 */
	virtual inline char *offset2Addr(size_t nOffset=0) const {return NULL;}
	/* 切换内存，仅供shmfile使用 */
	virtual bool refresh() {return true;}
	/* 得到文件名 */
	char *getFileName() const {
		return m_szFileName;
	}
	/* 在文件最后开辟一个新的空间， 返回最新开辟空间偏移量 */
	int32_t makeNewPage(int32_t nPageSize);
    /* 文件TYPE */
    virtual int getFileType() = 0;
	/* 释放文件所占用的内存资源 page cache  */
	virtual inline bool release() {return true;}
    
protected:
	char *m_szFileName;
};

}

#endif

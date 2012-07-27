#include "index_lib/IndexConfigParams.h"
#include "MMapFile.h"
#include <errno.h>
#include "util/Log.h"

namespace store
{

CMMapFile::CMMapFile(bool unload)
{
	m_szFileName = NULL;
	m_nLength = 0;
	m_nPos = 0;
	m_pBase = NULL;
	m_unload = unload;
#ifdef _WIN32
	m_pHander.m_hFile = NULL;
	m_pHander.m_hMapFile = NULL;
#else
	m_pHander.m_fd = 0;
#endif
}

CMMapFile::~CMMapFile()
{
	close();
}

/************************************************************************/
/* 打开文                                                               */
/************************************************************************/
bool CMMapFile::openEx(const char *szFileName, int flag, size_t length)
{
	if (!(flag & FILE_OPEN_READ)) return false;
	m_szFileName = strdup(szFileName);

	assert(m_pBase==NULL);
	if (m_pHander.m_fd > 0)
	{
		if(::close(m_pHander.m_fd) != 0){
			TERR("CMMapFile: close file fail: %s", strerror(errno));
		}else{
			m_pHander.m_fd = -1;
		}
	}
	m_pHander.m_fd = ::open(szFileName, O_RDONLY, 0644);
	if (m_pHander.m_fd == -1) {
		TERR("打开文件出错 %s 文件可能不存在, errno: %d, %s", szFileName, errno, strerror(errno));
		return false;
	}
	struct stat statInfo;
	if (fstat(m_pHander.m_fd, &statInfo ) < 0) {
		TERR("文件 %s fstat失败!", szFileName);
		::close(m_pHander.m_fd);
		return false;
	}
	m_nLength = statInfo.st_size;		//文件长度
	if(m_nLength == 0) return true;

    int mmap_flags = MAP_SHARED;
    if (index_lib::IndexConfigParams::getInstance()->isMemLock()) {
        TLOG("open %s with MAP_LOCKED flag!", szFileName);
        m_pBase = (char*)mmap(0, m_nLength, PROT_READ, mmap_flags | MAP_LOCKED, m_pHander.m_fd, 0);
        if (m_pBase == MAP_FAILED && errno == EAGAIN) {
            TERR("open %s with MAP_LOCKED flag failed: %s!", szFileName, strerror(errno));
            m_pBase = (char*)mmap(0, m_nLength, PROT_READ, mmap_flags, m_pHander.m_fd, 0);
        }
    }
    else {
        m_pBase = (char*)mmap(0, m_nLength, PROT_READ, mmap_flags, m_pHander.m_fd, 0);
    }

	//允许m_pBase 为NULL(文件size 为0的情况),后面程序逻辑上保证不去读取文件内容
	if(m_pBase == MAP_FAILED) {
		TERR("文件 %s 映射内存文件出错! error code:%d", szFileName,errno);
		::close(m_pHander.m_fd);
		m_pBase = NULL;
		return false;
	}
	madvise(m_pBase, m_nLength, MADV_SEQUENTIAL);	//Expect  page  references in sequential order.

	return true;
}


bool CMMapFile::open(const char *szFileName, int flag, size_t length)
{
	m_szFileName = strdup(szFileName);
	if (length > 0) {
		return create(szFileName, length);
	} else if ((flag & FILE_OPEN_WRITE)) {
		return openInternal(szFileName, false);
	} else {
		return openInternal(szFileName, true);
	}
}
bool CMMapFile::openInternal(const char* pszName, bool bReadOnly)
{
	assert(m_pBase==NULL);
#ifdef _WIN32
	char sFileName[MAX_FILE_PATH];
	char* pPos = strrchr(pszName, '/');
	assert(pPos!=NULL);
	++pPos;
	strcpy(sFileName, pPos);
	char* szWinName = string_utility::replace(pszName,'/','\\');
	m_pHander.m_hFile = ::CreateFile(szWinName,GENERIC_WRITE|GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	delete[] szWinName;
	if (m_pHander.m_hFile == INVALID_HANDLE_VALUE) {
		TERR("打开文件出错, %s 文件可能不存在!", pszName );
		return false;
	}
	m_pHander.m_hMapFile = CreateFileMapping(m_pHander.m_hFile, NULL, PAGE_READWRITE|SEC_COMMIT, 0, 0, sFileName);
	if (m_pHander.m_hMapFile == NULL) {
		TERR("创建映射文件 %s 失败.", pszName);
		CloseHandle(m_pHander.m_hFile);
		m_pHander.m_hFile = NULL;
		return false;
	}
	m_pBase = (char*)MapViewOfFile(m_pHander.m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (m_pBase == NULL) {
		TERR("创建映射视图 %s 失败.", pszName);
		CloseHandle(m_pHander.m_hMapFile);
		CloseHandle(m_pHander.m_hFile);
		m_pHander.m_hFile = m_pHander.m_hMapFile = NULL;
		return false;
	}
	m_nLength = GetFileSize(m_pHander.m_hFile,NULL); //文件长度
#else
	if (m_pHander.m_fd > 0)
	{
	        if(::close(m_pHander.m_fd) != 0){
        	        TERR("CMMapFile: close file fail: %s", strerror(errno));
        	}else{
                	m_pHander.m_fd = -1;
        	}
	}
	assert(!m_pHander.m_fd);
	if (bReadOnly) {
		m_pHander.m_fd = ::open(pszName, O_RDONLY, 0644);
	}
	else {
		m_pHander.m_fd = ::open(pszName, O_RDWR, 0644);
	}
	if (m_pHander.m_fd == -1) {
		TERR("打开文件出错 %s 文件可能不存在, errno: %d, %s", pszName, errno, strerror(errno));
		return false;
	}
	struct stat statInfo;
	if (fstat(m_pHander.m_fd, &statInfo ) < 0) {
		TERR("文件 %s fstat失败!", pszName);
		::close(m_pHander.m_fd);
		return false;
	}
	m_nLength = statInfo.st_size;		//文件长度

    int mmap_flags = MAP_SHARED;
    if (index_lib::IndexConfigParams::getInstance()->isMemLock()) {
        TLOG("open %s with MAP_LOCKED flag!", pszName);
        if (bReadOnly) {
            m_pBase = (char*)mmap(0, m_nLength, PROT_READ, mmap_flags | MAP_LOCKED, m_pHander.m_fd, 0);
        }
        else {
            m_pBase = (char*)mmap(0, m_nLength, PROT_READ|PROT_WRITE, mmap_flags | MAP_LOCKED, m_pHander.m_fd, 0);
        }
        if (m_pBase == MAP_FAILED && errno == EAGAIN) {
            TERR("open %s with MAP_LOCKED flag failed: %s!", pszName, strerror(errno));
            if (bReadOnly) {
                m_pBase = (char*)mmap(0, m_nLength, PROT_READ, mmap_flags, m_pHander.m_fd, 0);
            }
            else {
                m_pBase = (char*)mmap(0, m_nLength, PROT_READ|PROT_WRITE, mmap_flags, m_pHander.m_fd, 0);
            }
        }
    }
    else {
        if (bReadOnly) {
            m_pBase = (char*)mmap(0, m_nLength, PROT_READ, mmap_flags, m_pHander.m_fd, 0);
        }
        else {
            m_pBase = (char*)mmap(0, m_nLength, PROT_READ|PROT_WRITE, mmap_flags, m_pHander.m_fd, 0);
        }
    }

	//允许m_pBase 为NULL(文件size 为0的情况),后面程序逻辑上保证不去读取文件内容
	if (m_pBase == MAP_FAILED) {
		TERR("文件 %s 映射内存文件出错! error code:%d", pszName,errno);
		::close(m_pHander.m_fd);
		m_pBase = NULL;
		return false;
	}
	madvise(m_pBase, m_nLength, MADV_SEQUENTIAL);	//Expect  page  references in sequential order.
//	if(::close(m_pHander.m_fd) != 0){
//		TERR("CMMapFile: close file fail: %s", strerror(errno));
//	}else{
//		m_pHander.m_fd = -1;
//	}
#endif

	return true;
}

bool CMMapFile::reopen(off_t nOffset, size_t nLength)
{
	if(m_pHander.m_fd == -1)
		m_pHander.m_fd = ::open(m_szFileName, O_RDONLY, 0644);
	if (m_pHander.m_fd == -1) {
		TERR("打开文件出错 %s 文件可能不存在, errno: %d, %s", m_szFileName, errno, strerror(errno));
		return false;
	}
	if (m_pBase!=NULL)
	{
		assert(m_pBase!=NULL&&m_nLength!=0);
		if(munmap(m_pBase, m_nLength) != 0)
		{
			TERR("CMMapFile::reopen munmap fail: %s", strerror(errno));
			return false;
		}
		m_pBase = NULL;
		m_nLength = 0;
	}

    int mmap_flags = MAP_SHARED;
    if (index_lib::IndexConfigParams::getInstance()->isMemLock()) {
        TLOG("open %s with MAP_LOCKED flag!", m_szFileName);
        m_pBase = (char*)mmap(0, nLength, PROT_READ, mmap_flags | MAP_LOCKED, m_pHander.m_fd, nOffset);
        if ( m_pBase == MAP_FAILED && errno == EAGAIN) {
            TERR("open %s with MAP_LOCKED flag failed: %s!", m_szFileName, strerror(errno));
            m_pBase = (char*)mmap(0, nLength, PROT_READ, mmap_flags, m_pHander.m_fd, nOffset);
        }
    }
    else {
        m_pBase = (char*)mmap(0, nLength, PROT_READ, mmap_flags, m_pHander.m_fd, nOffset);
    }

	if (m_pBase == MAP_FAILED)
	{
		TERR("CMMapFile::reopen 文件 %s 映射内存文件出错! error code:%d  error info:%s", m_szFileName, errno, strerror(errno));
		m_pBase = NULL;
		return false;
	}
	m_nLength = nLength;
	m_nPos = 0;
	madvise(m_pBase, m_nLength, MADV_SEQUENTIAL);   //Expect  page  references in sequential order.
	if(::close(m_pHander.m_fd) != 0){
		TERR("CMMapFile: close file fail: %s", strerror(errno));
	}else{
		m_pHander.m_fd = -1;
	}

	return true;
}

bool CMMapFile::create(const char* pszName, size_t nLength)
{
	assert(m_pBase==NULL);
	assert(pszName!=NULL && nLength>0);
#ifdef _WIN32
   m_pHander.m_hFile = ::CreateFile(pszName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (m_pHander.m_hFile == INVALID_HANDLE_VALUE) {
		TERR("创建文件出错, %s 路径错误不存在!", pszName);
		return false;
	}
	m_nLength = nLength;
	m_pHander.m_hMapFile = ::CreateFileMapping(m_pHander.m_hFile, NULL, /*PAGE_WRITECOPY*/PAGE_READWRITE/*|SEC_COMMIT*/, 0, static_cast<DWORD>(nLength), NULL);
	if (m_pHander.m_hMapFile==NULL) {
		DWORD nError = ::GetLastError();
		if (nError == ERROR_ALREADY_EXISTS)
			TERR("要映射的文件对象已经被映射.");
		else
			TERR("创建映射文件 %s 失败.", pszName);
		::CloseHandle(m_pHander.m_hFile);
		m_pHander.m_hFile = NULL;
		m_nLength = 0;
		return false;
	}
	m_pBase = (char *)::MapViewOfFile(m_pHander.m_hMapFile, FILE_MAP_ALL_ACCESS, 0, 0, 0);
	if (m_pBase==NULL) {
		TERR("创建映射视图 %s 失败, 空间大小 %d.", pszName, m_nLength);
		::CloseHandle(m_pHander.m_hMapFile);
		::CloseHandle(m_pHander.m_hFile);
		m_pHander.m_hFile = m_pHander.m_hMapFile = NULL;
		::DeleteFile(pszName);
		m_nLength = 0;
		return false;
	}
#else
        if (m_pHander.m_fd > 0)
        {
                if(::close(m_pHander.m_fd) != 0){
                        TERR("CMMapFile: close file fail: %s", strerror(errno));
                }else{
                        m_pHander.m_fd = -1;
                }
        }

	if ((m_pHander.m_fd=::open(pszName, O_RDWR|O_CREAT, 0644))==-1) {
		TERR("创建文件出错, %s 路径错误不存在!", pszName);
		return false;
	}
	m_nLength = nLength;
	if(lseek(m_pHander.m_fd, m_nLength-1, SEEK_SET) == -1){
		TERR("lseek error. seek len=%lu. %s", m_nLength-1, strerror(errno));
		::close(m_pHander.m_fd);
		m_pHander.m_fd = 0;
		m_nLength = 0;
		return false;
	}
	if(::write(m_pHander.m_fd, "", 1) != 1){
	  	int nError = errno;
	  	TERR("文件截取定长 %lu 时出错. %s", m_nLength, strerror(nError));
		::close(m_pHander.m_fd);
	  	m_pHander.m_fd = 0;
	  	m_nLength = 0;
		if(nError == ENOSPC)
		  	abort();
		else
		  	return false;
	}

    int mmap_flags = MAP_SHARED;
    if (index_lib::IndexConfigParams::getInstance()->isMemLock()) {
        TLOG("open %s with MAP_LOCKED flag!", pszName);
        m_pBase = (char*)mmap(0, m_nLength, PROT_READ|PROT_WRITE, mmap_flags | MAP_LOCKED, m_pHander.m_fd, 0);
        if (m_pBase == MAP_FAILED && errno == EAGAIN) {
            TERR("open %s with MAP_LOCKED flag failed: %s!", pszName, strerror(errno));
            m_pBase = (char*)mmap(0, m_nLength, PROT_READ|PROT_WRITE, mmap_flags, m_pHander.m_fd, 0);
        }
    }
    else {
        m_pBase = (char*)mmap(0, m_nLength, PROT_READ|PROT_WRITE, mmap_flags, m_pHander.m_fd, 0);
    }

	if (m_pBase == MAP_FAILED) {
		TERR("文件 %s 影射内存文件出错!", pszName);
		::close(m_pHander.m_fd);
		m_pHander.m_fd = -1;
		m_nLength = 0;
		remove(pszName);
		return false;
	}
	madvise(m_pBase, m_nLength, MADV_SEQUENTIAL);		//Expect  page  references in sequential order.
//	if(::close(m_pHander.m_fd) != 0){
//		TERR("CMMapFile: close file fail: %s", strerror(errno));
//	}else{
//		m_pHander.m_fd = -1;
//	}
#endif
	return true;
}

bool CMMapFile::close()
{
  	bool bRes = true;
#ifdef _WIN32
	if (m_pBase!=NULL) {
		UnmapViewOfFile(m_pBase);
		CloseHandle(m_pHander.m_hMapFile);
		CloseHandle(m_pHander.m_hFile);
		m_pBase = NULL;
		m_nLength = 0;
		m_pHander.m_hFile = m_pHander.m_hMapFile = NULL;
	}
#else
	if (m_pBase!=NULL) {
		assert(m_pBase!=NULL&&m_nLength!=0);
		if(munmap(m_pBase, m_nLength) != 0){
			TERR("CMMapFile: munmap fail: %s", strerror(errno));
			bRes = false;
		}
		if(m_pHander.m_fd != -1 && ::close(m_pHander.m_fd) != 0){
		  	TERR("CMMapFile: close file fail: %s", strerror(errno));
			bRes = false;
		}
		m_pBase = NULL;
		m_nLength = 0;
		m_pHander.m_fd = 0;
	}
#endif
	if (m_szFileName != NULL) {
		free(m_szFileName);
		m_szFileName = NULL;
	}
	return bRes;
}

bool CMMapFile::flush()
{
#ifdef _WIN32
#else
	if (m_pBase!=NULL) {
		assert(m_pBase!=NULL&&m_nLength!=0);
		int nRet = msync(m_pBase, m_nLength, MS_ASYNC);		//MS_ASYNC表示同步可以不立即执行
		if (nRet == -1){
		  	TERR("CMMapFile: msync fail: %s", strerror(errno));
			return false;
		}
	}
#endif
	return true;
}


/*
*	写文件
*/
size_t CMMapFile::write(void *src, size_t len)
{
	if (len<=0) {
		return 0;
	}
	size_t wlen = len;
	if (m_nPos+len>m_nLength) {
		wlen = m_nLength - m_nPos;
	}
	if (wlen>0) {
		memcpy(m_pBase+m_nPos, src, wlen);
		m_nPos += wlen;
	}
	return wlen;
}

/*
*	读文件
*/
size_t CMMapFile::read(void *src, size_t len)
{
	size_t rlen;
	if (m_nPos>=m_nLength) {
		return 0;
	}
	// 如果要读的字节数超过实际的字节数，则只返回实际的字节数
	if (m_nPos+len>m_nLength) {
		rlen = m_nLength - m_nPos;
	} else {
		rlen = len;
	}
	memcpy(src, m_pBase+m_nPos, rlen);
	m_nPos += rlen;
	return rlen;
}

/*
*	设置文件位置
*/
bool CMMapFile::setPosition(size_t pos, int whence)
{
	if (whence == SEEK_CUR) {
		pos += m_nPos;
	} else if (whence == SEEK_END) {
		pos += m_nLength;
	}
	if (pos < 0) {
		return false;
	}
	m_nPos = pos;
	return true;
}

/*
* 得到文件当前位置
*/
size_t CMMapFile::getPosition() {
	return m_nPos;
}

}	//namespace store

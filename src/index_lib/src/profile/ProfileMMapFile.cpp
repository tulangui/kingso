#include "index_lib/ProfileMMapFile.h"
#include "index_lib/IndexConfigParams.h"

namespace index_lib
{

ProfileMMapFile::ProfileMMapFile(char* pszName, bool unload)
{
    if (pszName == NULL) {
        _szFileName[0] = '\0';
    }
    else {
        snprintf( _szFileName, PATH_MAX, "%s", pszName);
    }

    _bReadOnly = false;
    _bPrivate  = false;
    _unload    = unload;
    _nLength   = 0;
    _pBase     = NULL;
    _fd        = -1;
    _mmap      = false;
}

ProfileMMapFile::~ProfileMMapFile()
{
    close();
}

bool ProfileMMapFile::open(bool bReadOnly, bool bPrivate, size_t length)
{
    _mmap = false;
    if (_szFileName[0] == '\0') {
        return false;
    }

    if (length > 0 && _bReadOnly) {
        return false;
    }

    if (_pBase != NULL) {
        return false;
    }

    if (_fd > 0 && ::close(_fd) != 0) {
        TERR("ProfileMMapFile: close file fail:%s", strerror(errno));
    }
    _fd = -1;

    _bReadOnly = bReadOnly;
    _bPrivate  = bPrivate;

    if (length > 0) {
        // create file
        if ((_fd = ::open(_szFileName, O_RDWR|O_CREAT, 0644)) == -1) {
            TERR("ProfileMMapFile: open file fail:%s", strerror(errno));
            return false;
        }

        if (lseek(_fd, (length - 1), SEEK_SET) == -1) {
            TERR("ProfileMMapFile: lseek error. seek len=%lu. %s", (length - 1), strerror(errno));
            ::close(_fd);
            _fd = -1;
            return false;
        }

        if (::write(_fd, "", 1)!=1) {
            TERR("ProfileMMapFile: write 0 to new file fail:%s", strerror(errno));
            ::close(_fd);
            _fd = -1;
            return false;
        }
        _nLength = length;
    }
    else {
        // open file
        if (bReadOnly) {
            _fd = ::open(_szFileName, O_RDONLY, 0644);
        }
        else {
            _fd = ::open(_szFileName, O_RDWR, 0644);
        }

        if (_fd == -1) {
            TERR("ProfileMMapFile: open file fail:%s", strerror(errno));
            return false;
        }

        struct stat statInfo;
        if (fstat(_fd, &statInfo) < 0) {
            TERR("ProfileMMapFile: fstat fail:%s", strerror(errno));
            ::close(_fd);
            _fd = -1;
            return false;
        }
        _nLength = statInfo.st_size; 
    }

    int prot, flags;
    if (_bReadOnly) {
        prot = PROT_READ;
    }
    else {
        prot = PROT_READ|PROT_WRITE;
    }

    if (_bPrivate) {
        flags = MAP_PRIVATE;
    }
    else {
        flags = MAP_SHARED;
    }

    if (IndexConfigParams::getInstance()->isMemLock()) {
        TLOG("open %s with MAP_LOCKED flag!", _szFileName);
        _pBase = (char*)mmap(0, _nLength, prot, flags | MAP_LOCKED, _fd, 0);
        if (_pBase == MAP_FAILED && errno == EAGAIN) {
            TERR("ProfileMMapFile: mmap failed with MAP_LOCKED flag: %s", strerror(errno));
            _pBase = (char*)mmap(0, _nLength, prot, flags, _fd, 0);
        }
    }
    else {
        _pBase = (char*)mmap(0, _nLength, prot, flags, _fd, 0);
    }

    if (_pBase == MAP_FAILED) {
        TERR("ProfileMMapFile: mmap failed:%s", strerror(errno));
        ::close(_fd);
        _fd      = -1;
        _nLength = 0;
        return false;
    }

    if (_pBase != NULL && _fd > 0 && _nLength != 0) {
        _mmap = true;
    }

    madvise(_pBase, _nLength, MADV_SEQUENTIAL);
    return _mmap;
}


size_t ProfileMMapFile::makeNewPage(size_t nPageSize)
{
    if (!_mmap) {
        return 0;
    }

    _mmap = false;
    size_t nOldLength = _nLength;
    if (ftruncate(_fd, nOldLength + nPageSize) == -1) {
        TERR("ProfileMMapFile: ftruncate failed:%s", strerror(errno));
        return 0;
    }

    char* pNew = (char*)mremap((void*)_pBase, nOldLength, (nOldLength + nPageSize), MREMAP_MAYMOVE);
    if (pNew == MAP_FAILED) {
        TERR("ProfileMMapFile: mremap failed:%s", strerror(errno));
        ::close(_fd);
        _fd      = -1;
        _nLength = 0;
        return 0;
    }
    _mmap    = true;
    _pBase   = pNew;
    _nLength = nOldLength+nPageSize;
    return _nLength;
}

/**
 * 重置映射文件大小
 * @param  size     重置文件大小
 * @return          false, 重置失败;true, 重置成功
 */
bool ProfileMMapFile::resize(size_t size)
{
    if (!_mmap) {
        return false;
    }

    _mmap = false;
    uint32_t nOldLength = _nLength;
    if (ftruncate(_fd, size) == -1) {
        TERR("ProfileMMapFile: resize ftruncate failed:%s", strerror(errno));
        return false;
    }

    char* pNew = (char*)mremap((void*)_pBase, nOldLength, size, MREMAP_MAYMOVE);
    if (pNew == MAP_FAILED) {
        TERR("ProfileMMapFile: resize mremap failed:%s", strerror(errno));
        ::close(_fd);
        _fd      = -1;
        _nLength = 0;
        return false;
    }
    _mmap    = true;
    _pBase   = pNew;
    _nLength = size;
    return true;
}

bool ProfileMMapFile::flush(bool sync)
{
    if (_bReadOnly == true || !_mmap) {
        return false;
    }

    int nRet;
    if (!sync) {
        nRet = msync(_pBase, _nLength, MS_ASYNC);
    }
    else {
        nRet = msync(_pBase, _nLength, MS_SYNC);
    }

    if (nRet == -1) {
        TERR("ProfileMMapFile: msync fail:%s", strerror(errno));
        return false;
    }

    return true;
}

void ProfileMMapFile::preload()
{
    if (!_mmap) {
        return;
    }

    size_t fsize = 0;
    size_t nSize = 2*1024*1024;
    char   *pBuf = new char[nSize];
    for (size_t i = 0; i < _nLength; ) {
        fsize += this->read(i, pBuf, nSize);
        i += nSize;
    }

    TLOG("IndexLib profile preload:%s,%lu", _szFileName, fsize);
    delete [] pBuf;
}

bool ProfileMMapFile::release()
{
    if (_unload && _fd > 0) {
        if (posix_fadvise(_fd, 0, 0, POSIX_FADV_DONTNEED) != 0) {
            TERR("ProfileMMapFile: posix_fadvise fail:%s", strerror(errno));
        }
    }
    return true;
}

bool ProfileMMapFile::close()
{
    bool bRes = true;
    if (!_mmap) {
        return false;
    }
    
    _mmap = false;
    if (munmap(_pBase, _nLength)!=0) {
        TERR("ProfileMMapFile: munmap fail:%s", strerror(errno));
        bRes = false;
    }

    if (_fd != -1 && ::close(_fd) != 0) {
        TERR("ProfileMMapFile: close file fail:%s", strerror(errno));
        bRes = false;
    }

    _pBase   = NULL;
    _nLength = 0   ;
    _fd      = -1  ;

    return bRes;
}

/**
 * 向磁盘中写入实际数据
 * @param  addr    映射地址
 * @param  len     数据长度
 */
int   ProfileMMapFile::syncToDisk(char * addr, size_t len)
{
    if (!_mmap || len == 0 || addr == NULL) {
        return -1;
    }

    size_t pos = (addr - _pBase);
    //printf("fd pos:%lu, %s,addr: %lu, base: %lu\n", pos, _szFileName, addr, _pBase);
    if (lseek(_fd, pos, SEEK_SET) == -1) {
        TERR("ProfileMMapFile: syncToDisk error. seek len=%lu. %s", pos, strerror(errno));
        return -1;
    }

    if (::write(_fd, addr, len) != (ssize_t)len) {
        TERR("ProfileMMapFile: syncToDisk fail. write error:%s", strerror(errno));
        return -1;
    }

    return 0;
}

size_t ProfileMMapFile::write(size_t offset, const void *src, size_t len)
{
    if (!_mmap || len == 0 || src == NULL || offset >= _nLength) {
        return 0;
    }

    size_t wlen = len;
    if ((offset + len) > _nLength) {
        wlen = _nLength - offset;
    }

    if (wlen > 0) {
        memcpy((_pBase + offset), src, wlen);
    }

    return wlen;
}

size_t ProfileMMapFile::read(size_t offset, void *src, size_t len)
{
    size_t rlen;
    if (!_mmap || src == NULL || len == 0 || offset >= _nLength) {
        return 0;
    }

    if (offset+len > _nLength) {
        rlen = _nLength - offset;
    }
    else {
        rlen = len;
    }
   
    memcpy(src, (_pBase + offset), rlen);
    return rlen;
}

}

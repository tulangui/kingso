/** ProfileMMapFile.h
 *******************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision:  $
 *
 * $LastChangedDate: 2011-03-16 09:50:55 +0800 (Wed, 16 Mar 2011) $
 *
 * $Id: ProfileMMapFile.h  2011-03-16 01:50:55 pujian $
 *
 * $Brief: 内存映射功能包装类的实现 $
 *******************************************************************
*/
#ifndef _KINGSO_INDEXLIB_PROFILE_MMAP_FILE_H
#define _KINGSO_INDEXLIB_PROFILE_MMAP_FILE_H

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

#include "IndexLib.h"


namespace index_lib 
{

/* ProfileMMapFile class */
class ProfileMMapFile
{
public:
    /**
     * constructor
     * @param pszName  映射目标文件名
     * @param unload   是否可以被去除加载
     */
    ProfileMMapFile(char* pszName, bool unload = false);

    /**
     * destructor
     */
    ~ProfileMMapFile();

    /**
     * 打开并创建目标文件的内存映射
     * @param bReadOnly  内存映射是否只读
     * @param bPraivate  内存映射是否为私有
     * @param length     映射长度（为0时直接映射全部文件内容，不为0时创建文件并映射)
     * @return           true,映射成功;false, 映射失败
     */
    bool open(bool bReadOnly = false, bool bPrivate = false, size_t length = 0);

    /**
     * 扩展映射文件
     * @param  nPageSize   扩展的大小（byte）
     * @return             0, 扩展失败；other, 扩展后的大小
     */
    size_t makeNewPage(size_t nPageSize);

    /**
     * 重置映射文件大小
     * @param  size     重置文件大小
     * @return          false, 重置失败;true, 重置成功
     */
    bool resize(size_t size);

    /**
     * 将内存映射数据dump到硬盘文件
     * @param  sync     是否同步dump
     * @return          dump是否成功
     */
    bool flush(bool sync = false);

    /**
     * 预加载映射文件内容
     */
    void preload();

    /**
     * 去除加载文件的系统cache，必须创建对象时unload=true时有效
     * @return    true,OK; false,ERROR
     */
    bool release();

    /**
     * 关闭文件的内存映射
     * @return    true,OK; false,ERROR
     */
    bool close();

    /**
     * 根据偏移量获取目标地址
     * @param offset   偏移量（byte）
     * @return         偏移量对应的地址
     */
    inline char* offset2Addr(register size_t offset)
    {
        return (_pBase + offset);
    }

    /**
     * 根据偏移量获取目标地址(带有异常检验)
     * @param offset   偏移量（byte）
     * @return         偏移量对应的地址
     */
    inline char* safeOffset2Addr(size_t offset)
    {
        if (unlikely(!_mmap || offset >= _nLength)) {
            return NULL;
        }
        return (_pBase + offset);
    }

    /**
     * 返回映射基地址
     * @return         映射空间的起始地址
     */
    inline char* getBaseAddr() {
        return _pBase;
    }

    /**
     * 获取内存映射成功与否
     * @return       true, 映射成功；false，映射失败
     */
    inline bool getMMapStat() {
        return _mmap;
    }

    /**
     * 获取内存映射长度
     * @return      映射长度
     */
    inline size_t getLength() {
        return _nLength;
    }
    
    /**
     * 向目标偏移位置写入数据
     * @param  offset   写入目标偏移位置
     * @param  src      写入数据的buffer
     * @param  len      写入数据的长度
     * @return          实际写入的数据长度
     */
    size_t write(size_t offset, const void *src, size_t len);

    /**
     * 从目标偏移位置读入的数据
     * @param  offset  读取的目标偏移位置
     * @param  src     保存读取数据的buffer
     * @param  len     读取buffer的size
     * @return         实际读取的数据长度
     */
    size_t read(size_t offset, void *src, size_t len);

    /**
     * 向磁盘中写入实际数据
     * @param  addr    映射地址
     * @param  len     数据长度
     * @return         -1，ERROR；0，OK
     */
    int   syncToDisk(char * addr, size_t len);

protected:
    char   _szFileName[PATH_MAX];                     //文件名
    bool   _bReadOnly;                                //是否只读
    bool   _bPrivate;                                 //是否私有
    bool   _unload;                                   //是否可以去除加载（清系统cache)
    size_t _nLength;                                  //映射的字节长度
    char*  _pBase;                                    //映射的起始地址
    int    _fd;                                       //映射文件的fd
    bool   _mmap;                                     //映射状态
};

}

#endif //_KINGSO_INDEXLIB_PROFILE_MMAP_FILE_H


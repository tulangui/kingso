/**
 * @file MemPoolDefine.h
 * @brief some defines related with MemPool
 *
 * @version 1.0.0
 * @date 2008.12.26
 * @author ruijie.guo
 */

#ifndef __MEMPOOLDEFINE_H
#define __MEMPOOLDEFINE_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>

namespace pool{

class MemStat
{
public:
    MemStat()
        : m_nBytesUsed(0)
        , m_nTotalBytes(0)
    {}

public:
    /** get/set number of bytes used */
    inline size_t getBytesUsed()const{return m_nBytesUsed;}
    inline void setBytesUsed(const size_t nBytesUsed){m_nBytesUsed = nBytesUsed;}
    
    /** get number of free bytes*/
    inline size_t getBytesFree()const{return m_nTotalBytes - m_nBytesUsed;}
    
    /** get total bytes */
    inline size_t getTotalBytes()const{return m_nTotalBytes;}
    inline void setTotalBytes(const size_t nTotalBytes){m_nTotalBytes = nTotalBytes;}
    
    /** get usage ratio */
    inline double getUsageRatio()const{return ( (m_nTotalBytes == 0)? 0.0 : ( (double)(m_nBytesUsed) / (double)( m_nTotalBytes ) ) );}
protected:
    size_t m_nBytesUsed;    ///number of bytes used
    size_t m_nTotalBytes;   ///number of bytes allocated
};

class MemChunk
{
public:
    MemChunk(size_t nSize = 0)
        : m_nSize(nSize)
    {
        m_nPos = m_nHolder = sizeof(MemChunk);
		if(m_nSize < m_nPos)
			m_nSize = m_nPos;
    }

public:
    /**
     * malloc from chunk
     * @param nBytes number of bytes to malloc
     */
    inline void* malloc(const size_t nBytes)
    {
        if( m_nSize - m_nPos < nBytes)
            return NULL;
        void* ptr = (void*)( (char*)(this) + m_nPos );
        m_nPos += nBytes;
        return ptr;
    }

    /** 
     * is there any space to allocate
     * @return true if no free space
     */
    inline bool isEmpty(){return (m_nPos <= m_nSize);}

    /** get total size of chunk */
    inline size_t getSize()const{return m_nSize;}

    /** get current allocation position */
    inline size_t getPos()const{return m_nPos;}

	/** get free size */
	inline size_t getFreeSize(){return m_nSize - m_nPos;}
	
    /** get used size */
	inline size_t getUsedSize(){return m_nPos - m_nHolder;}
    
    /** reset chunk to free stat */
    inline void reset(){m_nPos = m_nHolder;}
protected:
    size_t m_nSize;   //size of the chunk
    size_t m_nPos;    //current allocation position 
    size_t m_nHolder;///size of this object         
};

class ChainedMemChunk : public MemChunk
{
public:
    ChainedMemChunk(size_t nSize)
        : MemChunk(nSize)
        , m_pNext(NULL)
    {
        m_nPos = m_nHolder = sizeof(ChainedMemChunk);
		if(m_nSize < m_nPos)
			m_nSize = m_nPos;
    };
public:
    /** get next chunk*/
    inline ChainedMemChunk* next()const{return m_pNext;}
    inline ChainedMemChunk*& next(){return m_pNext;}
protected:
    ChainedMemChunk*   m_pNext;
};

}

#endif

/**
 * @file MonotoneChunkPolicy.h
 * @brief declare MonotoneChunkPolicy class used for memory allocation of Pool 
 *
 * @version 1.0.1
 * @date 2009.1.19
 * @author ruijie.guo
 */

#ifndef __MONOTONECHUNKPOLICY_H
#define __MONOTONECHUNKPOLICY_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <new>
#include "PoolDefine.h"
#include "Allocators.h"

namespace pool{

///Fixed size chunk policy
template<typename Allocator = NewDeleteAllocator >
class MonotoneChunkPolicy
{
public:
    typedef Allocator allocator;
public:
    MonotoneChunkPolicy(const size_t nChunkSize = DEFAULT_CHUNK_SIZE);
    ~MonotoneChunkPolicy();
public:
    /** 
     * alloc a chunk
     * @param nBytes number of bytes to allocate
     * @return allocated chunk
     */
    MemChunk* allocChunk(const size_t nBytes);

    /**
     * get mempool MemStatus
     * @return MemStatus of mempool
     */
    const MemStat getMemStat()const;

    /**
     * release allocated chunks
     * @return total size of chunks
     */
    size_t  release();

	/**
	 * set memory chunk size
	 * @param nChunkSize chunk size
	 */
	void setChunkSize(const size_t nChunkSize){m_nChunkSize = nChunkSize;}
protected:
    ChainedMemChunk*  m_pMemChunk;
	size_t	m_nChunkSize;///chunk size
public:
    const static size_t DEFAULT_CHUNK_SIZE = 128 * 1024;//128k
};

//////////////////////////////////////////////////////////////////////////
///MonotoneChunkPolicy
template< typename Allocator >
MonotoneChunkPolicy<Allocator>::MonotoneChunkPolicy(const size_t nChunkSize)
    : m_pMemChunk(NULL)
	, m_nChunkSize(nChunkSize)
{
}

template< typename Allocator >
MonotoneChunkPolicy<Allocator>::~MonotoneChunkPolicy()
{
    release();
}

template< typename Allocator >
MemChunk* MonotoneChunkPolicy<Allocator>::allocChunk(const size_t nBytes)
{
	size_t nAllocSize = nBytes + sizeof(ChainedMemChunk);
	if(nAllocSize < m_nChunkSize)
		nAllocSize = m_nChunkSize;
    if(!m_pMemChunk)
        m_pMemChunk = new ( Allocator::malloc( nAllocSize ) ) ChainedMemChunk(nAllocSize);
    else
    {
        ChainedMemChunk* pMemChunk = new ( Allocator::malloc( nAllocSize ) ) ChainedMemChunk( nAllocSize );
        pMemChunk->next() = m_pMemChunk;
        m_pMemChunk = pMemChunk;
    }
    return m_pMemChunk;
}

template< typename Allocator >
size_t MonotoneChunkPolicy<Allocator>::release()
{
    ChainedMemChunk* pChunk = m_pMemChunk;
    size_t nTotalSize = 0;
    ChainedMemChunk* pChunk2 = NULL;
    while(pChunk){
        nTotalSize += pChunk->getUsedSize();
        pChunk2 = pChunk;
        pChunk = pChunk2->next();
        Allocator::free( (void*)pChunk2,pChunk2->getSize() );
    }
    m_pMemChunk = NULL;
    return nTotalSize;
}

template< typename Allocator >
const MemStat MonotoneChunkPolicy<Allocator>::getMemStat()const
{
    ChainedMemChunk* pChunk = m_pMemChunk;
    size_t nTotalSize = 0;
    size_t nUsedSize = 0;
    while(pChunk){
        nTotalSize += pChunk->getSize();
        nUsedSize += pChunk->getUsedSize();
        pChunk = pChunk->next();
    }
    MemStat stat;
    stat.setTotalBytes(nTotalSize);
    stat.setBytesUsed(nUsedSize);
    return stat;
}

} ///end of namespace pool

#endif

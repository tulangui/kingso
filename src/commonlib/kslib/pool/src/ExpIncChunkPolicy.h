/**
 * @file ExpIncChunkPolicy.h
 * @brief declare ExpIncChunkPolicy classes used for memory allocation of MemPool 
 *
 * @version 1.0.1
 * @date 2009.1.19
 * @author ruijie.guo
 */

#ifndef __EXPINCCHUNKPOLICY_H
#define __EXPINCCHUNKPOLICY_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <new>
#include "PoolDefine.h"
#include "Allocators.h"

namespace pool{

///exponential increase chunk policy
template<typename Allocator = NewDeleteAllocator>
class ExpIncChunkPolicy
{
public:
	typedef Allocator allocator;
public:
	ExpIncChunkPolicy();
	~ExpIncChunkPolicy();
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
	ChainedMemChunk*  m_pMemChunk;///chained memory chunks
	size_t	m_nChunkSize;///chunk size
	size_t	m_nNextExp;	 
};
    
//////////////////////////////////////////////////////////////////////////
///ExpIncChunkPolicy
template< typename Allocator >
ExpIncChunkPolicy<Allocator>::ExpIncChunkPolicy()
	: m_pMemChunk(NULL)
	, m_nChunkSize(128*1024)
	, m_nNextExp(0)
{
}

template< typename Allocator >
ExpIncChunkPolicy<Allocator>::~ExpIncChunkPolicy()
{
	release();
}

template< typename Allocator >
MemChunk* ExpIncChunkPolicy<Allocator>::allocChunk(const size_t nBytes)
{
	size_t nAllocSize = (m_nChunkSize << m_nNextExp);
	if(nAllocSize < nBytes + sizeof(ChainedMemChunk))
		nAllocSize = nBytes + sizeof(ChainedMemChunk);
	m_nNextExp++;
	if(!m_pMemChunk)
		m_pMemChunk = new ( Allocator::malloc( nAllocSize ) ) ChainedMemChunk(nAllocSize);
	else{
		ChainedMemChunk* pMemChunk = new ( Allocator::malloc( nAllocSize ) ) ChainedMemChunk( nAllocSize );
		pMemChunk->next() = m_pMemChunk;
		m_pMemChunk = pMemChunk;
	}
	return m_pMemChunk;
}

template< typename Allocator >
size_t ExpIncChunkPolicy<Allocator>::release()
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
	m_nNextExp = 0;
	return nTotalSize;
}

template< typename Allocator >
const MemStat ExpIncChunkPolicy<Allocator>::getMemStat()const
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

}///end of namespace pool

#endif

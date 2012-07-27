/**
 * @file AdaptiveStatPolicy.h
 * @brief declare AdaptiveStatPolicy classes used for memory allocation of MemPool 
 *
 * @version 1.0.1
 * @date 2009.1.19
 * @author ruijie.guo
 */

#ifndef __ADAPTIVESTATPOLICY_H
#define __ADAPTIVESTATPOLICY_H

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <new>
#include "PoolDefine.h"
#include "Allocators.h"

namespace pool{

///AdaptiveStatPolicy
template<typename Allocator = MMapAllocator,size_t ExceptionPercent = 80 >
class AdaptiveStatPolicy
{
public:
	typedef Allocator allocator;

	const static size_t ADJUST_FREQ = 1000; ///adjust the resident chunk size every ADJUST_FREQ release times
    const static size_t DEFAULT_INIT_CHUNK_SIZE = 128 * 1024;///128 KB
protected:
	class Statistic
	{
	public:
		const static size_t CHUNK_MINSIZE = 524288;//512 KB
		const static size_t STAT_CHUNKS = 12;
	public:
		Statistic()
		{	
			reset();
		}
	public:
		/** 
		 * reset statistic state
		 */
		void reset()
		{
			m_nTotalRefs = 0;
			m_slots[0].ref = 0;
			m_slots[0].size = CHUNK_MINSIZE;
			for (size_t i = 1; i < STAT_CHUNKS; i++)
			{
				m_slots[i].ref = 0;
				m_slots[i].size = (m_slots[i - 1].size << 1);
			}
		}

		/**
		 * number of bytes allocated
		 * @param nBytesAllocated number of bytes allocated
		 */
		void stat(size_t nBytesAllocated)
		{
			for (int i = STAT_CHUNKS - 1;i >=0; i--)
			{
				if(m_slots[i].size >= nBytesAllocated)
					m_slots[i].ref++;
				else break;
			}
			m_nTotalRefs++;
		}

		/**
		 * pridict the max size
		 * @param dbPercent percent of the size covered
		 */
		size_t pridict(double dbPercent)
		{
			size_t nThreshold = (size_t)(m_nTotalRefs * dbPercent);
			for (size_t i = 0;i < STAT_CHUNKS;i++)
			{
				if(m_slots[i].ref >= nThreshold)
					return m_slots[i].size;
			}
			return m_slots[0].size;
		}

		/** get total ref times */
		size_t getRefTimes(){return m_nTotalRefs;}
	protected:
		struct StatSlot  
		{
			size_t ref;	///reference of this slot
			size_t size;///size of this slot
		};
	protected:
		StatSlot m_slots[STAT_CHUNKS];	///slot array
		size_t	m_nTotalRefs;			///total reference
	};
public:
	AdaptiveStatPolicy(const size_t nInitChunkSize = DEFAULT_INIT_CHUNK_SIZE);
	~AdaptiveStatPolicy();
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

	MemChunk*  m_pResidentChunk;///resident chunk
	Statistic  m_statistic;

	size_t	m_nChunkSize;///chunk size
	size_t	m_nNextExp;	 
};

//////////////////////////////////////////////////////////////////////////
///AdaptiveStatPolicy
template< typename Allocator ,size_t ExceptionPercent>
AdaptiveStatPolicy<Allocator,ExceptionPercent>::AdaptiveStatPolicy(const size_t nInitChunkSize)
	: m_pMemChunk(NULL)
	, m_pResidentChunk(NULL)
	, m_nChunkSize(nInitChunkSize)
	, m_nNextExp(0)
{
}

template< typename Allocator ,size_t ExceptionPercent>
AdaptiveStatPolicy<Allocator,ExceptionPercent>::~AdaptiveStatPolicy()
{
	release();
	if(m_pResidentChunk){
        m_pResidentChunk->~MemChunk();      
		Allocator::free( (void*)m_pResidentChunk, m_pResidentChunk->getSize() );
		m_pResidentChunk = NULL;
	}
}

template< typename Allocator ,size_t ExceptionPercent>
MemChunk* AdaptiveStatPolicy<Allocator,ExceptionPercent>::allocChunk(const size_t nBytes)
{
	size_t nAllocSize = (m_nChunkSize << m_nNextExp);
	if(nAllocSize < nBytes + sizeof(ChainedMemChunk))
		nAllocSize = nBytes + sizeof(ChainedMemChunk);
	if(!m_pResidentChunk)
	{
		m_pResidentChunk = new (Allocator::malloc( nAllocSize ) ) MemChunk(nAllocSize);
		return m_pResidentChunk;
	}
	if(m_pResidentChunk->getFreeSize() > nBytes)
		return m_pResidentChunk;
	
	m_nNextExp++;
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

template< typename Allocator ,size_t ExceptionPercent >
size_t AdaptiveStatPolicy<Allocator,ExceptionPercent>::release()
{
	ChainedMemChunk* pChunk = m_pMemChunk;
	size_t nTotalSize = 0;
	ChainedMemChunk* pChunk2 = NULL;
	while(pChunk){
		nTotalSize += pChunk->getUsedSize();
		pChunk2 = pChunk;
		pChunk = pChunk2->next();
        pChunk2->~ChainedMemChunk();
		Allocator::free( (void*)pChunk2,pChunk2->getSize() );
	}
	m_pMemChunk = NULL;
	m_nNextExp = 0;

	if(m_pResidentChunk){
		nTotalSize += m_pResidentChunk->getUsedSize();
		m_pResidentChunk->reset();
	}
    if(nTotalSize == 0)
        return 0;
	m_statistic.stat(nTotalSize);
	if(m_statistic.getRefTimes() % ADJUST_FREQ == 0){ ///adjust resident chunk size
		double dbExp = (double)ExceptionPercent/100.0;
		while (dbExp > 1.0)
			dbExp = dbExp/10.0;
		size_t nResSize = m_statistic.pridict(dbExp);
		if(!m_pResidentChunk || nResSize != m_pResidentChunk->getSize()){
			if(m_pResidentChunk)
            {
                m_pResidentChunk->~MemChunk();              
				Allocator::free( (void*)m_pResidentChunk, m_pResidentChunk->getSize() );
            }            
			m_pResidentChunk = new ( allocator::malloc(nResSize) ) MemChunk( nResSize );
		}
	}

	return nTotalSize;
}

template< typename Allocator ,size_t ExceptionPercent>
const MemStat AdaptiveStatPolicy<Allocator,ExceptionPercent>::getMemStat()const
{
	ChainedMemChunk* pChunk = m_pMemChunk;
	size_t nTotalSize = 0;
	size_t nUsedSize = 0;
	while(pChunk){
		nTotalSize += pChunk->getSize();
		nUsedSize += pChunk->getUsedSize();
		pChunk = pChunk->next();
	}
	if(m_pResidentChunk){
		nTotalSize += m_pResidentChunk->getSize();
		nUsedSize += m_pResidentChunk->getUsedSize();
	}
	MemStat stat;
	stat.setTotalBytes(nTotalSize);
	stat.setBytesUsed(nUsedSize);
	return stat;
}

} //end of namespace pool

#endif


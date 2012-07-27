/*********************************************************************
 *
 * File:	hashmap容器,通用数据结构类
 * Version: $Id: HashMap.h,v 1.0 2007/05/19 09:19:24 victor Exp $
 * Desc:	bucket大小自动增长的hashmap,容器中的KEY, VALUE及hash算法
 *			可以通过模板自定义数据类型,当hash算法不指定时,可以根据数据类
 *			型默认hash算法
 * Log :
 * 			Create by victor, 2007/4/23
 *
 *********************************************************************/
#ifndef HASHMAP_H
#define HASHMAP_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "MemHeap.h"
#include "CommonAlgorithm.h"	//通用算法,KeyHash, KeyEqual类型的仿函数

/* 默认使用内存空间大小 */
#define DEF_HASHMAP_MEM_SIZE 256

#define MAX_SIZESTEP 24
extern unsigned int szHashMap_StepTable[];

/* hashmap节点结构 */
template <typename Key, typename Value>
struct SHashEntry {
	SHashEntry* _next;			//下一个节点(同一个entry的节点间形成单向链表)
	SHashEntry* _seq_next;		//将所有hashmap中的节点用双向链表连接起来,指向该链表中节点的下一个节点
	SHashEntry* _seq_prev;		//指向该链表中节点的上一个节点
	Key _key;					//hashmap容器中的key
	Value _value;				//hashmap容器中的value
};

/*
 *	模板hashmap类, Key用于hash的对象, Value哈希表中的元素对象, HashFunc函数函数, 默认为泛化的hash算法对象
 */
template< typename Key, typename Value, typename HashFunc = KeyHash<Key>, typename HashEqual = KeyEqual<Key> >
class CHashMap
{
public:
	//节点类型声明
	typedef SHashEntry<Key,Value> HashEntry;
	//用于优先队列排序的小于比较函数
	struct SLessThan {
		inline bool operator()(HashEntry *pEntry1, HashEntry *pEntry2) {
			return pEntry1->_key < pEntry2->_key;
		}
	};

private:
	//查找hash step表,决定hash size应该是多大比较合适
	inline unsigned int getHashSizeStep(unsigned int nHashSize)
	{
		if (nHashSize < szHashMap_StepTable[0])
			return 0;
		BinarySearch<unsigned int> binarySearch;
		return binarySearch(szHashMap_StepTable,0,MAX_SIZESTEP,nHashSize);
	}

private:
	bool m_bMultiKey;				//是否支持相同key,true表示允许多key
	unsigned int m_nHashSize;		//Bucket数组大小
	unsigned int m_nHashUsed;		//已经使用的元素个数
	unsigned int m_nRehashLimit;	//需要进行rehash限制值
	unsigned int m_nHashSizeStep;	//取得hash size的下标
	HashEntry **m_ppEntry;			//Bucket数组,存放一个个的entry指针

	HashFunc m_hasher;				//对key进行hash值计算的函数对象
	HashEqual m_comparator;			//对key进行比较的函数对象
	SHeap *m_pHeap;					//节点内存管理器

public:
	HashEntry *m_pFirst;			//所有元素链表的头指针
	HashEntry *m_pLast;				//所有元素链表的尾指针

public:
	/* 构造函数,hash size的大小默认为szHashMap_StepTable[2]的值 */
	CHashMap(unsigned char cHashSizeStep = 3);
	/* 构造函数, 直接指定需要多大的空间来存放元素 */
	CHashMap(unsigned int nHashSize);
	/* 析构函数 */
	virtual ~CHashMap(void);

public:
	/* 将容器设置为支持重复的多key */
	void setMultiKey(bool bMultiKey);
	/* 向容器中插入key和value */
	inline void insert(const Key& _key, const Value& _value);
	/* 根据key清空某一个元素 */
	inline void remove(const Key& _key);
	/* 清空容器中的所有元素 */
	void clear(void);
	/* 根据key查找value值,当key在容器中不存在时,返回none默认值 */
	inline Value& find(const Key& _key, Value& _none) const;
	inline Value& find(const Key& _key, Value& _none, bool &bExist) const;
	/* 根据key查找value值,当key在容器中不存在时,将key和value插入容器,同时返回value的值 */
	inline Value& find_or_insert(const Key& _key, Value& _value);
	/* 根据key返回对应的Entry,如果该容器支持multikey,则该方法可以查找相同key的元素 */
	inline HashEntry* lookupEntry(const Key& _key) const;
	/* 返回容器中元素的个数 */
	unsigned int size() const;
	/* 容器里元素的大小超过总容量的3/5,需要重新构造该hashmap */
	void rehash(unsigned int nHashSizeStep);
	/* printf hash size */
	void print(void);
};

/* 构造函数,hash size的大小默认为szHashMap_StepTable[2]的值 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
CHashMap<Key,Value,HashFunc,HashEqual>::CHashMap(unsigned char cHashSizeStep)
{
	m_nHashSizeStep = cHashSizeStep > MAX_SIZESTEP ? MAX_SIZESTEP : cHashSizeStep;
	m_nHashSize = szHashMap_StepTable[m_nHashSizeStep];
	m_nRehashLimit = (m_nHashSize * 3) / 5;
	m_nHashUsed = 0;
	m_bMultiKey = false;
	m_ppEntry = NULL;
	m_pFirst = m_pLast = NULL;
	typedef HashEntry *HashEntryPtr;
	m_ppEntry = new HashEntryPtr[m_nHashSize];
	assert(m_ppEntry != NULL);
	memset(m_ppEntry, 0x0, m_nHashSize*sizeof(HashEntryPtr));
	m_pHeap = suNewHeap(DEF_HASHMAP_MEM_SIZE);
}
/* 构造函数, 直接指定需要多大的空间来存放元素 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
CHashMap<Key,Value,HashFunc,HashEqual>::CHashMap(unsigned int nHashSize)
{
	m_nHashSizeStep = getHashSizeStep(nHashSize);
	m_nHashSize = szHashMap_StepTable[m_nHashSizeStep];
	m_nRehashLimit = (m_nHashSize * 3) / 5;
	m_nHashUsed = 0;
	m_bMultiKey = false;
	m_ppEntry = NULL;
	m_pFirst = m_pLast = NULL;

	typedef HashEntry *HashEntryPtr;
	m_ppEntry = new HashEntryPtr[m_nHashSize];
	assert(m_ppEntry != NULL);
	memset(m_ppEntry, 0x0, m_nHashSize*sizeof(HashEntry *));
	m_pHeap = suNewHeap(DEF_HASHMAP_MEM_SIZE);
}
/* 析构函数 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
CHashMap<Key,Value,HashFunc,HashEqual>::~CHashMap(void)
{
	clear();				//清空所有节点的内存
	delete[] m_ppEntry;
	m_ppEntry = NULL;
    if(m_pHeap)	 {
        suFreeHeap(m_pHeap);
    }
}
/* 将容器设置为支持重复的多key */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
void CHashMap<Key,Value,HashFunc,HashEqual>::setMultiKey(bool bMultiKey)
{
	m_bMultiKey = bMultiKey;
}

/* print */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
void CHashMap<Key,Value,HashFunc,HashEqual>::print()
{
	for (int32_t i=0; i<m_nHashSize; i++)
	{
		int32_t nSize = 0;
		HashEntry *pEntry = m_ppEntry[i];
		while (pEntry != NULL)
		{
			nSize++;
			pEntry = pEntry->_next;
		}
		printf("hash i=%d, nSize=%d.\n", i, nSize);
	}
	printf("Hash map size=%d.\n", this->size());
}

/* 向容器中插入key和value */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
void CHashMap<Key,Value,HashFunc,HashEqual>::insert(const Key& _key, const Value& _value)
{
	HashEntry *pMultiEntry = NULL;

	unsigned int nPos = m_hasher(_key) % m_nHashSize;			//计算key的hash值
	HashEntry *pEntry = m_ppEntry[nPos];
	while (pEntry != NULL && !m_comparator(_key,pEntry->_key))	//跟同hash的每一个元素相比较
		pEntry = pEntry->_next;
	if (pEntry != NULL) {		//该key已经存在
		if (m_bMultiKey) {		//如果允许重key,则要保留该指针作为下一个元素插入的位置
			pMultiEntry = pEntry;
		}
		else {					//否则不做任何操作
			return;
		}
	}

	pEntry = (HashEntry*)suAllocNode(m_pHeap, sizeof(HashEntry));	//从内存管理池中分配内存空间
	pEntry->_key = _key;
	pEntry->_value = _value;
	pEntry->_next = NULL;
	/* 将新元素插入到最后的位置 */
	if (pMultiEntry != NULL) {
		pEntry->_next = pMultiEntry->_next;
//		while (pMultiEntry->_next != NULL) {
//			pMultiEntry = pMultiEntry->_next;
//		}
		pMultiEntry->_next = pEntry;
	}
	else {
		pEntry->_next = m_ppEntry[nPos];
		m_ppEntry[nPos] = pEntry;
	}
	m_nHashUsed ++;

	/* 将新的元素插入元素序列中 */
	pEntry->_seq_next = NULL;
	if (m_pLast != NULL) {
		m_pLast->_seq_next = pEntry;
		pEntry->_seq_prev = m_pLast;
	}
	else {
		assert(m_pFirst == NULL);
		m_pFirst = pEntry;
		pEntry->_seq_prev = NULL;
	}
	m_pLast = pEntry;
	/* 判断容器是否超过限制需重新rehash */
	if (m_nHashUsed >= m_nRehashLimit && m_nHashSizeStep < MAX_SIZESTEP) {
		rehash(++m_nHashSizeStep);
	}
}
/* 容器里元素的大小超过总容量的3/5,需要重新构造该hashmap */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
void CHashMap<Key,Value,HashFunc,HashEqual>::rehash(unsigned int nHashSizeStep)
{
	typedef HashEntry *HashEntryPtr;
	m_nHashSize = szHashMap_StepTable[nHashSizeStep];
	m_nRehashLimit = (m_nHashSize * 3) / 5;
	HashEntry **ppNewEntries = new HashEntryPtr[m_nHashSize];
	memset(ppNewEntries, 0x0, m_nHashSize*sizeof(HashEntryPtr));

	unsigned int nHashKey;
	//遍历每一个bucket数据,将这些元素重新hash
	for (HashEntry *pEntry = m_pFirst; pEntry != NULL; pEntry = pEntry->_seq_next) {
		nHashKey = m_hasher(pEntry->_key) % m_nHashSize;
		pEntry->_next = ppNewEntries[nHashKey];
		ppNewEntries[nHashKey] = pEntry;
	}
	delete[] m_ppEntry;
	m_ppEntry = ppNewEntries;
}
/* 根据key返回对应的Entry,如果该容器支持multikey,则该方法可以查找相同key的所有元素 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
SHashEntry<Key,Value>* CHashMap<Key,Value,HashFunc,HashEqual>::lookupEntry(const Key& _key) const
{
	unsigned int nPos = m_hasher(_key) % m_nHashSize;			//计算key的hash值
	HashEntry *pEntry = m_ppEntry[nPos];
	while (pEntry != NULL && !m_comparator(_key,pEntry->_key))	//跟同hash的每一个元素相比较
		pEntry = pEntry->_next;

	return pEntry;
}
/* 根据key清空某一个元素 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
void CHashMap<Key,Value,HashFunc,HashEqual>::remove(const Key& _key)
{
	unsigned int nPos = m_hasher(_key) % m_nHashSize;
	HashEntry *pPrevEntry = NULL, *pCurEntry = m_ppEntry[nPos];

	while (pCurEntry != NULL && !m_comparator(_key,pCurEntry->_key)) {
		pPrevEntry = pCurEntry;
		pCurEntry = pCurEntry->_next;
	}

	if (pCurEntry != NULL) {	//考虑到可能是多key值的情况
		do {
			if (pPrevEntry == NULL) {					// 该entry的第一个元素
				assert(pCurEntry == m_ppEntry[nPos]);
				m_ppEntry[nPos] = pCurEntry->_next;
			} else {
				pPrevEntry->_next = pCurEntry->_next;
			}

			/* 从所有元素链表中,将该元素移除 */
			if (pCurEntry == m_pFirst) {
				m_pFirst = pCurEntry->_seq_next;
			}
			else if(pCurEntry->_seq_prev) {

				pCurEntry->_seq_prev->_seq_next = pCurEntry->_seq_next;
			}

			if (pCurEntry == m_pLast) {
				m_pLast = pCurEntry->_seq_prev;
			}
			else if(pCurEntry->_seq_next) {
				pCurEntry->_seq_next->_seq_prev = pCurEntry->_seq_prev;
			}
			m_nHashUsed --;
			pCurEntry = pCurEntry->_next;
		} while(m_bMultiKey&&( NULL != pCurEntry) && m_comparator(_key,pCurEntry->_key));
	}
}
/* 清空容器中的所有元素,但容器大小不做改变 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
void CHashMap<Key,Value,HashFunc,HashEqual>::clear(void)
{
	suResetHeap(m_pHeap);
	if (m_nHashUsed > 0) {
		memset(m_ppEntry, 0x0, m_nHashSize*sizeof(HashEntry *));
	}
	m_nHashUsed = 0;
	m_pFirst = m_pLast = NULL;
}
/* 根据key查找value值,当key在容器中不存在时,返回none默认值 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
Value& CHashMap<Key,Value,HashFunc,HashEqual>::find(const Key& _key, Value& _none) const
{
	HashEntry *pEntry = lookupEntry(_key);
	if (pEntry != NULL)
		return pEntry->_value;
	else
		return _none;
}
/* 根据key查找value值,当key在容器中不存在时,返回none默认值 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
Value& CHashMap<Key,Value,HashFunc,HashEqual>::find(const Key& _key, Value& _none, bool &bExist) const
{
  	bExist = false;
	HashEntry *pEntry = lookupEntry(_key);
	if (pEntry != NULL) {
	  	bExist = true;
		return pEntry->_value;
	}
	else
		return _none;
}
/* 根据key查找value值,当key在容器中不存在时,将key和value插入容器,同时返回value的值 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
Value& CHashMap<Key,Value,HashFunc,HashEqual>::find_or_insert(const Key& _key, Value& _value)
{
	unsigned int nPos = m_hasher(_key) % m_nHashSize;			//计算key的hash值
	HashEntry *pEntry = m_ppEntry[nPos];
	while (pEntry != NULL && !m_comparator(_key,pEntry->_key))	//跟同hash的每一个元素相比较
		pEntry = pEntry->_next;
	if (pEntry != NULL) {				//该key已经存在
		return pEntry->_value;
	}

	pEntry = (HashEntry*)suAllocNode(m_pHeap, sizeof(HashEntry));	//从内存管理池中分配内存空间
	pEntry->_key = _key;
	pEntry->_value = _value;
	/* 将新元素插入到合适的位置 */
	pEntry->_next = m_ppEntry[nPos];
	m_ppEntry[nPos] = pEntry;
	m_nHashUsed ++;

	/* 将新的元素插入元素序列中 */
	pEntry->_seq_next = NULL;
	if (m_pLast != NULL) {
		m_pLast->_seq_next = pEntry;
		pEntry->_seq_prev = m_pLast;
	}
	else {
		assert(m_pFirst == NULL);
		m_pFirst = pEntry;
		pEntry->_seq_prev = NULL;
	}
	m_pLast = pEntry;
	/* 判断容器是否超过限制需重新rehash */
	if (m_nHashUsed >= m_nRehashLimit && m_nHashSizeStep < MAX_SIZESTEP) {
		rehash(++m_nHashSizeStep);
	}

	return pEntry->_value;
}
/* 返回容器中元素的个数 */
template <typename Key, typename Value, typename HashFunc, typename HashEqual>
unsigned int CHashMap<Key,Value,HashFunc,HashEqual>::size() const
{
	return m_nHashUsed;
}

#endif

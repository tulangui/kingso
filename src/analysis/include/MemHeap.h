/****************************************************************************
 *
 *      File:   内存堆分配器
 *      Desc:   一个全局的内存堆分配器,所有内存使用者只分配不释放,只当进程结束
 *				时才全部释放堆内存
 *      Log:
 *              created by victor, 2007/4/23
 *
 ****************************************************************************/ 
#ifndef MEM_HEAP_H
#define MEM_HEAP_H

#include <stddef.h>
/////////////////////////////////////////////////////////////////////////////
//
// Data types
//
/////////////////////////////////////////////////////////////////////////////       
typedef struct _SMemPage {
	size_t iBytesUsed;			// number of bytes already used from this page
	size_t iPageSize;			// default value is specified in heap. can be changed, if necessary
	void *pMem;					// pointer to the memory block that to allocated
	struct _SMemPage *pNext;
	struct _SMemPage *pPrev;
} SMemPage;

typedef struct _SHeap {
  SMemPage *pHead;
  SMemPage *pCur;
  size_t iDefPageSize;
  size_t iNumOfPages;
} SHeap;

/////////////////////////////////////////////////////////////////////////////
//
// Prototypes
//
/////////////////////////////////////////////////////////////////////////////
#ifdef __cplusplus
extern "C" {
#endif

/* 创建内存池 */
SHeap *suNewHeap(size_t nSize);
/* 分配任意大小的内存空间,iAlign字节对齐大小,通常情况为sizeof(char*) */
void *suHeapAlloc(SHeap *pHeap, size_t iSize, size_t iAlign);
/* 为固定大小的内存需求分配内存空间,此函数不能与suHeapAlloc互相调用,此函数只适用于list,hashtable,hashmap等容器 */
void *suAllocNode(SHeap *pHeap, size_t iSize);
/* 将内存空间重新置位整理,并把其中较大块的内存页释放 */
void suResetHeap( SHeap *pHeap);
/* 释放整个内存池 */
void suFreeHeap( SHeap *pHeap);
size_t suGetHeapSize( SHeap *pHeap );
size_t suGetHeapUsedSize( SHeap *pHeap );
//char *suGetHeapDetail( char *szBuf, size_t iLen, SHeap *pHeap );

#ifdef __cplusplus
};
#endif


#endif  //_SE_HEAP_H

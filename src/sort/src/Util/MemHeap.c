/****************************************************************************
 *
 *      File:   su_heap.c
 *      Desc:   heap and memory page implementation
 *      Log:
 *              created by zwj, 04/18/2000
 *
 ******************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "MemHeap.h"

//////////////////////////////////////////////////////////////////////////////
//
//  SHeap *suNewHeap( int iDefPageSize );
//  Desc:  create a new heap
//      Params:
//         int iDefPageSize:	default page size for each page
//      Return:
//         SHeap *:	a pointer to the new heap; NULL if failed
//      Side Effect:
//
//////////////////////////////////////////////////////////////////////////////
SHeap *suNewHeap( size_t iDefPageSize )
{
	SHeap *pHeap;
	SMemPage *pPage;

	pHeap = (SHeap *)calloc(1, sizeof(SHeap));
	if ( pHeap == NULL ) {
		fprintf(stderr, "suNewHeap> calloc() failed for heap!\n");
		return NULL;
	}
	pHeap->iDefPageSize = iDefPageSize;
	//
	// allocate the first page
	//
	pPage = (SMemPage *)malloc(sizeof(SMemPage ));
	if ( pPage == NULL ) {
	    if(pHeap) {
            free(pHeap);
            pHeap = NULL;
        }
        fprintf(stderr, "suNewHeap> malloc() failed for page!\n");
		return NULL;
	}
	//
	// initialize the first page and the heap
	//
	pPage->iBytesUsed = 0;
	pPage->iPageSize = iDefPageSize;
	pPage->pNext = NULL;
	pPage->pPrev = NULL;

	pHeap->pHead = pHeap->pCur = pPage;
	pHeap->iNumOfPages = 1;
	//
	// allocate memory block for the first page
	//
	pPage->pMem = (void *)malloc(pPage->iPageSize);
	if (pPage->pMem == NULL) {
	    if(pHeap) {
            free(pHeap);
            pHeap = NULL;
        }
        fprintf(stderr,"suNewHeap> malloc() failed for page memory(size=%zd)!\n", pPage->iPageSize);
		return NULL;
	}

	return pHeap;
} // end of suNewHeap()

//////////////////////////////////////////////////////////////////////////////
//
//  void *suHeapAlloc( SHeap *pHeap, int iSize, int iAlign );
//  Desc:  allocate required amount of memory from the heap
//      Params:
//         SHeap *pHeap:  pointer to the heap where the memory to allocated
//	   int iSize:	  size (in byte) of the memory to be allocated
//	   int iAlign:	  alignment at the beginning of the memory block
//      Return:
//         void *:	a pointer to the allocated memory block; NULL if failed
//      Side Effect:
//
//////////////////////////////////////////////////////////////////////////////
void *suHeapAlloc(SHeap *pHeap, size_t iSize, size_t iAlign)
{
	void *pMem;
	size_t iOffset;
	SMemPage *pCur, *pPrev, *pNew;

	if (pHeap == NULL || iSize == 0) {
		//fprintf(stderr,"suHeapAlloc> alloc memory error!, alloc size = 0.\n");
		return NULL;
	}

	pCur = pHeap->pCur;
	pPrev = pCur->pPrev;
	iOffset = (iAlign - pCur->iBytesUsed % iAlign) % iAlign;
	//需要内存数超过默认页大小, 则为需求分配一个超过默认页大小的新的内存页
	if (iSize > pCur->iPageSize || (iSize > pHeap->iDefPageSize && pCur->iBytesUsed + iSize + iOffset > pCur->iPageSize))
	{
		pNew = (SMemPage *)malloc(sizeof(SMemPage));
        //allocate new memory of the iSize
		pNew->pMem = (void *)malloc(iSize);
		if (pNew->pMem == NULL)
		{
			free(pNew);
			fprintf( stderr, "suHeapAlloc> malloc() failed for large page memory(%zd)!\n", iSize);
			return NULL;
		}
		//set the used Bytes for:these bytes will be used soon
		pNew->iBytesUsed = pNew->iPageSize = iSize;

		// the new page will be inserted before the current page
		// next allocation will still in the current page.
		pNew->pPrev = pCur->pPrev;
		if (pNew->pPrev == NULL)
			pHeap->pHead = pNew;
		else
			pNew->pPrev->pNext = pNew;

		pNew->pNext = pCur;
		pCur->pPrev = pNew;

		pHeap->iNumOfPages ++;

		return pNew->pMem;
  	}
	else
	{
		//前一内存页是否有足够的空间,只向前看一页
		if (pPrev && pPrev->iBytesUsed + iSize + (iOffset = (iAlign - pPrev->iBytesUsed % iAlign) % iAlign) <= pPrev->iPageSize) {
			pCur = pPrev;
		}
		else if (pCur->iBytesUsed + iSize + (iOffset = (iAlign - pCur->iBytesUsed % iAlign) % iAlign) > pCur->iPageSize)
		{
			if (pCur->pNext == NULL)
			{
				pNew = (SMemPage *)malloc(sizeof(SMemPage));
				pNew->pNext = NULL;
				pNew->pPrev = pCur;
				pNew->iPageSize = pHeap->iDefPageSize;
				pNew->iBytesUsed = 0;
				pNew->pMem = (void *)malloc(pNew->iPageSize);
				if (pNew->pMem == NULL)
				{
					free(pNew);
			        pNew = NULL;	
                    fprintf(stderr, "suHeapAlloc> malloc failed for new page memory(%zd)!\n", pNew->iPageSize);
					return NULL;
				}
				pCur->pNext = pNew;
				pHeap->pCur = pCur = pNew;
				pHeap->iNumOfPages++;
			}
			else	// next page is already allocated (曾经一个suResetHeap函数被调用)
				pCur = pHeap->pCur = pCur->pNext;
		}

		//page located, allocate the memory
		pCur->iBytesUsed += iOffset;
		pMem = (char *)pCur->pMem + pCur->iBytesUsed;
		pCur->iBytesUsed += iSize;

		return pMem;
	}
}
/* 为固定大小的内存需求分配内存的函数,此函数不考虑字节对齐问题,另外,分配的内存大小不能比默认值大,否则会出错 */
void *suAllocNode(SHeap *pHeap, size_t iSize)
{
	void *pMem;
	SMemPage *pCur, *pNew;

	assert(iSize <= pHeap->iDefPageSize);
	pCur = pHeap->pCur;
	//需要内存数超过剩余内存大小, 重新寻找合适页
	if (pCur->iBytesUsed + iSize > pCur->iPageSize)
	{
		if (pCur->pNext == NULL)
		{
			pNew = (SMemPage *)malloc(sizeof(SMemPage));
			pNew->pNext = NULL;
			pNew->pPrev = pCur;
			pNew->iPageSize = pHeap->iDefPageSize;
			pNew->iBytesUsed = 0;
			pNew->pMem = (void *)malloc(pNew->iPageSize);
			if (pNew->pMem == NULL)
			{
				free(pNew);
				//fprintf(stderr, "suHeapAlloc> malloc failed for new page memory(%u)!\n", pNew->iPageSize);
				return NULL;
			}
			pCur->pNext = pNew;
			pHeap->pCur = pCur = pNew;
			pHeap->iNumOfPages++;
		}
		else
			pCur = pHeap->pCur = pCur->pNext;
	}

	//page located, allocate the memory
	pMem = (char *)pCur->pMem + pCur->iBytesUsed;
	pCur->iBytesUsed += iSize;

	return pMem;
}
//////////////////////////////////////////////////////////////////////////////
//
//  void suResetHeap( SHeap *pHeap );
//  Desc:  reset all the pointers but do not free any memory
//      Params:
//         SHeap *pHeap:  pointer to the heap to be reset
//      Return:
//         none
//      Side Effect:
//
//////////////////////////////////////////////////////////////////////////////
void suResetHeap( SHeap *pHeap )
{
	SMemPage *pPage;
	SMemPage *pPrev;
	SMemPage *pNext;

	for( pPage = pHeap->pHead; pPage; pPage = pPage->pNext )
	{
		if( pPage->iPageSize <= pHeap->iDefPageSize )
		{
			pPage->iBytesUsed = 0;
		}
		else
		{
			// if it's a big page, delete it
			// the assumption is that big page is needed very rarely
			pPrev = pPage->pPrev;
			pNext = pPage->pNext;
			free( pPage->pMem );
			free( pPage );

			if( pPrev ) {
				pPrev->pNext = pNext;
				pPage = pPrev;
			}
			else {
				pHeap->pHead = pNext;
				pPage = pHeap->pHead;
			}
			if( pNext )
				pNext->pPrev = pPrev;
		}
	}

	pHeap->pCur = pHeap->pHead;

} // end of suResetHeap()


//////////////////////////////////////////////////////////////////////////////
//
//  void suFreeHeap( SHeap *pHeap );
//  Desc:  free all the heap memory
//      Params:
//         SHeap *pHeap:  pointer to the heap to be freed
//      Return:
//         none
//      Side Effect:
//	   though the pHeap value in the calling function is not NULL after
//	   calling suFreeHeap(), the memory pointed by pHeap has been freed
//	   already and pHeap must not be used again.
//
//////////////////////////////////////////////////////////////////////////////
void suFreeHeap( SHeap *pHeap )
{
	SMemPage *pPage;
	SMemPage *pTemp;
	if(pHeap == NULL)
			return;
	for( pPage = pHeap->pHead; pPage; pPage = pTemp )
	{
		if(pPage->pMem != NULL){
			free( pPage->pMem );
			pPage->pMem = NULL;
		}
		pTemp = pPage->pNext;
		if(pPage != NULL)
		{
			free( pPage );
			pPage = NULL;
		}

	}
	free( pHeap );
	pHeap = NULL;
} // end of suFreeHeap()


//////////////////////////////////////////////////////////////////////////////
//
//  int suGetHeapSize( SHeap *pHeap );
//  Desc:  total size of all the allocated pages
//      Params:
//         SHeap *pHeap:  pointer to the heap of which the size to be found
//      Return:
//         the size of all the allocated pages
//      Side Effect:
//	   none
//
//////////////////////////////////////////////////////////////////////////////
size_t suGetHeapSize( SHeap *pHeap )
{
	SMemPage *pPage;
	size_t iTotal;

	if( pHeap == NULL )
		return 0;

	iTotal = 0;
	pPage = pHeap->pHead;
	while( pPage )
	{
		iTotal += pPage->iPageSize;
		pPage = pPage->pNext;
	}

	return iTotal;

} // end of suGetHeapSize()


//////////////////////////////////////////////////////////////////////////////
//
//  int suGetHeapUsedSize( SHeap *pHeap );
//  Desc:  size of the used memory in all the pages
//      Params:
//         SHeap *pHeap:  pointer to the heap of which the size to be found
//      Return:
//	   the size of the used memory in all the pages
//      Side Effect:
//	   none
//
//////////////////////////////////////////////////////////////////////////////
size_t suGetHeapUsedSize( SHeap *pHeap )
{
	SMemPage *pPage;
	size_t iTotal;

	if( pHeap == NULL )
		return 0;

	iTotal = 0;
	pPage = pHeap->pHead;
	while( pPage ) {
		iTotal += pPage->iBytesUsed;
		pPage = pPage->pNext;
	}

	return iTotal;

} // end of suGetHeapUsedSize()

/*
char *suGetHeapDetail( char *szBuf, size_t iLen, SHeap *pHeap )
{
	char buf[256];
	SMemPage *pPage;

	if( szBuf == NULL || iLen <= 0 )
		return NULL;

	szBuf[0] = '\0';
	if( pHeap == NULL ) {
		sprintf( buf, "NULL" );
		if( strlen(szBuf) + strlen(buf) < iLen )
			strcat( szBuf, buf );
		return szBuf;
	}

	sprintf( buf, "def_sz=%lu, #_pgs=%lu:", pHeap->iDefPageSize,
					      pHeap->iNumOfPages );
	if( strlen(szBuf) + strlen(buf) < iLen )
		strcat( szBuf, buf );

	pPage = pHeap->pHead;
	while( pPage ) {
		sprintf( buf, " %lu/%lu", pPage->iBytesUsed, pPage->iPageSize );
		if( strlen(szBuf) + strlen(buf) < iLen )
			strcat( szBuf, buf );
		pPage = pPage->pNext;
	}

	return szBuf;

}
*/


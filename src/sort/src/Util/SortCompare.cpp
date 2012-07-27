#include "SortCompare.h"
#include "SDM.h"
#include <string>
#include "sys/time.h"

using namespace sort_framework;

namespace sort_util  {
	SortCompare::SortCompare(MemPool *mempool,CompareInfo* pCmpInfo, SortCompareItem* pSortCmpItem)
	{
		_mempool = mempool;
		_pCompareInfo = pCmpInfo;
		_cmpNum = _pCompareInfo->FieldNum; 
		_pSortCmpItem = pSortCmpItem;
        _curSortNo = 0;
        _curResNum = 0;
        _pResArray = NULL;
        _pKeyRow = NULL;
	}

    int SortCompare::init(SDM* pSDM, SortQuery* pQuery)
    {
        for (int i=0; i<_cmpNum; i++){
            //排序参数替换
            OrderType ord = _pCompareInfo->CmpFields[i]->ord;
            const char* pFieldName = _pCompareInfo->CmpFields[i]->Field;
            if (_pCompareInfo->CmpFields[i]->QueryParam){ //有可替换参数
                std::string szQFieName = _pCompareInfo->CmpFields[i]->QueryParam;
                const char *pTmp = pQuery->getParam(szQFieName.c_str());
                if (pTmp != NULL) {
                    char* pTmpName = NEW_VEC(_mempool, char, strlen(pTmp)+1);
                    strcpy(pTmpName, pTmp); 
                    pFieldName = pTmpName;
                    ord = ASC;
                }
                else {
                    szQFieName = "_";
                    szQFieName += _pCompareInfo->CmpFields[i]->QueryParam;
                    pTmp = pQuery->getParam(szQFieName.c_str());
                    if (pTmp != NULL) {
                        char* pTmpName = NEW_VEC(_mempool, char, strlen(pTmp)+1);
                        strcpy(pTmpName, pTmp); 
                        pFieldName = pTmpName;
                        ord = DESC;
                    }
                }
            }
            //获取alias名称
            pFieldName = pSDM->getFieldAlias(pFieldName);
            SDM_ROW* pCmpRow = NULL;
            SDM_ROW* pCmpRowSrc = pSDM->FindRow(pFieldName);
            bool isCopy = false;
            for (int j=0; j<i; j++){
                if (_nodeList[j]->szName == pFieldName){  //前面以后需要copy
                    isCopy = true;
                }
            }
            if (isCopy){
                if (pCmpRowSrc){
                    if ((pCmpRow=pSDM->FindCopyRow(pFieldName)) == NULL) {
                        pCmpRow = pSDM->AddCopyRow(pCmpRowSrc);
                    }
                }
                else{
                    return -1;
                }
            }
            else{
                if (pCmpRowSrc){
                    pCmpRow = pCmpRowSrc;
				}
				else {
					pCmpRow = pSDM->AddRow(pFieldName);
				}
			}
			if (!pCmpRow){
				return -1;
			}
			if (!pCmpRow->isAlloc()){   //开空间
				if ((pCmpRow->Alloc(pSDM->getDocNum()))<0){
					//return -1;
				}
			}
			Cmp_Node* pCmpNode = NEW (_mempool, Cmp_Node);
			pCmpNode->szName = pFieldName;
			pCmpNode->pRow = pCmpRow;
			pCmpNode->ord = ord;
			switch (pCmpRow->dataType) {
				case DT_INT8:{
						     pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<int8_t>)((int8_t*)pCmpRow->getBuff(), pCmpNode->ord);
						     break;
					     }
				case DT_UINT8: {
						       pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<uint8_t>)((uint8_t*)pCmpRow->getBuff(), pCmpNode->ord);
						       break;
					       }
				case DT_INT16: {
						       pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<int16_t>)((int16_t*)pCmpRow->getBuff(), pCmpNode->ord);
						       break;
					       }
				case DT_UINT16: {
							pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<uint16_t>)((uint16_t*)pCmpRow->getBuff(), pCmpNode->ord);
							break;
						}
				case DT_INT32: {
						       pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<int32_t>)((int32_t*)pCmpRow->getBuff(), pCmpNode->ord);
						       break;
					       }
				case DT_UINT32: {
							pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<uint32_t>)((uint32_t*)pCmpRow->getBuff(), pCmpNode->ord);
							break;
						}
				case DT_INT64: {
						       pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<int64_t>)((int64_t*)pCmpRow->getBuff(), pCmpNode->ord);
						       break;
					       }
				case DT_UINT64: {
							pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<uint64_t>)((uint64_t*)pCmpRow->getBuff(), pCmpNode->ord);
							break;
						}
				case DT_FLOAT: {
						       pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<float>)((float*)pCmpRow->getBuff(), pCmpNode->ord);
						       break;
					       }
				case DT_DOUBLE: {
							pCmpNode->pCompare = (RowCompare*)NEW (_mempool, RowCompareBase<double>)((double*)pCmpRow->getBuff(), pCmpNode->ord);
							break;
						}
				default :{
                         TLOG("unknow data type in add field row");
						 return -1;
					 }
			}
			_nodeList.push_back(pCmpNode);
		}
		//设置排序队列
		_pKeyRow = pSDM->getKeyRow();
		if (!_pKeyRow) {
			return -1;
		}
		//保证第一排序数据加载
		if (!_nodeList[0]->pRow->isLoad()){
			_nodeList[0]->pRow->setDocNum(_pKeyRow->getDocNum());
			if(!_nodeList[0]->pRow->Load()) {
                return -1;
            }
		}
        //如果第一排序为RL，需要加载第二排序
        if (_nodeList[0]->szName == MNAME_RELESCORE ||
                _nodeList[0]->szName == "RL" ||
                _nodeList[0]->szName == "rl") 
        {
            if (!_nodeList[1]->pRow->isLoad()) {
                _nodeList[1]->pRow->setDocNum(_pKeyRow->getDocNum());
                _nodeList[1]->pRow->Load();
            }
        }
        _curSortNo = 0;
        return 0;
    }

	int SortCompare::sort()
	{
		int i;
		int32_t* heap = &_pSortCmpItem->pSortArray[_pSortCmpItem->sortBegNo];
		int allNum = _pSortCmpItem->curDocNum;
		void * pBuff;
		_curSortNo = _pSortCmpItem->sortBegNo;
		for (i=0; i<_cmpNum; i++){
			pBuff = _nodeList[i]->pRow->getBuff();
			if (!pBuff){    //确保row分配内存
				return -1;
			}
			_nodeList[i]->pCompare->setData(pBuff); //设置数据空间
		}
		//建立最小堆
		for (i=1; i<allNum; i++){
			Max_upHeap(heap, i, allNum, true);
		}
		return 0;
	}

	void SortCompare::push_front(Cmp_Node *cmpNode)
	{
		if (cmpNode == NULL) {
			return;
		}
		std::vector<Cmp_Node*> nodeList(_nodeList);
		_nodeList.clear();
		_nodeList.push_back(cmpNode);
		for (uint32_t i=0; i<nodeList.size(); i++) {
			_nodeList.push_back(nodeList[i]);
		}
		return;
	}

	int32_t SortCompare::getSortNext()
	{
		int32_t* heap = &_pSortCmpItem->pSortArray[0];
		//int32_t* heap = &_pSortCmpItem->pSortArray[_pSortCmpItem->sortBegNo];
		int tail = _pSortCmpItem->sortBegNo + _pSortCmpItem->curDocNum;
		if (_curSortNo >= tail)
			return -1;
		int32_t iTmp; //替换临时存储
		iTmp = heap[_curSortNo];
		heap[_curSortNo] = heap[tail-1];
		heap[tail-1] = iTmp;
		_curSortNo++;
		Max_downHeap(heap, _curSortNo, tail, true);
		return (int)_curSortNo-1;
	}

	int SortCompare::top()
	{
		return _pSortCmpItem->pSortArray[_pSortCmpItem->sortBegNo];
	}

	int SortCompare::top(int n, bool iSort)
	{
		if (n > _pSortCmpItem->curDocNum)
			n = _pSortCmpItem->curDocNum;
		if (n > (_pSortCmpItem->curDocNum *3/100)){
			topByMax(n,iSort);
		}
		else{
			topByMin(n,iSort);
		}
		return n;
	}

	inline int SortCompare::topByMax(int n, bool iSort)
	{
		int i;
		int32_t* heap = &_pSortCmpItem->pSortArray[_pSortCmpItem->sortBegNo];
		int allNum = _pSortCmpItem->curDocNum;

		void * pBuff;
		for (i=0; i<_cmpNum; i++){
			pBuff = _nodeList[i]->pRow->getBuff();
			if (!pBuff){    //确保row分配内存
				return -1;
			}
			_nodeList[i]->pCompare->setData(pBuff); //设置数据空间
		}
		//建立最小堆
		for (i=1; i<allNum; i++){
			Max_upHeap(heap, i, allNum, true);
		}
		int32_t iTmp; //替换临时存储
		for (i=0; i<n;){
			iTmp = heap[i];
			heap[i] = heap[allNum-1];
			heap[allNum-1] = iTmp;
			i++;
			Max_downHeap(heap, i, allNum, true);
		}
		return n;
	}

	int SortCompare::topByMin(int n, bool iSort)
	{
		int i;
		int32_t* heap = &_pSortCmpItem->pSortArray[_pSortCmpItem->sortBegNo];

		void * pBuff;
		for (i=0; i<_cmpNum; i++){
			pBuff = _nodeList[i]->pRow->getBuff();
			if (!pBuff){    //确保row分配内存
				return -1;
			}
			_nodeList[i]->pCompare->setData(pBuff); //设置数据空间
		}
		int lastnum = 0;    //有多少和最大值相等，这部分值需要二次排序
		//建立最小堆
		for (i=1; i<n;i++){
			upHeap(heap, i, false);
		}
		//比对最小堆是否需要调整
		int iflag;
		int32_t iTmp; //替换临时存储
		for (i=n;  i<_pSortCmpItem->curDocNum; i++){
			iflag = _nodeList[0]->pCompare->compare(heap[i], heap[0]); //比对是否需要调整
			if(iflag < 0) {
				iTmp = heap[0];
				heap[0] = heap[i];
				heap[i] = iTmp;    //对换
				downHeap(heap, n-1, iSort);  //调整
				//downHeap(heap, n-1, false);  //调整
				//
				if (!_nodeList[0]->pCompare->compare(heap[0], heap[i])){    //是否最大值等于弹出的值
					iTmp = heap[n+lastnum]; //保持连续
					heap[n+lastnum] = heap[i];
					heap[i] = iTmp;
					lastnum++;
				}
				else{
					lastnum=0;
				}
			}
			else if (iflag == 0){   //等于最大值
				iTmp = heap[n+lastnum]; //保持连续
				heap[n+lastnum] = heap[i];
				heap[i] = iTmp;
				lastnum++;
			}
		}

		int crtnum = 0;
		bool isCrt = (n==_pSortCmpItem->curDocNum)? false :true;
		for (i=n-1; i>=0;){
			if (isCrt){
				if(_nodeList[0]->pCompare->compare(heap[0], heap[n])){    //临界值区间结束
					isCrt = false;
					if (!iSort){
						break;  //不需要精确就退出
					}
				}
				else {
					crtnum++;
				}
			}
			if (i==0){
				break;
			}
			iTmp = heap[i];
			heap[i] = heap[0];
			heap[0] = iTmp;
			i--;
			downHeap(heap, i, iSort);
		}
		//对临界数据进行精确排序
		if (crtnum > 0){
			heap = &heap[n - crtnum];
			int allcnum = crtnum+lastnum;
			for (int i=1; i<crtnum; i++){
				upHeap(heap, i, true);
			}
			int cmpNo;
			for (int i=crtnum; i<allcnum;i++){
				cmpNo = 1;
				while (cmpNo<_cmpNum){
					if (!_nodeList[cmpNo]->pRow->isLoad()) {//除了第一和最后一个排序肯定load外，其它需要判断
						_nodeList[cmpNo]->pRow->Read(heap[i]);
                        _nodeList[cmpNo]->pRow->Read(heap[0]);
					}
					iflag = _nodeList[cmpNo]->pCompare->compare(heap[i], heap[0]); //比对是否需要调整
					if(iflag < 0) {
						iTmp = heap[0];
						heap[0] = heap[i];
						heap[i] = iTmp;    //对换
						downHeap(heap, crtnum-1, true);  //调整
						break;
					}
					if(iflag > 0) {
						break;
					}
					cmpNo++;
				}
			}
			for (i=crtnum-1; i>0;){
				iTmp = heap[i];
				heap[i] = heap[0];
				heap[0] = iTmp;
				i--;
				downHeap(heap, i, true);
			}
		}
		return n;
	}

	/* 向下调整优先队列 */
	inline void SortCompare::Max_downHeap(int32_t* heap, int nSize, int allSize, bool iSort)
	{
		int i = 0;
		int tail = allSize-1;
		int node = heap[tail-i];           // 保存最顶的元素
		int j = i << 1;             // 发现较小的孩子节点
		int k = j + 1;
		if (k <= (tail-nSize) && less(heap[tail-k], heap[tail-j], iSort)) {
			j = k;
		}
		while (j <= (tail-nSize) && less(heap[tail-j], node, iSort))
		{
			heap[tail-i] = heap[tail-j];            // 向下移至孩子节点
			i = j;
			j = i << 1;
			k = j + 1;
			if( k <= (tail-nSize) && less(heap[tail-k], heap[tail-j], iSort) ) {
				j = k;
			}
			if (j == 0)
				break;
		}
		heap[tail-i] = node;               // 放置节点到正确的位置
	}

	/* 向下调整优先队列 */
	inline void SortCompare::downHeap(int32_t* heap, int nSize, bool iSort)
	{
		int i = 0;

		int node = heap[i];           // 保存最顶的元素
		int j = i << 1;             // 发现较小的孩子节点
		int k = j + 1;
		if (k <= nSize && !less(heap[k], heap[j], iSort)) {
			j = k;
		}
		while (j <= nSize && !less(heap[j], node, iSort))
		{
			heap[i] = heap[j];            // 向下移至孩子节点
			i = j;
			j = i << 1;
			k = j + 1;
			if( k <= nSize && !less(heap[k], heap[j], iSort) ) {
				j = k;
			}
		}
		heap[i] = node;               // 放置节点到正确的位置
	}

	inline void SortCompare::Max_upHeap(int32_t* heap, int nSize, int allSize, bool iSort)
	{
		int i = nSize;
		int tail = allSize-1;
		int node = heap[tail-i];             // 保存最底的元素
		int j = i >> 1;
		while( j > 0 && less(node, heap[tail-j], true))
		{
			heap[tail-i] = heap[tail-j];            // 向上移至父节点
			i = j;
			j = j >> 1;
		}
		if ((j == 0) && less(node, heap[tail], true)){
			heap[tail-i] = heap[tail];
			i = 0;
		}

		heap[tail-i] = node;               // 放置节点到正确的位置
	}

	inline void SortCompare::upHeap(int32_t* heap, int nSize, bool iSort)
	{
		int i = nSize;
		int node = heap[i];             // 保存最底的元素
		int j = i >> 1;
		while( j > 0 && !less(node, heap[j], iSort))
		{
			heap[i] = heap[j];            // 向上移至父节点
			i = j;
			j = j >> 1;
		}
		if ((j == 0) && !less(node, heap[0], iSort)){
			heap[i] = heap[0];
			i = 0;
		}

		heap[i] = node;               // 放置节点到正确的位置
	}

	//比较函数
	//未来拆分两个 区别isort
	//inline __attribute__((always_inline)) bool SortCompare::less(int32_t no1, int32_t no2, bool iSort)
    inline bool SortCompare::less(int32_t no1, int32_t no2, bool iSort)
	{
		int iflag;
		if (iSort){ //精确
			iflag = _nodeList[0]->pCompare->compare(no1, no2); 
			if (iflag < 0)
				return true; 
			else if (iflag > 0)
				return false; 
			if (iflag == 0) {
				for (int i = 1; i< _cmpNum-1; i++){
					if (!_nodeList[i]->pRow->isLoad()) {//除了第一和最后一个排序肯定load外，其它需要判断
						_nodeList[i]->pRow->Read(no1);
						_nodeList[i]->pRow->Read(no2);
					}
					iflag = _nodeList[i]->pCompare->compare(no1, no2);
					if (iflag < 0)
						return true; 
					else if (iflag > 0)
						return false; 
				}
			}
			iflag = _nodeList[_cmpNum-1]->pCompare->compare(no1, no2);
			if (iflag < 0)
				return true; 
			else if (iflag > 0)
				return false; 
		}
		else {
			iflag = _nodeList[0]->pCompare->compare(no1, no2);
			if (iflag <= 0)
				return true;
			else
				return false; 
		}
		return true;
	}
	inline bool SortCompare::less_max(int32_t no1, int32_t no2, bool iSort)
	{
		int iflag;
		if (iSort){ //精确
			iflag = _nodeList[0]->pCompare->compare(no1, no2); 
			if (iflag > 0)
				return true; 
			else if (iflag < 0)
				return false; 
			if (iflag == 0) {
				for (int i = 1; i< _cmpNum-1; i++){
					if (!_nodeList[i]->pRow->isLoad()) {//除了第一和最后一个排序肯定load外，其它需要判断
						_nodeList[i]->pRow->Read(no1);
						_nodeList[i]->pRow->Read(no2);
					}
					iflag = _nodeList[i]->pCompare->compare(no1, no2);
					if (iflag > 0)
						return true; 
					else if (iflag < 0)
						return false; 
				}
			}
			iflag = _nodeList[_cmpNum-1]->pCompare->compare(no1, no2);
			if (iflag > 0)
				return true; 
			else if (iflag < 0)
				return false; 
		}
		return true; 
	}
}




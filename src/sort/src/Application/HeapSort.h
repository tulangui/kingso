#ifndef _HEAPSORT_H
#define _HEAPSORT_H
#include "SortModeBase.h"
namespace sort_application{

class HeapSort : public SortModeBase
{
public:
    HeapSort(){
    }

    virtual ~HeapSort(){}
    virtual int init(mxml_node_t* xNode, SortOperate* pOperate){
        return 0;
    }
    virtual int prepare(SDM* pSDM){
        //check compare
        if (!_pCompareInfo)
            return -1;
        return 0;
    }
    virtual int process(SDM* pSDM, sort_framework::SortQuery* pQuery,
            SearchResult *pResult, MemPool *mempool){
        SortCompare* pCompare;
        if (!(pCompare = pSDM->GetCmp(_pCompareInfo))){
            return -1;
        }
        int sortNum = pSDM->getAllRetNum();
        if (sortNum <= 0){
            return 0;
        }
        int ret = _pOperate->Top(pCompare, sortNum);
        if (ret <= 0) {
            return ret;
        }
        pSDM->Pop(ret);
        return 0;
    }
private:
};

}
#endif

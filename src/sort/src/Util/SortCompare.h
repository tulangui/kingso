#ifndef __SORT_COMPARE_H_
#define __SORT_COMPARE_H_
#include "SortCfgInfo.h"
#include "SortQuery.h"
#include "Util/SDMDEF.h"
#include "Util/SDMROW.h"
//#include "Util/SDM.h"

# if __WORDSIZE == 64
#  define __INT64_C(c)  c ## L
#  define __UINT64_C(c) c ## UL
# else
#  define __INT64_C(c)  c ## LL
#  define __UINT64_C(c) c ## ULL
# endif

/* Maximum of signed integral types.  */
#ifndef INT8_MAX
# define INT8_MAX       (127)
#endif 
#ifndef INT16_MAX
# define INT16_MAX      (32767)
#endif 
#ifndef INT32_MAX
# define INT32_MAX      (2147483647)
#endif 
#ifndef INT64_MAX
# define INT64_MAX      (__INT64_C(9223372036854775807))
#endif 

/* Maximum of unsigned integral types.  */
#ifndef UINT8_MAX
# define UINT8_MAX      (255)
#endif 
#ifndef UINT16_MAX
# define UINT16_MAX     (65535)
#endif 
#ifndef UINT32_MAX
# define UINT32_MAX     (4294967295U)
#endif 
#ifndef UINT64_MAX
# define UINT64_MAX     (__UINT64_C(18446744073709551615))
#endif 
#ifndef DBL_MAX 
#define DBL_MAX     1.7976931348623157e+308
#endif 
#ifndef FLT_MAX 
#define FLT_MAX     3.40282347e+38F
#endif 

namespace sort_util {
class SDM;

#define EPSINON 0.000001

inline bool Max_Eval(int8_t p1) {
    return p1 == INT8_MAX;
}
inline bool Max_Eval(int16_t p1) {
    return p1 == INT16_MAX;
}
inline bool Max_Eval(int32_t p1) {
    return p1 == INT32_MAX;
}
inline bool Max_Eval(int64_t p1) {
    return p1 == INT64_MAX;
}
inline bool Max_Eval(uint8_t p1) {
    return p1 == UINT8_MAX;
}
inline bool Max_Eval(uint16_t p1) {
    return p1 == UINT16_MAX;
}
inline bool Max_Eval(uint32_t p1) {
    return p1 == UINT32_MAX;
}
inline bool Max_Eval(uint64_t p1) {
    return p1 == UINT64_MAX;
}
inline bool Max_Eval(float p1) {
    return p1 == FLT_MAX;
}
inline bool Max_Eval(double p1) {
    return p1 == DBL_MAX;
}

inline bool EQ_Eval(int8_t p1, int8_t p2) {
            return p1 == p2;
}
inline bool EQ_Eval(int16_t p1, int16_t p2) {
            return p1 == p2;
}
inline bool EQ_Eval(int32_t p1, int32_t p2) {
            return p1 == p2;
}
inline bool EQ_Eval(int64_t p1, int64_t p2) {
            return p1 == p2;
}
inline bool EQ_Eval(uint8_t p1, uint8_t p2) {
            return p1 == p2;
}
inline bool EQ_Eval(uint16_t p1, uint16_t p2) {
            return p1 == p2;
}
inline bool EQ_Eval(uint32_t p1, uint32_t p2) {
            return p1 == p2;
}
inline bool EQ_Eval(uint64_t p1, uint64_t p2) {
            return p1 == p2;
}
    inline bool EQ_Eval(float p1, float p2) {
        if ((p1-p2)<EPSINON && (p1-p2)>-EPSINON)
            return true;
        return false;
    }
    inline bool EQ_Eval(double p1, double p2) {
        if ((p1-p2)<EPSINON && (p1-p2)>-EPSINON)
            return true;
        return false;
    }

class RowCompare{
public:
    RowCompare(){}
    virtual ~RowCompare(){}
    virtual void setData(void* pData)  = 0;
    virtual void setOrd(sort_framework::OrderType ord) = 0;
    virtual void setMaxOrder(int maxorder) = 0;
    virtual int compare(int32_t no1, int32_t no2) = 0;
};

template <typename T>
class RowCompareBase : public RowCompare{
public:
    RowCompareBase(T* pData, sort_framework::OrderType ord)
    : _pData(pData), _order(ord == sort_framework::ASC? -1 : 1), _maxorder(1) { }
    ~RowCompareBase(){}
    virtual void setOrd(sort_framework::OrderType ord){
        _order = (ord == sort_framework::ASC)? -1 : 1;
    }
    virtual void setMaxOrder(int maxorder) { _maxorder = maxorder;}
    int compare(int32_t no1, int32_t no2){
        if (EQ_Eval(_pData[no1], _pData[no2])) 
            return 0;
        else if(Max_Eval(_pData[no1]))
            return _maxorder;
        else if(Max_Eval(_pData[no2]))
            return -_maxorder;
        else if(_pData[no1] < _pData[no2]) 
            return _order;
        else
            return -_order;
    }

    void setData(void* pData){
        _pData = (T*)pData;
    }
private:
    T* _pData;
    int _order;
    int _maxorder;
};

typedef struct _Cmp_Node{
    std::string szName;
    SDM_ROW* pRow;
    sort_framework::OrderType ord;
    RowCompare* pCompare;
	_Cmp_Node()
	{
		pRow = NULL;
		ord = sort_framework::ASC;
		pCompare = NULL;
	}
}Cmp_Node;

class SortCompare
{
public:
    SortCompare(MemPool *mempool, sort_framework::CompareInfo* pCmpInfo, SortCompareItem* pSortCmpItem);
    int init(SDM* pSDM, sort_framework::SortQuery* pQuery);
    //int getDocNum(){ return _docNum; }
    int sort(); //全部队列进行排序
    int32_t getSortNext(); //全部队列进行排序
    //int sort(int start, int num, bool iFirst=false); //全部队列指定区间内进行排序
    //选取前N数据 , iSort  为true,topN数据将进行排序，否则只要保证为top数据即可。
    int top(int n, bool iSort=true); 
    int top();
    SDM_ROW* getRow(int rowno){
        if(_cmpNum <= rowno)
            return NULL;
        return _nodeList[rowno]->pRow;
    }
    void setRow(int rowno, SDM_ROW *pRow) {
        if ((pRow == NULL) || (_cmpNum <= rowno)) {
            return;
        }
        _nodeList[rowno]->pRow = pRow;
        return;
    }
    int getOrd(int node){
        if (_cmpNum <= node)
            return 0;
        return _nodeList[node]->ord;
    }
    int getCmpNum(){ return _cmpNum; }
    void* getVal(int rowno, int no) {
        if ((_cmpNum<= rowno) || (no<0))
            return NULL;
        SDM_ROW* pRow = _nodeList[rowno]->pRow;
        return pRow->getValAddr(no);
    }
    uint32_t getSortCur(){ return _curSortNo; }
	bool cmp(int32_t no1, int32_t no2) {return less(no1, no2, true);}
	SDM_ROW* getKeyRow() {return _pKeyRow;}
	void push_front(Cmp_Node *cmpNode);
    ~SortCompare(){}

    //暂时
    std::vector<Cmp_Node*> _nodeList;
    SortCompareItem* _pSortCmpItem;
private:
    bool less(int32_t no1, int32_t no2, bool iSort);
    bool less_max(int32_t no1, int32_t no2, bool iSort);
    void upHeap(int32_t* heap, int nSize, bool iSort);
    void downHeap(int32_t* heap, int nSize, bool iSort);
    void Max_upHeap(int32_t* heap, int nSize, int allSize, bool iSort);
    void Max_downHeap(int32_t* heap, int nSize, int allSize, bool iSort);

    int topByMin(int n, bool iSort=true); 
    int topByMax(int n, bool iSort=true); 

    int _cmpNum; //比较队列个数
    sort_framework::CompareInfo* _pCompareInfo; //比较类型
    MemPool* _mempool;
    SDM_ROW* _pKeyRow;  //SDM主键
    uint32_t*  _pResArray;  //返回给结果集合
    uint32_t _curResNum;   //当前返回结果个数
    uint32_t _curSortNo;   //当前Sort的游标
};

}
#endif

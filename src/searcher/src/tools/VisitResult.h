#ifndef TOOLS_VISIT_RESULT_H_
#define TOOLS_VISIT_RESULT_H_

#include "VisitStatArgs.h"

#pragma pack (1)
struct QueryInfo {
    char* query;
    int32_t freq;
};

struct VisitInfo {
    int64_t nid;
    int64_t st;
};
#pragma pack ()

class VisitResult {
public:
    VisitResult(int colNum);
    ~VisitResult();

    void setSingleResult(VisitInfo* visit, int num);
    void setMulResult(VisitInfo* visit, int num);
    
    int getColResult(int no, VisitInfo*& info, int& num);
    int output(char* filename, int per, int mod = 720);

    bool isOk() {return _colNum == _maxColNum; }
private:
    int        _colNum;
    int        _maxColNum;

    int32_t    _visitInfoNum[MAX_STAT_COL_NUM];
    VisitInfo* _ppVisitInfo[MAX_STAT_COL_NUM];

public:
    static int cmpNid(const void * p1, const void * p2);
    static int cmpSt(const void * p1, const void * p2);
};


#endif // TOOLS_VISIT_RESULT_H_

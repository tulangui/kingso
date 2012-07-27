#include <zlib.h>
#include <dirent.h>
#include <ifaddrs.h>
#include <sys/types.h>

#include "VisitStat.h" 
#include "util/MD5.h"
#include "util/iniparser.h"
#include "index_lib/DocIdManager.h"


VisitResult::VisitResult(int colNum)
{
    _colNum = 0;
    _maxColNum = colNum;
    memset(_visitInfoNum, 0, sizeof(int32_t) * MAX_STAT_COL_NUM);
    memset(_ppVisitInfo, 0, sizeof(VisitInfo*) * MAX_STAT_COL_NUM); 
}

VisitResult::~VisitResult()
{
    for(int i = 0; i < _colNum; i++) {
        if(_ppVisitInfo[i]) delete[] _ppVisitInfo[i];
    }
    _colNum = 0;
    memset(_ppVisitInfo, 0, sizeof(VisitInfo*) * MAX_STAT_COL_NUM); 
}

void VisitResult::setSingleResult(VisitInfo* visit, int num)
{
    qsort(visit, num, sizeof(VisitInfo), cmpNid);

    for(int i = 0; i < num; i++) {
        int no = visit[i].nid % _maxColNum;
        _visitInfoNum[no]++;

        if(_ppVisitInfo[no]) continue;
        _ppVisitInfo[no] = visit + i;
    }

    for(int i = 0; i < _maxColNum; i++) {
        if(_ppVisitInfo[i] == NULL) continue;
        VisitInfo* tmp = new VisitInfo[_visitInfoNum[i]];
        memcpy(tmp, _ppVisitInfo[i], sizeof(VisitInfo) * _visitInfoNum[i]);
        _ppVisitInfo[i] = tmp;
    }
    _colNum = _maxColNum;
}

void VisitResult::setMulResult(VisitInfo* visit, int num)
{
    if(num <= 0) {
        _visitInfoNum[_colNum] = 0;
        _ppVisitInfo[_colNum] = NULL;
    } else {
        _visitInfoNum[_colNum] = num;
        _ppVisitInfo[_colNum] = new VisitInfo[num];
        memcpy(_ppVisitInfo[_colNum], visit, sizeof(VisitInfo) * num);
    }
    _colNum++;
}

int VisitResult::getColResult(int no, VisitInfo*& info, int& num)
{
    if(no >= _colNum) {
        return -1;
    }
    info = _ppVisitInfo[no];
    num = _visitInfoNum[no];

    return 0;
}

int VisitResult::output(char* filename, int per, int mod)
{
    int num = 0;
    for(int i = 0; i < _colNum; i++) {
        num += _visitInfoNum[i];
    }

    VisitInfo* info = new VisitInfo[num];
    VisitInfo* p = info;

    for(int i = 0; i < _colNum; i++) {
        if(_visitInfoNum[i] <= 0) continue;
        memcpy(p, _ppVisitInfo[i], _visitInfoNum[i] * sizeof(VisitInfo));
        p += _visitInfoNum[i];
    }

    qsort(info, num, sizeof(VisitInfo), cmpSt);
    int top = (int)(num * ((float)per / 100.0));
   
    int64_t topNum = 0;
    int64_t allNum = 0;

    FILE* fpArray[mod];
    memset(fpArray, 0, sizeof(FILE*) * mod);

    for(int i = 0; i < top; i++) {
        int no = info[i].nid % mod;
        if(fpArray[no] == NULL) {
            char name[PATH_MAX];
            sprintf(name, "%s-%.5d", filename, no);
            fpArray[no] = fopen(name, "wb");
            if(NULL == fpArray[no]) {
                return -1;
            }
        }

        topNum += info[i].st;
        fprintf(fpArray[no], "%lu\n", info[i].nid);
    }

    for(int i = 0; i < mod; i++) {
        if(fpArray[i]) {
            fclose(fpArray[i]);
        }
    }

    allNum = topNum;
    for(int i = top; i < num; i++) {
        allNum += info[i].st;
    }
    TLOG("all visit=%lu, top visit=%lu, per=%.3f", allNum, topNum, topNum * 1.0 / allNum * 100.0);

    return 0; 
}

int VisitResult::cmpNid(const void * p1, const void * p2)
{
    if(((VisitInfo*)p1)->nid < ((VisitInfo*)p2)->nid)
        return -1;
    if(((VisitInfo*)p1)->nid > ((VisitInfo*)p2)->nid)
        return 1;
    return 0;
}

int VisitResult::cmpSt(const void * p1, const void * p2)
{
    if(((VisitInfo*)p1)->st > ((VisitInfo*)p2)->st)
        return -1;
    if(((VisitInfo*)p1)->st < ((VisitInfo*)p2)->st)
        return 1;
    return 0;
}


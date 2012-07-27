#ifndef __SORT_OPRATE_H_
#define __SORT_OPRATE_H_
#include "Util/SDM.h"
#include "Util/SortCompare.h"

namespace sort_util {

typedef std::map<uint64_t,  char > hashType;
typedef hashType::iterator hashIter;

class SortOperate
{
public:
    SortOperate(){}
    ~SortOperate(){}
    /*
     * filename
     * val
     * pRow 写入地址
     * weight加权
     */ 
    int Match(const char * pfieldName, const char* val, SDM_ROW* pRow, float weight){return 0;} 
    int Pop(){return 0;}
    int PopSel(){return 0;}

    int GroupBySel(MemPool* mempool, SDM* pSDM, SortCompare *pSortCmp, SDM_ROW** pDistRow, 
            int * pDistCnt, int distNum, int num, int same_level_num = 0){
        int32_t* pSortArray = pSDM->getSortArray();
        if ((!pDistCnt) || (!pDistRow) || (!pSortArray)
                || (distNum<1) || (num<1)){
            return -1;
        }

        int renum = 0;
        hashType *distMap = NEW_ARRAY(mempool, hashType, distNum);
        hashIter *distIter = NEW_ARRAY(mempool, hashIter, distNum);
        
        uint64_t nKeyHash;
        bool iflag;
        int32_t *reArray= NEW_VEC(mempool, int32_t, num);
        uint32_t *rePost= NEW_VEC(mempool, uint32_t, num);

        int32_t curNo = 0;
        int32_t sx = pSortCmp->getSortCur()-pSDM->getBegNo();
        int pop_num = 0;
        while( true ){  //循环排序队列
            if(same_level_num != 0 && curNo >= same_level_num ) {
                break;
            }
            iflag = true;
            if (sx <= 0){
                if (pSortCmp->getSortNext() < 0){
                    break;
                }
            }
            else{
                sx--;
            }
            uint32_t docId = pSortArray[curNo];
            for (int j=0; j<distNum; j++){   //循环
                pDistRow[j]->Read(docId);
                pDistRow[j]->getHash( docId, nKeyHash);
                if (nKeyHash != INT64_MAX){     //空值不做dist判断
                    if ((distIter[j] = distMap[j].find(nKeyHash)) == distMap[j].end()){//没有
                        distMap[j].insert(std::make_pair(nKeyHash, 0));
                        distIter[j] = distMap[j].find(nKeyHash);
                    }
                    else if (distIter[j]->second >= pDistCnt[j]){
                        iflag = false;
                        break;
                    }
                }
                else{
                    distIter[j] = distMap[j].end();
                }
            }
            if (iflag){
                for (int j=0; j<distNum; j++){   //循环
                    if (distIter[j] != distMap[j].end()){
                        distIter[j]->second++;
                    }
                }
                rePost[renum] = curNo;
                reArray[renum++] = pSortArray[curNo];
            }
            if (renum == num)
                break;
            curNo++;
        }
        if (renum>0){
            int32_t * pNew = &pSortArray[rePost[renum-1]];
            int32_t * pOld;
            int iMovNum;
            for (int i=renum-1; i>0; i--){  //重新整理排序队列
                iMovNum = rePost[i]-rePost[i-1]-1;
                pOld = &pSortArray[rePost[i-1]+1]+iMovNum-1;
                for (int j=0; j<iMovNum;j++){
                    *pNew-- = *pOld--;
                }
            }
            pOld =  &pSortArray[0];
            iMovNum = rePost[0];
            for (int j=0; j<iMovNum;j++){
                *pNew-- = *pOld--;
            }
            memcpy(pSortArray, reArray, renum*sizeof(int32_t));
        }
        return renum;
    }


    int Top(SortCompare *pSortCmp, int num, bool isSort=true){
        return pSortCmp->top(num, isSort);
    }

    int Del(){return 0;}

private:
};

}
#endif

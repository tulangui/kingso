#ifndef _SORT_DATA_MANAGE_H
#define _SORT_DATA_MANAGE_H
#include <string>
#include <map>
#include "commdef/SearchResult.h"
#include "Application/string_split.h"
#include "SortCfgInfo.h"
#include "SortQuery.h"
#include "Util/SDMDEF.h"
#include "Util/SDMROW.h"
#include "Util/SortCompare.h"
#include "Util/SDMMemCopy.h"
#include "sort/OPDefine.h"

namespace sort_util
{

    //class SortCompare;

    class SDM
    {
        public:
            const SearchResult *_pSearchRes;//search result info
            std::string _szUrlLoc;
            std::string _sDebugInfo;
            sort_framework::SortQuery* _pQuery;  //sort query
            uint32_t _docNum;
            uint32_t _docsFound;
            uint32_t _startNo;        //query参数s
            uint32_t _retNum;        //query参数n
            uint32_t _allRetNum;        //query参数n
            uint32_t *_pResArray;  //返回给结果集合
            uint32_t _curResNum;   //当前返回结果个数
            uint32_t _estDocsFound;
            uint32_t _now;
            double _estFactor;
            SortCompareItem *_pSortCmpItem;
            uint32_t _uniqNullCount;

        public:
            SDM(MemPool *mempool, sort_framework::SortQuery* pQuery, const SearchResult *searchRes);
            SDM(MemPool *mempool, sort_framework::SortQuery* pQuery);
            SDM(MemPool *mempool);
            ~SDM(){}
            int32_t init(sort_framework::SortQuery* pQuery);
            SDM_ROW *getFirstRankRow();
            SDM_ROW *getSecondRankRow();
			void Alloc();
            void Load();
            void Dump(FILE *pFile, bool bPrint=false);
			int32_t Append(SDM **ppSDM, uint32_t size);
            int32_t FilterInvalidID(uint32_t nNum=0);
            SDM_ROW *AddTmpRow(SDM_ROW* pSrcSdm);
            SDM_ROW *AddCopyRow(SDM_ROW* pSrcSdm);
            SDM_ROW *AddVirtRow(const char* name,PF_DATA_TYPE type, int size);
            SDM_ROW *AddOtherRow(const char* name,PF_DATA_TYPE type, int size);
            SDM_ROW *AddProfRow(const char* name);
            SDM_ROW *FindRow(const char* rowname);
            SDM_ROW *FindCopyRow(const char* rowname);
            SDM_ROW *AddRow(const char* name, bool isCopy = false);
            int setKeyRow(SDM_ROW* pKeyRow);
            int AddCmp(sort_framework::CompareInfo* pCmpInfo, sort_framework::SortQuery* pQuery);
            SortCompare *GetCmp(sort_framework::CompareInfo* pCmpInfo);
            SDM_ROW *getKeyRow(){ return _keyRow;}
            uint32_t *getResArray(){return _pResArray;}
            int32_t *getSortArray(){
                if (!_pSortCmpItem)
                    return NULL;
                return &_pSortCmpItem->pSortArray[_pSortCmpItem->sortBegNo];
            }
            uint32_t getBegNo(){
                if (!_pSortCmpItem)
                    return -1;
                return _pSortCmpItem->sortBegNo;
            }
            uint32_t getSortNum(){
                if (!_pSortCmpItem)
                    return -1;
                return _pSortCmpItem->curDocNum;
            }
            const char *getFieldAlias(const char * key) const
            {
                if(key && (strcmp(key, "rl") == 0 || strcmp(key, "RL") == 0)) {
                    return MNAME_RELESCORE;
                }
                if(key && (strcmp(key, "coefp") == 0)) {
                    return "static_trans_score";
                }
                return key;
            }

            int Pop(uint32_t num);
            MemPool *_mempool;
            uint32_t _curNo;
            std::map<std::string, SDM_ROW*> _rowMap;
            std::vector<SDM_ROW*> _rows;
            SDMReader* _pSDMReader;
            SDM_ROW* _keyRow;

            std::map<sort_framework::CompareInfo*, SortCompare*> _cmpMap;
            std::vector<SortCompare*> _cmpList;

            uint32_t getStartNo(){return _startNo;}
            uint32_t getretNum(){return _retNum;}
            uint32_t getAllRetNum(){return _allRetNum-_curResNum;}
            //提供给heapsort2d等需要修改返回结果数量的sort
            void resetAllRetNum(uint32_t ret)
            {
                _allRetNum=ret;
                //需要重新分配结果空间
                _pResArray = NEW_VEC(_mempool, uint32_t, _allRetNum);
            }
            uint32_t getCurResNum() {return _curResNum;}
            void setCurResNum(uint32_t curResNum){_curResNum = curResNum;}
            uint32_t getDocNum(){return _docNum;}
            void setDocNum(uint32_t docNum){_docNum = docNum;}
            void addDocNum(uint32_t docNum){_docNum += docNum;}
            uint32_t getDocsFound() {return _docsFound;}
            uint32_t getEstimateDocsFound() {return _estDocsFound;}
            double getEstimateFactor() {return _estFactor;}
            void setDocsFound(uint32_t docsFound) {_docsFound=docsFound;}
            void setEstimateDocsFound(uint32_t estDocsFound) {_estDocsFound=estDocsFound;}
            void setUniqNullCount(uint32_t nullCount) {_uniqNullCount=nullCount;}
            uint32_t getUniqNullCount() {return _uniqNullCount;}
            int32_t FullRes();
            //序列化函数
            int32_t serialize(char *str, uint32_t size);
            int32_t getSerialSize();
            //反序列化
            int32_t deserialize(char *str, uint32_t size);
    };

}

#endif

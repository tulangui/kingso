#ifndef _SORT_DEFINE_H
#define _SORT_DEFINE_H

//#define MNAME_DOCID "docId"
//#define MNAME_RELESCORE "relevance"
namespace sort_util
{
typedef struct _SortCompareItem{
    int32_t* pSortArray;   //待排序的队列
    uint32_t docNum;   //search的结果个数
    uint32_t curDocNum;    //当前待排序个数
    uint32_t sortBegNo;    //起始no号
    uint32_t sortEndNo;    //终止no号
}SortCompareItem;

}
#endif

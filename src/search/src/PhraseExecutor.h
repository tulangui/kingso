/** \file 
 ********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 11424 $
 *
 * $LastChangedDate: 2012-04-20 13:52:13 +0800 (Fri, 20 Apr 2012) $
 *
 * $Id: PhraseExecutor.h 11424 2012-04-20 05:52:13Z pujian $
 *
 * $Brief: phrase查询器 $
 ********************************************************************
 */

#ifndef _PHRASE_EXECUTOR_H_
#define _PHRASE_EXECUTOR_H_

#include "AndExecutor.h"
#include "PostExecutor.h"
#include "commdef/commdef.h"
#include "util/MemPool.h"

using namespace util;

namespace search {

class PhraseExecutor : public AndExecutor
{
public:
    /**
     * 构造函数
     */
    PhraseExecutor();
 
    /**
     * 析构函数
     */
    virtual ~PhraseExecutor();

    /**
     * 归并seek接口，输入参数docid，输出当前第一个等于或大于输入参数的docid值
     * @param nDocId docid值
     * @return 当前第一个等于或大于输入参数的docid值
     */
    inline virtual uint32_t seek(uint32_t nDocId);

    /**
     * 初始化内部变量空间，addExecutor之后调用
     * @param pHeap MemPool对象指针
     * @return 0 成功 非0 错误码
     */
    int init(MemPool *pHeap); 
 
    /**
     * 批量归并，直接返回结果集
     * @param SearchResult 输出search结果集结构
     * @param nFlag 降低归并层次优化条件
     * @return 0 成功 非0 错误码
     */
    inline virtual int32_t doExecute(SearchResult *&pSearchResult, uint32_t nCutLen, 
            int32_t nFlag = 0, uint32_t nInitDocId = 0); 
 
    /**
     * 调整语法归并节点，索引底层seek可以优化为next
     * @param pContext 框架context
     * @return 优化后的Executor，NULL无须优化
     */
    virtual Executor* getOptExecutor(Executor* pParent, framework::Context* pContext, ExecutorType exType = ET_NONE);
 
    /**
     * 索引截断性能优化
     * @param szSortField 排序字段名
     * @param nSortType 排序类型 (0:正排 1:倒排)
     * @param p_doc_accessor    正排索引访问接口
     * @return 阶段后长度
     */
    virtual int32_t truncate(const char* szSortField, int32_t nSortType, index_lib::IndexReader *pIndexReader, index_lib::ProfileDocAccessor *pAccessor, MemPool *pHeap);

protected:
    /**
     * 判断occ是否相邻
     * @return true 相邻 flase 不相邻
     */ 
    inline bool isPhrase();

protected:
    uint8_t **_ppOcc;          // 记录当前所有doc的occ
    int32_t *_pOccNum;         // 记录当前所有doc的occ个数
    int32_t *_pStarts;         // 记录occ数组当前下标
    MemPool *_pHeap;           // mempool

};

uint32_t PhraseExecutor::seek(uint32_t nDocId)
{
    uint32_t nCurDocId = AndExecutor::seek(nDocId);
    int32_t nCurNum = 0;
    bool bNoOcc = false;
    while(nCurDocId < INVALID_DOCID) 
    {  
        for(int32_t i = 0; i < _executor_size; i++) 
        {
            _ppOcc[i] = ((PostExecutor*)_executors[i])->getOcc(nCurNum);
            if(!_ppOcc[i])
            {
                bNoOcc = true;
                break;
            }
            _pOccNum[i] = nCurNum;
        }
        if(bNoOcc || isPhrase()) {
            break;
        }
        nCurDocId = AndExecutor::seek(++nCurDocId); 
    } 
    return nCurDocId;
}

bool PhraseExecutor::isPhrase()
{
    int32_t i = 0, j = 0, k = 0;
    uint8_t usFirst = 0;
    memset(_pStarts, 0x0, _executor_size*sizeof(int32_t));
    do
    {
        usFirst = _ppOcc[0][k];
        i = 1;
        do
        {
            usFirst += 1;
            j = _pStarts[i];
            do
            {
                if (_ppOcc[i][j] >= usFirst)
                    break;
                _pStarts[i] ++;
                j++;
            }while(j < _pOccNum[i]);
            if (_pStarts[i] == _pOccNum[i]) {//查找结束
                return false;
            }
            if (_ppOcc[i][j] > usFirst) { //从第一个字开始验证下一个可能相邻的位置
                break;
            }
            i++;
        }while(i < _executor_size);
        if (i == _executor_size) {    //找到匹配的结果
            return true;
        }
        k++;
    }while(k < _pOccNum[0]); 

    return false;
}

int32_t PhraseExecutor::doExecute(SearchResult *&pSearchResult, uint32_t nCutLen, 
        int32_t nFlag, uint32_t nInitDocId)
{
    int32_t k = 0;
    uint32_t nCurDocId = nInitDocId;
    int32_t nCurNum = 0;
    bool bNoOcc = false;
    while((nCurDocId = AndExecutor::seek(nCurDocId)) < INVALID_DOCID) 
    {  
        for(int32_t i = 0; i < _executor_size; i++) 
        {
            _ppOcc[i] = ((PostExecutor*)_executors[i])->getOcc(nCurNum);
            if(!_ppOcc[i])
            {
                bNoOcc = true;
                break;
            }
            _pOccNum[i] = nCurNum;
        }
        if(bNoOcc || isPhrase()) 
        { 
            for(k = 0; k < _filter_size; k++)
            {
                if(!(_filters[k]->process(nCurDocId))) {
                    break;
                }
            }
            if(k == _filter_size)
            {
                pSearchResult->nDocIds[pSearchResult->nDocSize] = nCurDocId;
                pSearchResult->nRank[pSearchResult->nDocSize] = score();
                pSearchResult->nDocSize++;
            }
        } 
        nCurDocId++;
    }


    return KS_SUCCESS;
}

}

#endif


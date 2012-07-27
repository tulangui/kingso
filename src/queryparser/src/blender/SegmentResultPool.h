/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: SegmentResultPool.h 2577 2011-03-09 01:50:55Z yanhui.tk $
 *
 * $Brief: 分词结果池 $
 ********************************************************************/

#ifndef QUERYPARSER_SEGRESULTPOOL_H
#define QUERYPARSER_SEGRESULTPOOL_H

#include "AliWS/ali_tokenizer.h"
#include <pthread.h>

#define MAX_SEGMENT_REUSLT_NUM 40 // 结果池中保存的SegResult实体的最大个数(理论上不超过blender线程数)

namespace queryparser{

    class SegmentResultPool
    { 
        public:
            SegmentResultPool();
            
            ~SegmentResultPool();
            
            /** 
             * @brief     初始化分词结果池和锁
             *
             * @param[in] p_tokenizer 分词器
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t init(ws::AliTokenizer* p_tokenizer);

            /** 
             * @brief     从分词结果池中获取一个分词结果项
             *
             * @return    分词结果
             */
            ws::SegResult *pop();

            /** 
             * @brief     向分词结果池中归还一个分词结果项
             *
             * @param[in] p_seg_reuslt 分词结果
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t push(ws::SegResult *p_seg_reuslt);

        private:
            ws::AliTokenizer   *_p_tokenizer;                           // 分词器
            ws::SegResult      *_seg_results[MAX_SEGMENT_REUSLT_NUM];   // 分词结果
            int32_t             _count;                                 // 已分配分词结果个数
            pthread_spinlock_t  _lock;                                  // 自旋锁
    };
}

#endif

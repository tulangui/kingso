/*********************************************************************
 * $Author: yangfan.yf $
 *
 * $LastChangedBy: yangfan.yf $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2012-06-1809:50:55 +0800 (Mon, 09 June 2012) $
 *
 * $Id: TokenizerPool.h 2577 2012-06-18 01:50:55Z yangfan.yf $
 *
 * $Brief: 分词器管理器 $
 ********************************************************************/

#ifndef LIB_TOKENIZERPOOL_H
#define LIB_TOKENIZERPOOL_H

#include <pthread.h>
#include "commdef/TokenizerFactory.h"

#define MAX_TOKENIZER_NUM 80 // 结果池中保存的SegResult实体的最大个数(理论上不超过blender线程数)
#define MAX_WORDSPLIT_LEN 64  // 分词器类型名最大长度

namespace queryparser{

class TokenizerPool
{ 
    public:
        TokenizerPool();
        
        ~TokenizerPool();
        
        /** 
         * @brief     初始化分词器管理器和锁
         *
         * @param[in] filename 分词器配置文件
    		 * @param[in] ws_type  分词器类型
         * 
         * @return    0 成功; 非0 错误码 
         */
        int32_t init(const char *filename, const char* ws_type);

        /** 
         * @brief     从分词器管理器中获取一个分词器
         *
         * @return    分词器
         */
        ITokenizer* pop();

        /** 
         * @brief     向分词器管理器中归还一个分词器
         *
         * @param[in] p_tokenizer 分词器
         * 
         * @return    0 成功; 非0 错误码 
         */
        int32_t push(ITokenizer* p_tokenizer);

    private:
        CTokenizerFactory _factory;                                 // 分词器工厂
        char _ws_type[MAX_WORDSPLIT_LEN];							// 分词器类型
        ITokenizer   *_tokenizer_stack[MAX_TOKENIZER_NUM];          // 分词器存储器
        int32_t             _count;                                 // 已分配分词器个数
        pthread_spinlock_t  _lock;                                  // 自旋锁
};

}

#endif

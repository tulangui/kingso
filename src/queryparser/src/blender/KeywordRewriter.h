/*********************************************************************
 * $Author: yanhui.tk $
 *
 * $LastChangedBy: yanhui.tk $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: KeywordRewriter.h 2577 2011-03-09 01:50:55Z yanhui.tk $
 *
 * $Brief: query中的关键词(q或rewq)的改写器，包含中文分词和简单的语法检查 $
 ********************************************************************/

#ifndef QUERYPARSER_KEYWORDREWRITER_H
#define QUERYPARSER_KEYWORDREWRITER_H

#include "mxml.h"
#include "util/MemPool.h"
#include "StopwordDict.h"

#include "queryparser/QueryRewriter.h"
#include "QueryRewriterDef.h"
#include "commdef/TokenizerFactory.h"



namespace queryparser{
    class TokenizerPool;

    class KeywordRewriter
    { 
        public:
            /** 
             * @brief     构造函数
             *
             * @param[in] p_mem_pool        内存管理器
             * @param[in] p_tokenizer       分词器
             * @param[in] p_seg_reuslt_pool 分词结果管理器
             * @param[in] p_stopword_dict   停用词表
             * @param[in] package_conf_map  package配置信息
             *
             */
            KeywordRewriter(MemPool *p_mem_pool,
                            TokenizerPool *p_seg_reuslt_pool,
                            StopwordDict *p_stopword_dict,
                            const PackageConfigMap &package_conf_map);

            ~KeywordRewriter();
 
            /** 
             * @brief     keyword改写器的对外调用接口，主控流程
             *
             * @param[in] keyword   原始keyword
             * @param[in] len       原始keyword长度
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doRewrite(const char *keyword, int32_t len);
    
            /** 
             * @brief     获取改写后的keyword
             *
             * @return    改写后的keyword，没有返回NULL
             */
            char *getRewriteKeyword();

            /** 
             * @brief     获取改写后的keyword的长度
             *
             * @return    改写后keyword的长度，没有返回0
             */
            int32_t getRewriteKeywordLen();

        private:
            /** 
             * @brief     对keyword中的括号进行合法性检查，并进行整理
             *
             * @param[in|out] keyword   原始keyword副本
             * @param[in]     len       原始keyword长度
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doBracketCheck(SectionInfo *section_infos, int32_t section_num);
            int32_t doBracketErase(SectionInfo *section_infos, int32_t section_num);
            int32_t doBracketRemove(SectionInfo *section_infos, int32_t section_num);
 
            /** 
             * @brief     将keyword切分成段落，要求是原始keyword副本
             *
             * @param[in]   keyword   原始keyword副本
             * @param[in]   len       原始keyword长度
             * @param[out]  section_infos keyword切分后的段落信息
             * @param[out]  section_num   keyword切分后的段落个数
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doSectionSplit(char *keyword, int32_t len,
                                   SectionInfo *section_infos, int32_t &section_num);

            /** 
             * @brief     对keyword进行分词
             *
             * @param[in|out] section_infos keyword切分后的段落信息
             * @param[in]     section_num   keyword切分后的段落个数
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doKeywordTokenize(SectionInfo *section_infos, int32_t section_num);
            
            /** 
             * @brief     去掉无效操作符
             *
             * @param[in|out] section_infos keyword切分后的段落信息
             * @param[in]     section_num   keyword切分后的段落个数
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doOperatorCheck(SectionInfo *section_infos, int32_t section_num);
            int32_t doOperatorRemove(SectionInfo *section_infos, int32_t section_num);
            int32_t doOperatorRange(SectionInfo *section_infos, int32_t section_num);
            
            /** 
             * @brief     对keyword进行分词决策: 判断那些段落需要分词以及需要分词段落的起始位置
             *
             * @param[in|out] section_infos keyword切分后的段落信息
             * @param[in]     section_num   keyword切分后的段落个数
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doTokenizeDecision(SectionInfo *section_infos, int32_t section_num);
            
            /** 
             * @brief     对keyword切分并已经分词后的段落进行合并，
             *
             * @param[in|out] section_infos keyword切分后的段落信息
             * @param[in]     section_num   keyword切分后的段落个数
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doSectionCombine(SectionInfo *section_infos, int32_t section_num);
            
            /** 
             * @brief     对keyword切分后的段落信息进行语法检查（暂未实现）
             *
             * @param[in|out] section_infos keyword切分后的段落信息
             * @param[in]     section_num   keyword切分后的段落个数
             * 
             * @return    0 成功; 非0 错误码 
             */
            int32_t doSectionCheck(SectionInfo *section_infos, int32_t section_num);
            int32_t doSectionRemove(SectionInfo *section_infos, int32_t section_num);
            
            
            int32_t doPackageNameCheck(SectionInfo *section_infos, int32_t section_num);
            int32_t doPackageNameRemove(SectionInfo *section_infos, int32_t section_num);

        private:
            MemPool                 *_p_mem_pool;           // 内存池
            TokenizerPool 			*_p_tokenizer_pool;     // 分词器管理器
            StopwordDict            *_p_stopword_dict;      // 停用词表 
            const PackageConfigMap  &_package_conf_map;     // package配置信息映射表

            char                    *_rew_keyword;          // 改写后的关键词
            int32_t                 _rew_keyword_len;       // 改写后的关键词长度
    };

}

#endif

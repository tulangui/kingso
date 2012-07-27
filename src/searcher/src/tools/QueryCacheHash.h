/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: QueryCacheHash.h 2577 2011-03-09 01:50:55Z pujian $
 *
 * $Brief: query hash for cache $
 ********************************************************************/

#ifndef KINGSO_QUERYCACHEHASH_H_
#define KINGSO_QUERYCACHEHASH_H_

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

#include "commdef/commdef.h"
#include "util/common.h"
#include "util/Log.h"
#include "util/HashMap.hpp"

#define MAX_PARAM_PAIR_NUM  256

namespace cache_hash
{

typedef struct _param_key_value
{
    const char * key;
    const char * value;
}ParamPair;

class QueryCacheHash
{ 
public:
    QueryCacheHash();
    ~QueryCacheHash();

    /**
     * 添加规则字段
     * @param key               规则目标字段名
     * @param 字段规则          黑名单(false) / 白名单(true)
     */
    void addRule(const char* key, bool rule = false);

    /**
     * 解析query，计算cache相关的hash值
     * @param query             目标query
     * @return                  cache哈希值
     */
    uint32_t getHash(const char* query);

    /**
     * 根据一个字符串,计算hash code
     * @param szString          源字符串
     * @param strLen            用来及算hash的前几个字符, 为0表示用整个串
     * @param dwHashType        hash方法
     * @return                  哈希值
     */
    static inline uint64_t hash(const char *szString, int strLen, uint64_t dwHashType = 0)
    {
        unsigned char*  key   = (unsigned char *)szString;
        uint64_t   seed1 = 0x7FED7FED;
        uint64_t   seed2 = 0xEEEEEEEE;

        int ch  = 0;
        int pos = 0;

        if (strLen > 0) {
            while ((*key != 0) && (pos < strLen))
            {
                ch = toupper(*key++);
                pos ++;
                seed1 = CRYPT_TABLE[(dwHashType << 8) + ch] ^ (seed1 + seed2);
                seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
            }
        }
        else {
            while (*key != 0)
            {
                ch = toupper(*key++);
                seed1 = CRYPT_TABLE[(dwHashType << 8) + ch] ^ (seed1 + seed2);
                seed2 = ch + seed1 + seed2 + (seed2 << 5) + 3;
            }
        }
        return seed1;
    }

    /**
     * 初始化对象，添加需要过滤的字段名称
     */
    void init();

private:
    /**
     * 解析query为key/value对儿
     * @param query         原始query字符串
     * @param queryBuf      中间字符串存储buffer
     * @param paramArray    key/value存储buffer
     * @param paramNum      key/value个数
     * @return              0, OK; -1, ERROR
     */
    int  parseQuery(const char *query, char *queryBuf, ParamPair *paramArray, int &paramNum);

    /**
     * 排序并组装key/value对儿
     * @param paramArray    key/value对儿存储buffer
     * @param paramNum      key/value对儿个数
     * @param outputArray   输出数组
     * @param size          输出数组的长度
     * @return              -1,ERROR; other,组装后的长度
     */
    int  assembleParams(ParamPair *paramArray, int paramNum, char * outputArray, int size);

private:
    static const uint64_t CRYPT_TABLE[1280];                //Hash算法对应的内码表
    util::HashMap<const char *, bool>         _ruleList;    //黑白名单列表，暂时默认全为黑名单
};

}

#endif //KINGSO_QUERYCACHEHASH_H_

/*********************************************************************
 * $Author: pujian $
 *
 * $LastChangedBy: pujian $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-22 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ProfileHashtable.h 2577 2011-03-22 01:50:55Z pujian $
 *
 * $Brief: 哈希表功能实现，改自pdict $
 ********************************************************************/
#ifndef KINGSO_INDEXLIB_PROFILE_HASHTABLE_H_
#define KINGSO_INDEXLIB_PROFILE_HASHTABLE_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <ctype.h>
#include <limits.h>

#include "IndexLib.h"

// 计算64位签名对应的哈希桶的下标位置
#define PROFILE_DICT_GET_HASH(sign,hashsize) \
    (((sign)&0x00000000FFFFFFFF + ((sign)>>32)) % (hashsize))


// 表示节点无效的游标值
static const uint32_t INVALID_NODE_POS = 0x7FFFFFFF;
// block不够，重新分配内存时的步长
static const uint32_t INCREMENT_BLOCK_NUM = 50000;

namespace index_lib
{

// data structures defined here
#pragma pack(1)
/** 哈希表内部结点结构体定义 */
typedef struct _idx_hash_node
{
	uint64_t sign;           //64位数字签名
	uint32_t flag : 1,       //当前node是否为可用空间（桶内节点的可用标识，0标识未占用，1标识已占用）
             next : 31;      //下一个node的偏移量
	char     payload[0];     //payload指针
}ProfileHashNode;
#pragma pack()

/** ProfileHashTable类的定义 */
class ProfileHashTable
{
public:
    /**
     * 构造函数
     * @param  incr_step  哈希表结点数组空间扩展的步长大小
     */
    ProfileHashTable(uint32_t incr_step = INCREMENT_BLOCK_NUM);

    /**
     * 析构函数
     */
    ~ProfileHashTable();

    /**
     * 创建一个哈希表
     * @param hashsize     哈希桶的大小
     * @param payload_size 哈希表结点中存储的有效数据的空间大小
     * @return             true,OK; false,ERROR
     */
    bool initHashTable(const uint32_t hashsize, const uint32_t payload_size);

    /**
     * 从硬盘加载一个哈希表
     * @param path  文件所在路径
     * @param file  文件名
     * @return      true,OK; false,ERROR
     */
    bool loadHashTable(const char* path, const char* file);

    /**
     * dump哈希表到硬盘
     * @param  path    文件路径
     * @param  file    文件名
     * @param  resize  dump过程中是否进行bucket重整(true进行重整)
     * @return         true,OK; false,ERROR
     */
    bool dumpHashTable(const char* path, const char* file, bool resize = false);

    /**
     * 释放哈希表资源
     */
    void destroyHashTable();

    /**
     * reset哈希表
     */
    void resetHashTable();

    /**
     * 向哈希表中添加一个节点
     * @param  node  输入节点的指针
     * @return       1, 找到key，修改value; 0, 添加节点成功; -1, ERROR;
     */
    inline int addNode(ProfileHashNode *node)
    {
        if (unlikely(node == NULL)) {
            return -1;
        }

        ProfileHashNode *pCurrent = NULL;
        ProfileHashNode *ptarget  = NULL;

        uint32_t pos = PROFILE_DICT_GET_HASH(node->sign, _hashSize);
        pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
        if (pCurrent->flag != 0) {
            // 桶空间对应位置已占用
            if (pCurrent->sign == node->sign) {
                ptarget = pCurrent;
            }
            else {
                while(pCurrent->next != INVALID_NODE_POS) {
                    pos = pCurrent->next;
                    pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
                    if (pCurrent->sign == node->sign) {
                        ptarget = pCurrent;
                        break;
                    }
                }
            }
        }

        if (ptarget != NULL) {
            // find hashnode, modify data
            memcpy(ptarget->payload, node->payload, _payloadSize);
            return 1;
        }
        else {
            // not find hash node, add node
            if (pCurrent->flag == 0) {
                // 在hash桶对应位置原地修改
                node->next = INVALID_NODE_POS;
                node->flag = 1;
                memcpy(pCurrent, node, _blockSize);
                _nodeNum++;
            }
            else {
                // 在hash桶空间外部修改
                if (_blockPos == _blockNum) {
                    // block array is full, realloc
                    char *buffer = _pBlockAddr;
                    buffer       = (char *)realloc(buffer, (_blockNum + _incrNum) * _blockSize);
                    if (buffer == NULL) {
                        return -1;
                    }
                    // reset next value
                    ProfileHashNode *pnode = NULL;
                    for(uint32_t ind = 0; ind < _incrNum; ind++) {
                        pnode = (ProfileHashNode *)(buffer + (_blockNum + ind) * _blockSize);
                        pnode->next = INVALID_NODE_POS;
                        pnode->flag = 0;
                    }
                    _pBlockAddr = buffer;
                    _blockNum  += _incrNum;
                    pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
                }

                node->next = INVALID_NODE_POS;
                node->flag = 1;
                memcpy(_pBlockAddr + (_blockPos * _blockSize), node, _blockSize);
                pCurrent->next = _blockPos++;
                _nodeNum++;
            }
            return 0;
        }
        return -1;
    }

    /**
     * 向哈希表中添加payload value
     * @param  key     关键词
     * @param  keylen  关键词长度
     * @param  payload value值的指针
     * @return         1, 找到相同的key,修改值; 0, 没有找到key，添加值成功; -1, error;
     */
    inline int addValuePtr(const char* key, int keylen, void* payload)
    {
        if (unlikely(key == NULL || keylen < 0)) {
            return -1;
        }
        uint64_t sign = 0;
        int      ret  = 0;

        sign = hash(key, keylen);
        ret  = addValuePtr(sign, payload);
        return ret;
    }


    /**
     * 向哈希表中添加payload value
     * @param  sign    关键词签名值
     * @param  payload value值的指针
     * @return         1, 找到相同的key,修改值; 0, 没有找到key，添加值成功; -1, error;
     */
    inline int addValuePtr(uint64_t sign, void* payload)
    {
        if (unlikely(payload == NULL)) {
            return -1;
        }

        ProfileHashNode *pCurrent = NULL;
        ProfileHashNode *ptarget  = NULL;

        uint32_t pos = PROFILE_DICT_GET_HASH(sign, _hashSize);
        pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
        if (pCurrent->flag != 0) {
            // 桶空间对应位置已占用
            if (pCurrent->sign == sign) {
                ptarget = pCurrent;
            }
            else {
                while(pCurrent->next != INVALID_NODE_POS) {
                    pos = pCurrent->next;
                    pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
                    if (pCurrent->sign == sign) {
                        ptarget = pCurrent;
                        break;
                    }
                }
            }
        }

        if (ptarget != NULL) {
            // find hashnode, modify data
            memcpy(ptarget->payload, payload, _payloadSize);
            return 1;
        }
        else {
            // not find hash node, add node
            if (pCurrent->flag == 0) {
                // 在hash桶对应位置原地修改
                memcpy(pCurrent->payload, payload, _payloadSize);
                pCurrent->next = INVALID_NODE_POS;
                pCurrent->sign = sign;
                pCurrent->flag = 1;
                _nodeNum++;
            }
            else {
                // 在hash桶空间外部修改
                if (_blockPos == _blockNum) {
                    // block array is full, realloc
                    char *buffer = _pBlockAddr;
                    buffer       = (char *)realloc(buffer, (_blockNum + _incrNum) * _blockSize);
                    if (buffer == NULL) {
                        return -1;
                    }
                    // reset next value
                    ProfileHashNode *pnode = NULL;
                    for(uint32_t ind = 0; ind < _incrNum; ind++) {
                        pnode = (ProfileHashNode *)(buffer + (_blockNum + ind) * _blockSize);
                        pnode->next = INVALID_NODE_POS;
                        pnode->flag = 0;
                    }
                    _pBlockAddr = buffer;
                    _blockNum  += _incrNum;
                    pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
                }

                ptarget = (ProfileHashNode *)(_pBlockAddr + (_blockPos * _blockSize));
                memcpy(ptarget->payload, payload, _payloadSize);
                ptarget->next = INVALID_NODE_POS;
                ptarget->flag = 1;
                ptarget->sign = sign;
                pCurrent->next = _blockPos++;
                _nodeNum++;
            }
            return 0;
        }
        return -1;
    }


    /**
     * 按照关键词查找对应的哈希表节点
     * @param  key     关键词
     * @param  keylen  关键词长度
     * @return         NULL,ERROR; 哈希表节点的指针
     */
    inline ProfileHashNode* findNode(const char* key, const int keylen)
    {
        if (unlikely(key == NULL || keylen < 0)) {
            return NULL;
        }

        ProfileHashNode *pnode = NULL;
        pnode = findNode(hash(key, keylen));
        return pnode;
    }

    /**
     * 按照关键词签名查找对应的哈希表节点
     * @param  sign  关键词前面值
     * @return       NULL,ERROR; 哈希表节点的指针
     */
    inline ProfileHashNode* findNode(uint64_t sign)
    {
        ProfileHashNode *pCurrent = NULL;
        ProfileHashNode *ptarget  = NULL;

        uint32_t pos = PROFILE_DICT_GET_HASH(sign, _hashSize);
        pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
        if (pCurrent->flag != 0) {
            // 桶空间对应位置已占用
            if (pCurrent->sign == sign) {
                ptarget = pCurrent;
            }
            else {
                while((pos = pCurrent->next) != INVALID_NODE_POS) {
                    pCurrent = (ProfileHashNode *)(_pBlockAddr + (pos * _blockSize));
                    if (pCurrent->sign == sign) {
                        ptarget = pCurrent;
                        break;
                    }
                }
            }
        }
        return ptarget;
    }

    /**
     * 按照关键词查找哈希表节点中的value值地址
     * @param  key       关键词
     * @param  keylen    关键词长度
     * @return           NULL,ERROR; value值的地址
     */
    inline char* findValuePtr(const char* key, const int keylen)
    {
        if (unlikely(key == NULL || keylen < 0)) {
            return NULL;
        }

        char *payload = NULL;
        uint64_t sign = 0;
        sign          = hash(key, keylen);
        payload       = findValuePtr(sign);

        return payload;
    }

    /**
     * 按照关键词签名查找哈希表节点中的value值地址
     * @param  sign  关键词签名
     * @return NULL,ERROR; value值的地址
     */
    inline char* findValuePtr(uint64_t sign)
    {
        ProfileHashNode * pnode = findNode( sign );

        if (unlikely( pnode == NULL )) return NULL;

        return pnode->payload;
    }

    /**
     * 获取哈希表中的首个哈希表节点
     * @param  pos  首个节点的位置下标
     * @return      NULL, ERROR; else, 首个哈希表节点的指针
     */
    inline ProfileHashNode* firstNode(uint32_t &pos)
    {
        ProfileHashNode *ptarget = NULL;
        ProfileHashNode *pblock  = NULL;
        for (uint32_t bpos = 0; bpos < _blockPos; bpos++) {
            pblock  = (ProfileHashNode *)(_pBlockAddr + (bpos * _blockSize));
            if (pblock->flag != 0) {
                pos = bpos;
                ptarget = pblock;
                break;
            }
        }
        return ptarget;
    }

    /**
     * 获取目标位置哈希表节点的下一个节点
     * @param  pos   目标节点位置
     * @return       NULL, 无下一个节点; else, 下一个节点的指针
     */
    inline ProfileHashNode* nextNode(uint32_t &pos)
    {
        ProfileHashNode *ptarget = NULL;
        ProfileHashNode *pblock  = NULL;
        for (uint32_t bpos = (pos + 1); bpos < _blockPos; bpos++) {
            pblock  = (ProfileHashNode *)(_pBlockAddr + (bpos * _blockSize));
            if (pblock->flag != 0) {
                pos     = bpos;
                ptarget = pblock;
                break;
            }
        }
        return ptarget;
    }

    /**
     * 获取哈希表内部block块的当前位置
     * @return      从0开始的内部哈希表节点block块的位置
     */
    inline uint32_t getBlockPos() { return _blockPos;}

    /**
     * 获取哈希表内部block块的总数
     * @return      哈希表节点数组中block块的总数量
     */
    inline uint32_t getBlockNum() { return _blockNum;}

    /**
     * 获取哈希表内部的有效哈希节点数量
     * @return      有效哈希节点（一个key/value对）的个数
     */
    inline uint32_t getKeyValueNum() { return _nodeNum;}

    /**
     * 获取单个block块的空间大小
     * @return      block块的空间大小
     */
    inline uint32_t getBlockSize() { return _blockSize;}

    /**
     * 获取hashtable内部的payload value size
     * @return      payload value的空间大小
     */
    inline uint32_t getPayloadSize() { return _payloadSize; }

private:
    char*     _pBlockAddr;     //哈希表内部block空间初始地址
    uint32_t  _nodeNum;        //哈希表中的key/value结点数量
    uint32_t  _blockNum;       //哈希表内部block块的总大小
    uint32_t  _blockPos;       //哈希表内部block块的当前位置(从预留桶大小开始)
    uint32_t  _blockSize;      //哈希表内部block块的空间大小
    uint32_t  _payloadSize;    //哈希表节点内部payload对应的大小（单位byte）
    uint32_t  _hashSize;       //内部哈希表数组大小
    uint32_t  _incrNum;        //空间不足时Block每次增长的数量

public:
    /**
     * 根据实际哈希node个数，计算推荐的新bucket大小
     * @param  num  哈希表中记录的keyvalue结点数量
     * @return      推荐的bucket大小（素数)
     */
    static uint32_t getNewBucketNum(uint32_t num);

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
            //while ((*key != 0) && (pos < strLen))
            while ( pos < strLen )
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

private:
    static const uint64_t CRYPT_TABLE[1280];    //Hash算法对应的内码表

    /**
     * 内部dump数据逻辑
     */
    bool normalDump(const char *path, const char *file);
    bool resizeDump(const char *path, const char *file);

    // test only
    float calculateEmptyHoleRate();
};

}

#endif //KINGSO_INDEXLIB_PROFILE_HASHTABLE_H_

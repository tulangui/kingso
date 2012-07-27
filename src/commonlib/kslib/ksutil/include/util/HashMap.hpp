
/** \file
 *********************************************************************
 * $Author: gongyi.cl $
 *
 * $LastChangedBy: gongyi.cl $
 *
 * $Revision: 895 $
 *
 * $LastChangedDate: 2011-04-18 16:38:19 +0800 (Mon, 18 Apr 2011) $
 *
 * $Id: HashMap.hpp 895 2011-04-18 08:38:19Z gongyi.cl $
 *
 * $Brief: c++动态数组模板类 $
 ********************************************************************/
#ifndef _CPP_HASH_MAP_H
#define _CPP_HASH_MAP_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "namespace.h"
#include "crc.h"

UTIL_BEGIN;

/**
 * @brief 哈希表节点模板
 */
template < typename Key, typename Value > struct _HashNode {
    struct _HashNode <Key, Value > *next;
    struct _HashNode <Key, Value > *seq_next;
//    struct HashNode* seq_prev;
    Key key;
    Value value;
};

/**
 * @brief 哈希算法模板
 */
template < typename Key > struct KeyHash {
};

/**
 * @brief 哈希比较函数模板
 */
template < typename Key > struct KeyEqual {
};

/**
 * @brief 字符串哈希算法——crc64
 */
template <> struct KeyHash <const char *> {
    inline uint64_t operator() (const char *key) const {
        return get_crc64(key, strlen(key));
    }
};

/**
 * @brief 字符串哈希算法——crc64
 */
template <> struct KeyHash <char *> {
    inline uint64_t operator() (char *key) const {
        return get_crc64(key, strlen(key));
    }
};

/**
 * @brief 整型hash算法
 */
template <> struct KeyHash <const int32_t> {
    inline uint64_t operator() (const int32_t key) const {
        return (uint64_t)key;
    }
};

/**
 * @brief 整型hash算法
 */
template <> struct KeyHash <int32_t> {
    inline uint64_t operator() (int32_t key) const {
        return (uint64_t)key;
    }
};

/**
 * @brief 无符号整型hash算法
 */
template <> struct KeyHash <const uint32_t> {
    inline uint64_t operator() (const uint32_t key) const {
        return (uint64_t)key;
    }
};

/**
 * @brief 无符号整型hash算法
 */
template <> struct KeyHash <uint32_t> {
    inline uint64_t operator() (uint32_t key) const {
        return (uint64_t)key;
    }
};

/**
 * @brief 整型hash算法
 */
template <> struct KeyHash <const int64_t> {
    inline uint64_t operator() (const int64_t key) const {
        return (uint64_t)key;
    }
};

/**
 * @brief 整型hash算法
 */
template <> struct KeyHash <int64_t> {
    inline uint64_t operator() (int64_t key) const {
        return (uint64_t)key;
    }
};

/**
 * @brief 无符号整型hash算法
 */
template <> struct KeyHash <const uint64_t> {
    inline uint64_t operator() (const uint64_t key) const {
        return (uint64_t)key;
    }
};

/**
 * @brief 无符号整型hash算法
 */
template <> struct KeyHash <uint64_t> {
    inline uint64_t operator() (uint64_t key) const {
        return (uint64_t)key;
    }
};



/**
 * @brief 字符串比较
 */
template <> struct KeyEqual <char *> {
    inline bool operator() (char *k1, char *k2) const {
        if (strcmp(k1, k2) == 0) {
            return true;
        }
        else {
            return false;
        }
    }
};

/**
 * @brief 字符串比较
 */
template <> struct KeyEqual <const char *> {
    inline bool operator() (const char *k1, const char *k2) const {
        if (strcmp(k1, k2) == 0) {
            return true;
        }
        else {
            return false;
        }
    }
};

/**
 * @brief 整型比较
 */
template <> struct KeyEqual <const int32_t> {
    inline bool operator() (const int32_t k1, const int32_t k2) const {
        return (k1 == k2); 
    }
};

/**
 * @brief 整型比较
 */
template <> struct KeyEqual <int32_t> {
    inline bool operator() (int32_t k1, int32_t k2) const {
        return (k1 == k2); 
    }
};

/**
 * @brief 整型比较
 */
template <> struct KeyEqual <const uint32_t> {
    inline bool operator() (const uint32_t k1, const uint32_t k2) const {
        return (k1 == k2); 
    }
};

/**
 * @brief 整型比较
 */
template <> struct KeyEqual <uint32_t> {
    inline bool operator() (uint32_t k1, uint32_t k2) const {
        return (k1 == k2); 
    }
};


/**
 * @brief 整型比较
 */
template <> struct KeyEqual <const int64_t> {
    inline bool operator() (const int64_t k1, const int64_t k2) const {
        return (k1 == k2); 
    }
};

/**
 * @brief 整型比较
 */
template <> struct KeyEqual <int64_t> {
    inline bool operator() (int64_t k1, int64_t k2) const {
        return (k1 == k2); 
    }
};

/**
 * @brief 整型比较
 */
template <> struct KeyEqual <const uint64_t> {
    inline bool operator() (const uint64_t k1, const uint64_t k2) const {
        return (k1 == k2); 
    }
};

/**
 * @brief 整型比较
 */
template <> struct KeyEqual <uint64_t> {
    inline bool operator() (uint64_t k1, uint64_t k2) const {
        return (k1 == k2); 
    }
};

/**
 * @brief HashMap迭代器
 */
template < typename Key, typename Value > 
struct _HashMapIterator{
    typedef _HashNode < Key, Value > _Node;
    typedef _HashMapIterator < Key, Value > _Self;
    _Node *_curNode;

    _HashMapIterator():_curNode(){}
    
    explicit
    _HashMapIterator(_Node *curNode):_curNode(curNode){}



    _Node& operator* () const {
        return *_curNode;
    }

    _Node* operator->() const {
        return _curNode;
    }
    
    _Self& operator++(){
        assert(_curNode);
        _curNode = _curNode->seq_next;
        return *this;
    }

    _Self operator++(int){
        assert(_curNode);
        _Self tmp = *this;
        _curNode = _curNode->seq_next;
        return tmp;
    }

    bool operator==(const _Self& another) const {
        return _curNode == another._curNode;
    }

    bool operator!=(const _Self& another) const {
        return _curNode != another._curNode;
    }

};

/**
 * @brief HashMap const迭代器
 */
template < typename Key, typename Value > 
struct _HashMapConstIterator{
    typedef const _HashNode < Key, Value > _Node;
    typedef _HashMapConstIterator < Key, Value > _Self;
    typedef _HashMapIterator < Key, Value > iterator;
    const _Node *_curNode;

    _HashMapConstIterator():_curNode(){}
    
    explicit
    _HashMapConstIterator(const _Node *curNode):_curNode(curNode){}
    
    _HashMapConstIterator(const iterator& x):_curNode(x._curNode){}



    const _Node& operator* () const {
        return *_curNode;
    }

    const _Node* operator->() const {
        return _curNode;
    }
    
    _Self& operator++(){
        assert(_curNode);
        _curNode = _curNode->seq_next;
        return *this;
    }

    _Self operator++(int){
        assert(_curNode);
        _Self tmp = *this;
        _curNode = _curNode->seq_next;
        return tmp;
    }

    bool operator==(const _Self& another) const {
        return _curNode == another._curNode;
    }

    bool operator!=(const _Self& another) const {
        return _curNode != another._curNode;
    }

};











/**
 * @brief 哈希表类
 */
template < typename Key, typename Value
         , typename HashFunc = KeyHash < Key >
         , typename HashEqual = KeyEqual < Key > >
         class HashMap {

public:

    /**
     * @brief 迭代器类型
     */
    typedef _HashMapIterator < Key, Value >  iterator;
    
    /**
     * @brief 迭代器const类型
     */
    typedef _HashMapConstIterator < Key, Value >  const_iterator;

/**
 * @brief 哈希表构造函数
 * 
 * @param bucketSize 哈希表桶数量，默认 4096
 * @param flags      选项，默认0
 *
 */
    HashMap(uint32_t bucketSize = 4096, uint32_t flags = 0) {

        _seqTail = NULL;
        _seqHead = NULL;
        _itCur = NULL;
        _bucketSize = bucketSize;
        _flags = flags;
        _count = 0;
        _inited = false;

        if (bucketSize <= 0) {
            return;
        }

        _table = new HashNodePtr[bucketSize];
        if (!_table) {
            return;
        }
        memset(_table, 0, bucketSize * sizeof(HashNodePtr));

        _inited = true;
    }

/**
 * @brief 析构函数
 */
    virtual ~HashMap(void) {
        HashNodePtr pnode = NULL;
        HashNodePtr ptmp = NULL;

        for (pnode = _seqHead; pnode; pnode = ptmp) {
            ptmp = pnode->seq_next;
            delete pnode;
        }

        delete[]_table;
        _table = NULL;
    }

/* 未实现
    void clear(){
        ;
    }
*/

/**
 * @brief 插入一个元素到HashMap
 * @param key 关键字
 * @param value 值
 */
    inline bool insert(Key key, Value  value) {
        uint64_t hash = 0;
        uint32_t index = 0;
        hash = _keyHash(key);
        index = hash % _bucketSize;
        HashNodePtr pnode = NULL;

        for (pnode = _table[index]; pnode; pnode = pnode->next) {
            if (_keyEqual(pnode->key, key)) {
                return false;
            }
        }

        pnode = new HashNode;
//        memset(pnode, 0, sizeof(HashNode));
        pnode->next = NULL;
        pnode->seq_next = NULL;
        pnode->key = key;
        pnode->value = value;
        if (!_seqTail) {
            _seqHead = pnode;
            _seqTail = pnode;
        }
        else {
            _seqTail->seq_next = pnode;
            _seqTail = pnode;
        }
        pnode->next = _table[index];
        _table[index] = pnode;
        ++_count;
        return true;
    }

/**
 * @brief 查找一个key
 * @param key 需要查找的关键字
 *
 * @return end() 没有找到
 * @return iterator 制定位置的迭代器
 */
    inline iterator find(const Key & key) {
        
        uint64_t hash = 0;
        uint32_t index = 0;
        hash = _keyHash(key);
        index = hash % _bucketSize;
        HashNodePtr pnode = NULL;
        for (pnode = _table[index]; pnode; pnode = pnode->next) {
            if (_keyEqual(pnode->key, key)) {
                return iterator(pnode);
            }
        }
        return iterator(NULL);
    }

/**
 * @brief 查找一个key
 * @param key 需要查找的关键字
 *
 * @return end() 没有找到
 * @return iterator 制定位置的迭代器
 */
    inline const_iterator find(const Key & key) const {
        
        uint64_t hash = 0;
        uint32_t index = 0;
        hash = _keyHash(key);
        index = hash % _bucketSize;
        HashNodePtr pnode = NULL;
        for (pnode = _table[index]; pnode; pnode = pnode->next) {
            if (_keyEqual(pnode->key, key)) {
                return const_iterator(pnode);
            }
        }
        return const_iterator(NULL);
    }

/**
 * @brief 查找一个key
 *
 * @param key 需要查找的关键字
 * @param value 需要接收的value引用
 *
 * @return NULL 没有找到
 * @return ptr  _HashNode<Key,Value>结构的指针，可以直接访问，修改key,value
 */
    inline struct _HashNode <Key, Value > *find(const Key & key,
                                                Value & value) {
        uint64_t hash = 0;
        uint32_t index = 0;
        hash = _keyHash(key);
        index = hash % _bucketSize;
        HashNodePtr pnode = NULL;

        for (pnode = _table[index]; pnode; pnode = pnode->next) {
            if (_keyEqual(pnode->key, key)) {
                value = pnode->value;
                return pnode;
            }
        }

        return NULL;
    }

/**
 * @brief 重置遍历指针
 */
    void itReset() {
        _itCur = _seqHead;
    }

/**
 * @brief 遍历hash表
 * 
 * @param key [OUT] key引用
 * @param value [OUT] value引用
 *
 * @return false 已经遍历至最后一个节点
 */
    bool itNext(Key & key, Value & value) {
        if (!_itCur) {
            return false;
        }
        key = _itCur->key;
        value = _itCur->value;
        _itCur = _itCur->seq_next;
        return true;
    }

/**
 * @brief 是否存在下一个节点
 *
 * @return true 还有下一个节点（遍历未结束)
 */
    bool itHasNext() {
        if (!_itCur) {
            return false;
        }
        if (_itCur->seq_next) {
            return true;
        }
        return false;
    }

/**
 * @brief 得到HashMap中元素的数量
 *
 * @return 元素的总数
 */
    uint32_t size() const {
        return _count;
    }

/**
 * @brief 清除HashMap中所有元素
 */
    void clear(){
        
        _itCur = NULL;
        _count = 0;
        HashNodePtr pnode = NULL;
        HashNodePtr ptmp = NULL;

        for (pnode = _seqHead; pnode; pnode = ptmp){
            ptmp = pnode->seq_next;
            delete pnode;
        }
        

        for (uint32_t i = 0; i<_bucketSize; i++){
            _table[i] = NULL;
        }

        _seqHead = NULL;
        _seqTail = NULL;

    }


    iterator begin(){
        return iterator(_seqHead);
    }
/**
 * @brief 是否初始化成功
 */
    bool isInited() {
        return _inited;
    }

    const_iterator begin() const {
        return const_iterator(_seqHead);
    }


    iterator end(){
        return iterator(NULL);
    }
    
    const_iterator end() const {
        return const_iterator(NULL);
    }


private:
    typedef _HashNode < Key, Value > HashNode;
    typedef HashNode *HashNodePtr;
    uint32_t _bucketSize;
    uint32_t _flags;
    uint32_t _count; //元素的数量
    HashFunc _keyHash;
    HashEqual _keyEqual;
    HashNodePtr *_table;
    HashNodePtr _seqHead;
    HashNodePtr _seqTail;
    HashNodePtr _itCur;
    bool _inited;
};

UTIL_END;

#endif

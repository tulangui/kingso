/** \file 优先队列模板类
 *********************************************************************
 * @brief 一个基于堆排序的优先队列容器,容器中的元素类型和比较函数可以
 *         通过模板参数任意指定
 *
 * $作者： victor $
 *
 * $LastChangedBy: gongyi.cl $
 *
 * $Revision: 188 $
 *
 * $LastChangedDate: 2011-03-25 15:03:24 +0800 (Fri, 25 Mar 2011) $
 *
 * $Id: PriorityQueue.h 188 2011-03-25 07:03:24Z gongyi.cl $
 *
 ********************************************************************/
#ifndef PRIORITYQUEUE_H
#define PRIORITYQUEUE_H

#include <sys/types.h>
#include <stdio.h>
#include <assert.h>

#include "util/namespace.h"


UTIL_BEGIN;


template < class T, class Less > class PriorityQueue {
private:
    T * _heap;
    int _nMaxSize;
    Less _less;
    int _nSize;

public:
    
    /**
     * @brief 构造函数 
     */
    PriorityQueue() {
        _nSize = 0;
        _heap = NULL;
    }
    
    /**
     * @brief 构造函数 
     *
     * @param l 排序方式。（升降序）
     */
    PriorityQueue(const Less & l):_less(l) {
        _nSize = 0;
        _heap = NULL;
    }

    /**
     * @brief 析构函数
     */
    ~PriorityQueue() {
        delete[]_heap;
    }

    /**
     * @brief 初始化一个优先队列 
     *
     * @param maxSize 最大的可容纳数量
     *
     */
    void initialize(int maxSize) {
        assert(_heap == NULL);
        _nMaxSize = maxSize;
        _heap = new T[_nMaxSize + 1];
    }
    
    /**
     * @brief 设置排序方式
     *
     * @param ls 新的方式
     *
     */
    void setLess(const Less ls) {
        _less = ls;
    }


    /**
     * @brief 在p4pSortDoc里需要或者Compare函数 以便获得第n排序字段
     *
     * @return 排序方式
     */
    const Less & getLess() const {
        return _less;
    }


    /** 
     * @brief 为比较函数设置参数,注意：比较函数需要存在setOrder才行,为了减轻SScoreDoc结构体大小才设置此函数
     *
     * @param bPrimaryOrder 主顺序参数
     * @param bSecondOrder  第二顺序参数
     */
    void setCmpParameter(bool bPrimaryOrder, bool bSecondOrder) {
        _less.setOrder(bPrimaryOrder, bSecondOrder);
    }
    
    /**
     * @brief 设置最大容纳数量
     *
     * @param maxSize 数量
     */
    void setMaxSize(int maxSize) {
        _nMaxSize = maxSize;
    }


    /**
     * @brief 加一个元素到优先队列
     *
     * @param element 元素
     *
     * @return 是否成功
     */
    bool push(T element) {
        if(_nSize>=_nMaxSize){
            return false;
        }
        _nSize++;
        _heap[_nSize] = element;
        upHeap();
        return true;
    }


    /*
     * @brief 加一个元素到优先队列,如果队列已满,则当插入元素不小于最小值时插入
     *        队列,否则不插入,如果有元素插入队列返回true,否则返回false
     *
     * @param element 插入的元素
     *
     * @param 是否成功
     */
    bool insert(T element) {
        if (_nSize < _nMaxSize) {
            push(element);
            return true;
        }
        else if (_nSize > 0 && _less(element, top())) {
            _heap[1] = element;
            adjustTop();
            return true;
        }
        else {
            return false;
        }
    }

    /** 
     * @brief 当队列最小元素值被改变时,要调用该函数
     */
    void adjustTop() {
        downHeap();
    }


    /**
     * @breif 从优先队列里移去最小的元素
     *
     * @return 弹出元素
     */
    T pop() {

        if (_nSize == 1) {
            _nSize = 0;
            return _heap[1];
        }
        else if (_nSize > 0) {
            T result = _heap[1]; // 保存返回值
            _heap[1] = _heap[_nSize]; // 将最后一个值保持到最先的的一个值
            _heap[_nSize] = T();  // 最后一个值用空对象代替
            _nSize--;
            downHeap();         // 向下调整_heap
            return result;
        }
        else {
            return T();
        }
    }

    /**
     * @brief 从优先队列里移去最小的元素, 并插入一个新的元素
     *
     * @param element 要插入的元素
     *
     * @return 弹出的元素
     */
    T doPopAndPush(T element) {
        
        if (_nSize > 0) {
            T result = _heap[1]; // 保存返回值
            _heap[1] = element;  // 将第一个值设为新需要加入的元素
            downHeap();         // 向下调整_heap
            return result;
        }
        else {
            _heap[1] = element;  // 将第一个值设为新需要加入的元素
            return T();
        }
    }

    /** 
     * @brief 返回当前优先队列中最小的一个元素
     *
     * @return 最小的元素
     */
    T top() {
        if (_nSize > 0) {
            return _heap[1];
        }
        else {
            return T();
        }
    }

    /**
     * @brief 清空队列内所有元素
     */
    void clear() {
        for (int i = 0; i <= _nSize; i++) {
            _heap[i] = T();
        }
        _nSize = 0;
    }

    /**
     * @brief 返回优先队列元素个数
     *
     * @return 个数
     */
    int size() {
        return _nSize;
    }

protected:

    /** 
     * @brief 向上调整优先队列
     */
    inline void upHeap() {
        int i = _nSize;
        T node = _heap[i];       // 保存最底的元素
        int j = i >> 1;

        while (j > 0 && !_less(node, _heap[j])) {
            _heap[i] = _heap[j];  // 向上移至父节点
            i = j;
            j = j >> 1;
        }

        _heap[i] = node;         // 放置节点到正确的位置

    }

    /** 
     * @brief 向下调整优先队列
     */
    inline void downHeap() {
        int i = 1;
        T node = _heap[i];       // 保存最顶的元素
        int j = i << 1;         // 发现较小的孩子节点
        int k = j + 1;

        if (k <= _nSize && !_less(_heap[k], _heap[j])) {
            j = k;
        }

        while (j <= _nSize && !_less(_heap[j], node)) {
            _heap[i] = _heap[j];  // 向下移至孩子节点
            i = j;
            j = i << 1;
            k = j + 1;
            if (k <= _nSize && !_less(_heap[k], _heap[j])) {
                j = k;
            }
        }

        _heap[i] = node;         // 放置节点到正确的位置

    }

};

UTIL_END;

#endif                          //PRIORITYQUEUE_H

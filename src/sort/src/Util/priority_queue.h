/*********************************************************************
 *
 * File:	优先队列模板类
 * Desc:	一个基于堆排序的优先队列容器,容器中的元素类型和比较函数可以
 *			通过模板参数任意指定
 * Log :
 * 			Create by victor,  2007/5/23
 *
 *********************************************************************/
#ifndef __PRIORITY_QUEUE_H
#define __PRIORITY_QUEUE_H

#include <stdlib.h>
#include <assert.h>

template <class T, class less>
class CPriorityQueue
{
    private:
        T* heap;
        int nMaxSize;
        less  _less;

    public:
        int nSize;

    public:
        /* 构造函数 */
        CPriorityQueue()
        {
            nSize = 0;
            heap = NULL;
        }
        CPriorityQueue(const less & l) : _less(l) {
            nSize = 0;
            heap = NULL;
        }
        /* 析构函数 */
        ~CPriorityQueue()
        {
            delete[] heap;
        }
        /* 初始化一个优先队列 */
        void initialize( int maxSize )
        {
            assert(heap == NULL);
            nMaxSize = maxSize;
            heap = new T[nMaxSize+1];
        }
        inline void setless(const less ls)
        {
            _less=ls;
        }

        /* 在p4pSortDoc里需要或者Compare函数 以便获得第n排序字段 */
        inline const less& getLess() const
        {
            return _less;
        }
        /* 为比较函数设置参数,注意：比较函数需要存在setOrder才行,为了减轻SScoreDoc结构体大小才设置此函数 */
        inline void setCmpParameter(bool bPrimaryOrder, bool bSecondOrder)
        {
            _less.setOrder(bPrimaryOrder, bSecondOrder);
        }
        inline void setMaxSize( int maxSize )
        {
            nMaxSize = maxSize;
        }
        /*
         * 加一个元素到优先队列
         */
        void push( T & element )
        {
            nSize++;
            heap[nSize] = element;
            upHeap();
        }
        /*
         * 加一个元素到优先队列,如果队列已满,则当插入元素不小于最小值时插入
         * 队列,否则不插入,如果有元素插入队列返回true,否则返回false
         */
        bool insert( T & element )
        {
            if( nSize < nMaxSize )
            {
                push(element);
                return true;
            }
            else if( nSize > 0 && _less(element, heap[1]) )
            {
                heap[1] = element;
                adjustTop();
                return true;
            }
            else
                return false;
        }
        /* 当队列最小元素值被改变时,要调用该函数 */
        void adjustTop()
        {
            downHeap();
        }
        /* 从优先队列里移去最小的元素 */
        T pop()
        {
            if ( nSize == 1)
            {
                nSize = 0;
                return heap[1];
            }
            else if( nSize > 0 )
            {
                T result = heap[1];			// 保存返回值
                heap[1] = heap[nSize];		// 将最后一个值保持到最先的的一个值
                heap[nSize] = T();			// 最后一个值用空对象代替
                nSize--;
                downHeap();					// 向下调整heap
                return result;
            }
            else
                return T();
        }
        /* 从优先队列里移去最小的元素, 并插入一个新的元素 */
        T pop_and_push( T & element)
        {
            if( nSize > 0 )
            {
                T result = heap[1];			// 保存返回值
                heap[1] = element;		    // 将第一个值设为新需要加入的元素
                downHeap();					// 向下调整heap
                return result;
            }
            else
            {
                heap[1] = element;		    // 将第一个值设为新需要加入的元素
                return T();
            }
        }

        /* 返回当前优先队列中最小的一个元素 */
        inline T  top()
        {
            if( nSize > 0 )
                return heap[1];
            else
                return T();
        }
        /* 清空队列内所有元素 */
        void clear()
        {
            for( int i = 0; i <= nSize; i++ )
                heap[i] = T();
            nSize = 0;
        }
        /* 返回优先队列元素个数 */
        int size()
        {
            return nSize;
        }


    protected:
        /* 向上调整优先队列 */
        inline void upHeap()
        {
            int i = nSize;
            T node = heap[i];			  // 保存最底的元素
            int j = i >> 1;
            while( j > 0 && !_less(node, heap[j]) )
            {
                heap[i] = heap[j];			  // 向上移至父节点
                i = j;
                j = j >> 1;
            }
            heap[i] = node;				  // 放置节点到正确的位置
        }
        /* 向下调整优先队列 */
        inline void downHeap()
        {
            int i = 1;
            T node = heap[i];			// 保存最顶的元素
            int j = i << 1;				// 发现较小的孩子节点
            int k = j + 1;
            if (k <= nSize && !_less(heap[k], heap[j]))
            {
                j = k;
            }
            while (j <= nSize && !_less(heap[j], node))
            {
                heap[i] = heap[j];			  // 向下移至孩子节点
                i = j;
                j = i << 1;
                k = j + 1;
                if( k <= nSize && !_less(heap[k], heap[j]) )
                {
                    j = k;
                }
            }
            heap[i] = node;				  // 放置节点到正确的位置
        }

};

#endif //PRIORITYQUEUE_H

#include <stdio.h>
#include <stdint.h>
#include <new>
#include "TaskQueue.h"
#include "util/errcode.h"

namespace update {

TaskQueue::TaskQueue() : 
    _terminated(false), 
    _heap(NULL), 
    _size(0)
{ 
}

TaskQueue::~TaskQueue() 
{ 
    if(_heap) delete[] _heap;
    _heap = NULL;
}

int TaskQueue::init( int maxSize )
{
    _max = maxSize;
    _heap = new(std::nothrow) Session*[_max+1];
    if(NULL == _heap) {
        return -1;
    }
    return 0;
}

int32_t TaskQueue::enqueue(Session *task) 
{
    _lock.lock();
    if (unlikely(_terminated)) {
        _lock.unlock();
        return EOF;
    }
    if (unlikely(insert( task ) == false)) {
        _lock.unlock();
        return EOF;
    }
    if (_size == 1) {
        _lock.signal();
    }
    _lock.unlock();
    return KS_SUCCESS;
}

int32_t TaskQueue::dequeue(Session **taskp) 
{
    _lock.lock();
    if (unlikely(_terminated)) {
        _lock.unlock();
        return EOF;
    }
    while (_size == 0) {
        _lock.wait();
        if (unlikely(_size == 0)) {
            if (_terminated) {
                _lock.unlock();
                return EOF;
            } 
            else {
                _lock.unlock();
                return EAGAIN;
            }
        }
    }
    *taskp = pop();
    _lock.unlock();

    return KS_SUCCESS;
}

/* 加一个元素到优先队列,如果队列已满,否则返回false */
bool TaskQueue::insert( Session* element )
{
    if( _size >= _max ) {
        return false;
    }
    
    _size++;
    _heap[_size] = element;
    upHeap();
    return true;
}

/* 从优先队列里移去最小的元素 */
Session* TaskQueue::pop()
{
    if ( _size == 1)
    {
        _size = 0;
        return _heap[1];
    }
    else if( _size > 0 )
    {
        Session* result = _heap[1];		// 保存返回值
        _heap[1] = _heap[_size];		// 将最后一个值保持到最先的的一个值
        _heap[_size] = NULL;			// 最后一个值用空对象代替
        _size--;
        downHeap();					    // 向下调整_heap
        return result;
    }

    return NULL;
}

/* 向上调整优先队列 */
void TaskQueue::upHeap()
{
    int i = _size;
    Session* node = _heap[i];	// 保存最底的元素
    int j = i >> 1;
    while( j > 0 && !node->less( _heap[j] ) )
    {
        _heap[i] = _heap[j];		// 向上移至父节点
        i = j;
        j = j >> 1;
    }
    _heap[i] = node;				// 放置节点到正确的位置
}

/* 向下调整优先队列 */
void TaskQueue::downHeap()
{
    int i = 1;
    Session* node = _heap[i];   // 保存最顶的元素
    int j = i << 1;				// 发现较小的孩子节点
    int k = j + 1;
    if (k <= _size && !_heap[k]->less(_heap[j]))
    {
        j = k;
    }
    while (j <= _size && !_heap[j]->less( node ) )
    {
        _heap[i] = _heap[j];			  // 向下移至孩子节点
        i = j;
        j = i << 1;
        k = j + 1;
        if( k <= _size && !_heap[k]->less(_heap[j]) ) 
        {
            j = k;
        }
    }
    _heap[i] = node;				  // 放置节点到正确的位置
}

void TaskQueue::interrupt() 
{
    _lock.lock();
    _lock.broadcast();
    _lock.unlock();
}

void TaskQueue::terminate() 
{
    _lock.lock();
    _terminated = true;
    _lock.broadcast();
    _lock.unlock();
}

}


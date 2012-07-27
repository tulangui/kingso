#include "framework/TaskQueue.h"

FRAMEWORK_BEGIN;

TaskQueue::TaskQueue() : 
    _terminated(false), 
    _head(NULL), 
    _tail(NULL), 
    _size(0) { 
    }

TaskQueue::~TaskQueue() 
{ 
    while (_head) {
        _tail = _head->_next;
        delete _head;
        _head = _tail;
    }
}

int32_t TaskQueue::enqueue(Session *task) 
{
    _lock.lock();
    if (unlikely(_terminated)) {
        _lock.unlock();
        return EOF;
    }
    task->_prev = NULL;
    if ( _size == 0 ) {
        task->_next = NULL;
        _head = task;
        _tail = task;
        _size++;
    } 
    else {
        task->_next = _head;
        _head->_prev = task;
        _head = task;
        _size++;
    }
    if (_size == 1) {
        _lock.signal();
    }
    _lock.unlock();
    return KS_SUCCESS;
}

int32_t TaskQueue::dequeue(Session **taskp) 
{
    Session *task;
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
    task = _tail;
    if ( _size == 1 ) {
        _head = NULL;
        _tail = NULL;
    } 
    else {
        _tail = task->_prev;
        _tail->_next = NULL;
    }
    _size--;
    _lock.unlock();
    if (taskp) {
        *taskp = task;
    }
    return KS_SUCCESS;
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

FRAMEWORK_END;


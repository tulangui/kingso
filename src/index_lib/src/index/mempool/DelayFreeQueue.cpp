#include "DelayFreeQueue.h"

namespace index_mem
{

/**
 * @brief 返回队列支持的最大容量
 *
 * @return  int 队列支持的最大容量
**/
int 
DelayFreeQueue::capacity() 
{
    if (_data == NULL) {
        return 0;
    }
    return _data->_capacity;
}

/**
 * @brief 判断队列是否为空
 *
 * @return  bool 返回true就是空，false非空
**/
bool
DelayFreeQueue::empty() 
{
    if (_data == NULL) {
        return true;
    }
    return _data->_size == 0;
}

/**
 * @brief 返回队列是否已满
 *
 * @return  bool 返回true就是满，false非满
 */
bool
DelayFreeQueue::full()
{
    if (_data == NULL) {
        return true;
    }
    return _data->_size == _data->_capacity;
}

/**
 * @brief 清空队列的内容，但不回收空间
 *
 * @return  void 
 */
void
DelayFreeQueue::clear() 
{
    if (_data) {
        _data->_size = 0;
        _data->_front = 0;
        _data->_rear = 1;
    }
}

/**
 * @brief 创建队列
 *
 * @param [in qcap   : int 队列支持的最大长度
 * @return  int 成功返回0，其他失败
 */
int
DelayFreeQueue::create(struct queue_t * addr, int qcap) 
{
    //下的算法如果qcap小于2将无法正常工作
    if (qcap < 2) {
        return -1;
    }

    _data = addr;
    if (_data == NULL) {
        return -1;
    }
    clear();
    _data->_capacity = qcap;
    _dSize = sizeof(struct delay_t) * qcap + sizeof(struct queue_t);
    return 0;
}

int
DelayFreeQueue::load(struct queue_t * addr)
{
    if (addr == NULL) {
        return -1;
    }

    _data = addr;
    _dSize = sizeof(struct delay_t) * _data->_capacity + sizeof(struct queue_t);
    return 0;
}

int
DelayFreeQueue::destroy() 
{
    if (_data) {
        _data->_capacity = 0;
    }
    return 0;
}

int
DelayFreeQueue::push_back(const struct delay_t & val)  
{
    if (full()) {
        return -1;
    }
    _data->_array[_data->_rear] = val;
    ++(_data->_size);
    if (++(_data->_rear) == _data->_capacity) {
        _data->_rear = 0;
    }
    return 0;
}

int 
DelayFreeQueue::push_front(const struct delay_t &val) 
{
    if (full()) {
        return -1;
    }
    _data->_array[_data->_front] = val;
    ++(_data->_size);
    if (--(_data->_front) < 0) {
        _data->_front = _data->_capacity - 1;
    }
    return 0;
}

int
DelayFreeQueue::top_back(struct delay_t &val) 
{
    if (empty()) {
        return -1;
    }
    int idx = _data->_rear - 1;
    if (idx < 0) {
        idx = _data->_capacity - 1;
    }
    val = _data->_array[idx];
    return 0;
}

int
DelayFreeQueue::pop_back(struct delay_t &val) 
{
    if (empty()) {
        return -1;
    }
    if (--(_data->_rear) < 0) {
        _data->_rear = _data->_capacity-1;
    }
    val = _data->_array[_data->_rear];
    --(_data->_size);
    return 0;
}

int
DelayFreeQueue::top_front(struct delay_t &val) 
{
    if (empty()) {
        return -1;
    }
    int idx = _data->_front + 1;
    if (idx == _data->_capacity) {
        idx = 0;
    }
    val = _data->_array[idx];
    return 0;
}

int
DelayFreeQueue::pop_front(struct delay_t &val)  
{
    if (empty()) {
        return -1;
    }
    ++ (_data->_front);
    if (_data->_front == _data->_capacity) {
        _data->_front = 0;
    }
    val = _data->_array[_data->_front];
    -- (_data->_size);
    return 0;
}

int 
DelayFreeQueue::pop_backs(struct delay_t *val, int nums) 
{
    int cnt = 0;
    while (cnt < nums) {
        if (pop_back(val[cnt]) != 0) {
            return cnt;
        }
        ++ cnt;
    }
    return cnt;
}


}


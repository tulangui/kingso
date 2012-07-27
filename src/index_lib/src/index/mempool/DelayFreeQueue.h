#ifndef _DELAYFREEQUEUE_H
#define _DELAYFREEQUEUE_H

#include <stdio.h>
#include <stdint.h>

typedef uint32_t u_int; 

namespace index_mem {

struct delay_t {
    u_int reltime;
    u_int addres;
};

struct queue_t {
    int _capacity;
    int _size;
    int _front;
    int _rear;
    struct delay_t _array[0];
};

class DelayFreeQueue
{
private:
	struct queue_t *_data;		//队列存储数据空间
    size_t          _dSize;     //队列存储数据空间大小

public:
	DelayFreeQueue() {
		_data  = NULL;
        _dSize = 0;
	}

	~DelayFreeQueue() {
	}

	/**
	 * @brief 返回队列支持的最大容量
	 *
	 * @return  int 队列支持的最大容量
	**/
	int capacity();

	/**
	 * @brief 判断队列是否为空
	 *
	 * @return  bool 返回true就是空，false非空
	**/
	bool empty();

	/**
	 * @brief 返回队列是否已满
	 *
	 * @return  bool 返回true就是满，false非满
	**/
	bool full();

	/**
	 * @brief 清空队列的内容，但不回收空间
	 *
	 * @return  void 
	**/
	void clear();

	/**
	 * @brief 创建队列
	 *
	 * @param [in qcap   : int 队列支持的最大长度
	 * @return  int 成功返回0，其他失败
	**/
	int create(struct queue_t * addr, int qcap);

    int load(struct queue_t * addr);

	int destroy();
	
	int push_back(const struct delay_t & val);

	int push_front(const struct delay_t &val);

	int top_back(struct delay_t &val);
	
	int pop_back(struct delay_t &val);

	int top_front(struct delay_t &val);

	int pop_front(struct delay_t &val);

	int pop_backs(struct delay_t *val, int nums); 	
};
}
#endif


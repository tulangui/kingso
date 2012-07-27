/***********************************************************************************
 * Describe : queryparser输出的解析结果类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-07
 **********************************************************************************/


#ifndef _QP_CLASS_QPRESULT_H_
#define _QP_CLASS_QPRESULT_H_


#include "Param.h"
#include "FilterList.h"
#include "util/MemPool.h"
#include "qp_syntax_tree.h"

namespace queryparser {

class QPResult
{
	public:
		
		/**
		 * 构造函数
		 *
		 * @param mempool 内存池
		 */
		QPResult(MemPool* memPool);
		
		/**
		 * 析构函数
		 */
		~QPResult();
		
		/**
		 * 获取querystring解析成key-value的结果
		 *
		 * @return !NULL key-value解析结果
		 *         NULL  该类构造出错
		 */
		Param* getParam();
		
		
		
		/**
		 * 获取内存池
		 *
		 * @return !NULL 内存池
		 *         NULL  获取出错
		 */
		MemPool* getMemPool();
		
		/**
		 * 计算query序列化结果大小
		 *
		 * @return query序列化结果大小
		 */
		int getSerialSize();
		
		/**
		 * 序列化query
		 *
		 * @param buffer 保存序列化结果的buffer
		 * @param size buffer大小
		 *
		 * @return 序列化结果大小
		 */
		int serialize(char* buffer, int size);
		
		/**
		 * 反序列化
		 *
		 * @param buffer 序列化字符串
		 * @param size 序列化字符串长度
		 * @param memPool 内存池
		 *
		 * @return ==0 反序列化成功
		 *         < 0 反序列化失败
		 */
		int deserialize(char* buffer, int size, MemPool* memPool);
		
		/**
		 * 打印query解析结果，调试时使用
		 */
		void print();
		
        qp_syntax_tree_t *getSyntaxTree();

        FilterList *getFilterList();
    private:
		
        void init();
	


	private:
		
		Param *_param;                    // sort&everyone		
		FilterList* _filter_list;          // filter
		
		qp_syntax_tree_t *_qp_tree;    // 语法树实体
		
        MemPool* mMemPool;

};


}
#endif

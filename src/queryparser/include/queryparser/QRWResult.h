/***********************************************************************************
 * Describe : Query Parser Rewriter 输出的解析结果类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-05-25
 **********************************************************************************/


#ifndef _QP_CLASS_QRWRESULT_H_
#define _QP_CLASS_QRWRESULT_H_


#include "util/MemPool.h"


namespace queryparser{

class QRWResult
{
	public:
		
		/**
		 * 构造函数
		 *
		 * @param mempool 内存池
		 */
		QRWResult(MemPool* memPool);
		
		/**
		 * 析构函数
		 */
		~QRWResult();
		
		/**
		 * 获取rewrite query存储空间
		 *
		 * @return rewrite query存储空间
		 */
		char* getRewriteQuery();
		
		/**
		 * 创建rewrite query存储空间
		 *
		 * @param size rewrite query存储空间大小
		 *
		 * @return !NULL 创建成功
		 *         NULL  该类构造出错
		 */
		char* createRewriteQuery(int size);
		
		/**
		 * 获取内存池
		 *
		 * @return !NULL 内存池
		 *         NULL  获取出错
		 */
		MemPool* getMemPool();
		
		/**
		 * 打印query rewrite结果，调试时使用
		 */
		void print();
		
	private:
		
		char* mRewriteQuery;    // rewrite query存储空间
		
		MemPool* mMemPool;      // 内存池

};

}


#endif

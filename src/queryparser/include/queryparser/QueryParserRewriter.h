/***********************************************************************************
 * Describe : Query Parser Rewriter 调用接口类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-05-25
 **********************************************************************************/


#ifndef _QP_CLASS_QUERYPARSERREWRITER_H_
#define _QP_CLASS_QUERYPARSERREWRITER_H_


#include "framework/Context.h"
#include "framework/namespace.h"
#include "mxml.h"
#include "util/MemPool.h"
#include "queryparser/QRWResult.h"


namespace queryparser{

class QueryParserRewriter
{
	public:
		
		/**
		 * 无参构造函数
		 */
		QueryParserRewriter();
		
		/**
		 * 析构函数
		 */
		~QueryParserRewriter();
		
		/*
		 * 初始化QueryParserRewriter
		 *
		 * @param config 配置集合
		 *
		 * @return < 0 初始化出错
		 *         ==0 初始化成功
		 */
		int init(mxml_node_t* config);
		
		/*
		 * 重写一个查询条件
		 *
		 * @param context 上下文环境
		 * @param qrwresult 重写结果
		 * @param querystring 查询条件字符串
		 *
		 * @return < 0 解析出错
		 *         ==0 解析成功
		 */
		int doRewrite(const FRAMEWORK::Context* context, QRWResult* qrwresult, const char* querystring);
		
	private:
    
    MemPool mMemPool;    // 内存池
    void* mDetail;       // 相关数据
    
};

}


#endif

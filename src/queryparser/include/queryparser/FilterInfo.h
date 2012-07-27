/***********************************************************************************
 * Describe : queryparser输出的过滤信息存储结构的虚基类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-13
 **********************************************************************************/


#ifndef _QP_CLASS_FILTERINFO_H_
#define _QP_CLASS_FILTERINFO_H_


#include "QPTypes.h"
#include <stdint.h>


namespace queryparser{

class FilterInfo
{
	public:
		
		FilterInfo() {}
		virtual ~FilterInfo() {}
		
		/**
		 * 获取节点个数
		 *
		 * @return < 0 执行错误
		 *         >=0 节点个数
		 */
		virtual const int getNum() const = 0;
		
		/*
		 * 获取旺旺在线状态
		 *
		 * @return 旺旺在线状态
		 */
		virtual const OluStatType getOluStatType() const = 0;
		
		/*
		 * 使游标指向第一个数据
		 */
		virtual void first() = 0;
		
		/*
		 * 获取下一组数据
		 *
		 * @param fieldName 过滤项对应的字段名
		 * @param fieldItems 过滤项对应的取值范围
		 * @param number 过滤项对应的取值范围数组长度
		 * @param type 过滤项对应的取值类型
		 * @param relation 过滤项对应的关系（是/非）
		 *
		 * @return < 0 遍历完成
		 *         ==0 正确返回结果
		 */
		virtual int next(char* &fieldName, char* &appendFieldName, FilterItem* &fieldItems, uint32_t &number,
			FilterItemType &type, bool &relation) = 0;
};

}


#endif

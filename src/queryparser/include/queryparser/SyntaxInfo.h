/***********************************************************************************
 * Describe : queryparser输出的语法存储结构的虚基类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-10
 **********************************************************************************/

#ifndef _QP_CLASS_SYNTAXINFO_H_
#define _QP_CLASS_SYNTAXINFO_H_


#include "QPTypes.h"


namespace queryparser{

class SyntaxInfo
{
	public:
		
		SyntaxInfo() {}
		virtual ~SyntaxInfo() {}
		
		/**
		 * 获取节点个数
		 *
		 * @return < 0 执行错误
		 *         >=0 节点个数
		 */
		virtual const int getNum() const = 0;
		
		/**
		 * 设置一次遍历的开始
		 *
		 * @param order 遍历的顺序（<0，前序遍历；==0，中序遍历；>0，后序遍历）
		 */
		virtual void first(int order) = 0;
		
		/**
		 * 获取下一个节点的内容
		 *
		 * @param fieldName 指向当前节点所存储的域名（如果当前节点是叶子节点）
		 * @param fieldValue 指向当前节点所存储的域值（如果当前节点是叶子节点）
		 * @param relation 当前节点的左右子树的关系（如果当前节点是非叶节点）
		 *
		 *  @return < 0 遍历完成
		 *          ==0 当前节点是非叶节点，返回关系
		 *          > 0 当前节点是叶子节点，返回内容
		 */
		virtual int next(char* &fieldName, char* &fieldValue, NodeLogicType &relation) = 0;
		
};

}


#endif

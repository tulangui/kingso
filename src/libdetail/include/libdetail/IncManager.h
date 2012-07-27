/***********************************************************************************
 * Describe : detail更新接口
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-05-05
 **********************************************************************************/


#ifndef _DI_CLASS_INCMANAGER_H_
#define _DI_CLASS_INCMANAGER_H_


#include "commdef/commdef.h"
#include "commdef/DocMessage.pb.h"


namespace detail{

class IncManager
{

	public:

		/**
		 * 获取单实例对象指针
		 *
		 * @return  NULL: error
		 */
		static IncManager* getInstance();
		
		/**
		 * 添加一个doc到detail中
		 *
		 * @param msg
		 *
		 * @return ture:成功 ; false:失败
		 */
		bool add(const DocMessage* msg);
		
		/**
		 * 删除一个doc
		 *
		 * @param msg
		 *
		 * @return ture:成功 ; false:失败
		 */
		bool del(const DocMessage* msg);
		
		/**
		 * 修改doc对应的detail信息
		 *
		 * @param msg
		 *
		 * @return ture:成功 ; false:失败
		 */
		bool update(const DocMessage* msg);
		
		/**
		 * 获取持久化了多少doc
		 *
		 * @return 返回持久化的doc数量
		 */
		inline uint32_t getMsgCount() { return mMsgCount; }

	private:

		// 构造函数、析构函数
		IncManager();
		virtual ~IncManager() {}

private:

		static IncManager* mInstance;    // 惟一实例
		
		uint32_t mMsgCount;              // 持久化doc数量
		FILE* mMsgcntFile;               // 持久化doc数量文件
		
		char* mStrBuffer;
		int mStrBuflen;
		
		int* mFieldPointer;
		int mFieldPoinlen;

};

}


#endif

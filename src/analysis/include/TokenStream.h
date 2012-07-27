/******************************************************************************
*
* File     :TokenStream.h
* Version  :$Id: TokenStream.h,v 1.8 2005/03/29 02:54:32 jamesyu Exp $
* Desc     :token流的接口类
*			
* Log      :Created by yusiheng, 2004-1-11
*
******************************************************************************/

#ifndef TOKENSTREAM_H
#define TOKENSTREAM_H

namespace analysis 
{
class CToken;

class CTokenStream
{
protected:
	CTokenStream *pNext;		//指向下一个tokenStream

public:
	/* 构造函数 */
	CTokenStream(void)
	{
		pNext = NULL;
	}
	/*空的析构函数*/
	virtual ~CTokenStream(void)
	{
		if (pNext != NULL) {
			delete pNext;
		}
	}
	/* 纯虚函数,取得下一个token对象的指针 */
	virtual CToken* next() = 0;
};

} //end namespace

#endif

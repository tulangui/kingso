/******************************************************************************
*
* File     :Tokenizer.h
* Version  :$Id: Tokenizer.h,v 1.5 2005/03/09 02:41:22 kwq Exp $
* Desc     :分解器的基类,分解器的作用是将给定的字符串,按照某种策略进行分解
*			Tokenizer是分词器的制造者,过滤器必须在此之上
*			
* Log      :Created by yusiheng, 2004-1-11
*
******************************************************************************/
#ifndef TOKENIZER_H
#define TOKENIZER_H

#include "Token.h"
#include "TokenStream.h"
#include "charset_func.h"

namespace analysis {

class CTokenizer : public CTokenStream
{
public:
	/* 构造函数,为了没一个tokenizer分词出来的token不被重复构建,特在基类里面创建一个全局的token对象 */
	CTokenizer(const char* szInput)
	{
		m_nSLen = 0;
        strncpy(m_szInput, szInput, strlen(szInput));
        m_szInput[strlen(szInput)] = 0;
	//	m_szInput = szInput;
		m_nStart = 0;				//默认是从第一个字符开始
		m_pToken = new CToken();
	}
	CTokenizer()
	{
	   // m_szInput = NULL;
	    m_nStart = 0;
	    m_pToken = new CToken();
	}
	virtual void setText(const char* szInput,basis::n32_t nSLen)
    {
        code_utf8_to_gbk(m_szInput, DEFAULT_BUFFER_LENGTH, szInput, CC_FLAG_IGNORE);
	   // m_szInput = szInput;
		m_nSLen = strlen(m_szInput);
	    m_nStart = 0;
	    if (m_pToken != NULL) {
	        m_pToken->m_nFeatureMask = 0;
	        m_pToken->m_nStartOffset = 0;
		    m_pToken->m_nEndOffset = 0;
		    m_pToken->m_nSerial = 0;
		    m_pToken->m_pText = NULL;
		    m_pToken->m_category = -1;
	    }
	}
	/* 稀构函数 */
	virtual ~CTokenizer(void)
	{
		delete m_pToken;
		m_pToken = NULL;
	}

protected:
	/* 用于分词的原字符串 */
	char m_szInput[DEFAULT_BUFFER_LENGTH];
	/* 保存分解出来的token信息,由构造函数传入 */
	CToken *m_pToken;
	/* 开始分解token的源字符串的位置 */
	basis::n32_t m_nStart;
	/* 加入分词源字符串长度 */
	basis::n32_t m_nSLen;
};

}
#endif


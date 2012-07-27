/******************************************************************************
 *
 * File     :Token.h
 * Version  :$Id: Token.h,v 1.32 2007/01/22 07:01:22 regerett Exp $
 * Desc     :从一串字符串中分解出来的最小单位,也称之为term,同时也包含了在分解串中
 *			的位置信息和这个term类型的标识
 *
 * Log      :Created by yusiheng, 2004-1-11
 *
 ******************************************************************************/
#ifndef TOKEN_H
#define TOKEN_H

#include <string.h>
#include <stdio.h>
#include "BasisDef.h"
#include "charset_func.h"

namespace analysis 
{

class CToken
{
public:
	static const basis::u_n16_t IS_FINISHWORD_MASK = 0x1;		//第1位是否所属Field的最后一个Loc特征
	static const basis::u_n16_t IS_FIRSTWORD_MASK = 0x2;		//第2位是否所属Field的第一个Loc特征

public:
	/* 构造函数 */
	CToken(void)
	{
		m_nStartOffset = 0;
		m_nEndOffset = 0;
		m_nSerial = 0;
		m_pText = NULL;
		m_category = -1;
	}
	/* 稀构函数 */
	virtual ~CToken(void)
	{}

/* 为了性能上的考虑,将m_szText改为public,可以直接访问 */
public:
	basis::u_n32_t m_nStartOffset;					//起始位置,从原始字串开始的偏移位置
	basis::u_n32_t m_nEndOffset;						//结束位置,从原始字串开始的偏移位置
	basis::u_n32_t m_nSerial;							//Token序号,也称为token的位置,用来确定两个token是否邻接
	basis::u_n16_t m_nFeatureMask;					//特征集,里面的成员值为1表示有该项成员特征
	size_t m_nTextLen;							//分解出的token长度
	char m_szText[MAX_TOKEN_LENGTH + 1];		//保存分解出的token,由于要对关键子做转化,故要拷贝源字符串
	const char *m_pText;						//指向源数据字符串的指针,用于stopFilter或其他需要上下文的过滤功能
	basis::n32_t m_category;      //类目

/* TokenStream对Token的访问接口 */
public:
	CToken& operator =(const CToken& token)
	{
    	if (this == &token) 
        	return *this;
		m_nStartOffset = token.m_nStartOffset;
		m_nEndOffset = token.m_nEndOffset;
		m_nSerial = token.m_nSerial;
		m_nFeatureMask = token.m_nFeatureMask;
		m_nTextLen = token.m_nTextLen;
		strncpy(m_szText, token.m_szText, token.m_nTextLen);
		m_szText[m_nTextLen] = 0;
		m_pText = token.m_pText;
		m_category = token.m_category;
		return *this;
	}
	void print()
	{
		printf("m_nStartOffset = %d\n", m_nStartOffset);
		printf("m_nEndOffset = %d\n", m_nEndOffset);
		printf("m_nSerial = %d\n", m_nSerial);
		printf("m_nFeatureMask = %d\n", m_nFeatureMask);
		printf("m_nTextLen = %d\n", m_nTextLen);
		printf("m_szText = %s\n", m_szText);
		printf("m_pText = %s\n", m_pText);
		printf("m_category = %d\n", m_category);
	}
	/* 设置token的text值,该token可能是两个单词或词语,他们之间连续空格必须只保留为一个 */
	void setText(const char* szText, size_t nTextLen)
	{
		m_pText = szText;
		if (nTextLen > MAX_TOKEN_LENGTH) {	//受最大token长度限制
			nTextLen = MAX_TOKEN_LENGTH;
		}
        int k = 0;
        bool bSpace = false;
        for (basis::u_n32_t i = 0; i < nTextLen; i++) {
        	if( k==0&&szText[i]==' ' )
        		continue;
            if (szText[i]==' '){
                if (bSpace){   //去掉连续的空格
                	continue;
                }
                else {
                    m_szText[k++] = ' ';
                    bSpace = true;
                }
           }
           else {
           	m_szText[k++] = szText[i];
           	bSpace = false;
          }
     }
     if(bSpace)
     	k--;
     m_szText[k] = 0;
	 m_nTextLen = k;
    }
	/*增加序号*/
	void incrementPos()
	{
		m_nSerial++;
	}
	/* 每次要给Token对象set数据的时候,都需要先调用这个方法,将原来的feature的值清除 */
	void clearFeature()
	{
		m_nFeatureMask = 0;
	}
	/* 将某种feature的值,与feature的mask进行 or 操作*/
	void setFeature(basis::u_n16_t mask)
	{
		m_nFeatureMask |= mask;
	}
    void toUTF8()
    {
        char szTmp[MAX_TOKEN_LENGTH+1];
        code_gbk_to_utf8(szTmp, MAX_TOKEN_LENGTH+1, m_szText, CC_FLAG_IGNORE);
        strncpy(m_szText, szTmp, strlen(szTmp));
        m_szText[strlen(szTmp)] = 0;
    }
};

} //end namespace analysis

#endif

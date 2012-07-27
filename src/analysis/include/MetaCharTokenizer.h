/******************************************************************************
 *
 * File     :MetaCharTokenizer.h
 * Version  :$Id: MetaCharTokenizer.h,v 1.8 2006/12/19 02:41:39 kwq Exp $
 * Desc     :对字符串进行分解操作,分解成一个一个的字,对英文单词而言,是分解成单个
 *			 的单词,对汉字而言,是分解成字,对分解出来的字,还要判定其token的type
 * Log      :Created by yusiheng, 2005-1-11
 *
 ******************************************************************************/

#ifndef _META_CHAR_TOKENIZER_H_
#define _META_CHAR_TOKENIZER_H_

#include "Tokenizer.h"

namespace analysis {

class CToken;

class CMetaCharTokenizer : public CTokenizer
{
public:
	/* 初始化构造函数 */
	CMetaCharTokenizer(const char* szSentence);
	CMetaCharTokenizer();
	/* 析构函数 */
	~CMetaCharTokenizer(void){};
	/* 必须实现基类的纯虚函数 */
	CToken* next(void);

private:
	/*
	 * 从字符串nStartOfferset位置之后的字符,是否为EndWord
	 * 定义EndWord的概念为,在下列词之前为EndWord
	 *  英文: "," 空格 ":" "-"
	 *  中文: 中文标点符号 全角分号 全角空格 "及" "和" "或" "子"  "代理" "合作" "（图" "(图" "诚招"
	 */
	bool hasEndWord(basis::n32_t nStartOfferset) const;
	bool isTrim(const char *pchChar);
	bool isSplit(const char *pchChar);
	bool isIgnore(const char *pchChar);
	bool isChineseWord(const char *pchChar);
	const char* wStrWChr(const char *pWstr, const char *pchChar);
	const char* wStrChr(const char *pWstr, const char cChar);
    size_t isQuoted(const char *szStr);
};

}

#endif


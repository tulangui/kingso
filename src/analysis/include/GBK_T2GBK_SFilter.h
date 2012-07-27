/*
 *	GBK编码中文字符繁体到简体转换过滤器
 */
#ifndef GBK_T2GBK_S_FILTER_H
#define GBK_T2GBK_S_FILTER_H

#include "TokenFilter.h"

namespace analysis {

class CGBK_T2GBK_SFilter : public CTokenFilter
{
public:
	/* 构造函数 */
	CGBK_T2GBK_SFilter(CTokenStream *pInput) : CTokenFilter(pInput) {}
	/* 稀构函数 */
	~CGBK_T2GBK_SFilter(){}
	/* 过滤下一个token */
	CToken *next();
	/* GBK中字符转为单字节节符 */
	static long ConvertTo(const char *pchFrom, char *pchTo);

protected:
	/* 计算偏移量 */
	static inline int GBKTableOffset(unsigned char c1, unsigned char c2);
	/* GBK字符集首字节 */
	static inline bool isGBK1(unsigned char c);
	/* GBK字符集次字节 */
	static inline bool isGBK2(unsigned char c);
	/* 中文字符的判断 */
	static inline bool isChineseCharacter(unsigned char c1, unsigned char c2);
};

}
#endif

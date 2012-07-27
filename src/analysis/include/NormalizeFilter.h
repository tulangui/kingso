/******************************************************************************
 *
 * File     :NormalizeFilter.h
 * Version  :$Id: NormalizeFilter.h,v 1.9 2005/03/29 09:19:24 victor Exp $
 * Desc     :一个将token进行标准化的token,标准化的工作包括去掉连续的空格和在写
			 法上一些变化,比如路牌号都统一为阿拉伯数字等
 * Log      :Create by victor, 2004-12-24
 *
 ******************************************************************************/
#ifndef NORMALIZE_FILTER_H
#define NORMALIZE_FILTER_H

#include "TokenFilter.h"

namespace analysis {

class CToken;
class CTokenStream;

class CNormalizeFilter : public CTokenFilter
{
public:
	/* 构造函数,目前标准化只是将连续的空格去掉 */
	CNormalizeFilter(CTokenStream *pInput) : CTokenFilter(pInput) {}
	/* 稀构函数 */
	~CNormalizeFilter(void) {}
	/* 过滤下一个token */
	CToken *next();

};

}

#endif

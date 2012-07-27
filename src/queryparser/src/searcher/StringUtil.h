/*********************************************************************
 * $Author: xiaoleng.nj $
 *
 * $LastChangedBy: xiaoleng.nj $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: StringUtil.h 2577 2011-03-09 01:50:55Z xiaoleng.nj $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SEARCHER_STRINGUTIL_H_
#define SEARCHER_STRINGUTIL_H_
namespace queryparser {
/**
 * 将key-value参数值子句分析成type,name,value
 *
 * @param string key-value参数值子句
 * @param slen key-value参数值子句长度
 * @param name key-value参数值子句中的key
 * @param nlen key-value参数值子句中的key长度
 * @param value key-value参数值子句中的value
 * @param vlen key-value参数值子句中的value长度
 * @param type value的类型
 * @param tlen value的类型长度
 *
 * @return ==0 分析成功
 *         < 0 分析失败
 */
extern int _parse_keyvalue_string(char* string, int slen,
	char** name, int* nlen, char** value, int* vlen, char** type, int* tlen);

}
#endif //SEARCHER_STRINGUTIL_H_

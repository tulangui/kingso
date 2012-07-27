/*********************************************************************
 * 
 * File:	字符串处理函数集合
 * Desc:	包括字符串的复制、合并、哈希、除空格、对比(无大小)、判断、大
 *			小写的转换等多种字符串处理函数
 * Log :
 * 			Create by victor, 2007/4/23
 *
 *********************************************************************/
#ifndef STRING_UTILITY_H
#define STRING_UTILITY_H

#include <stdint.h>
#include "util/namespace.h"
#include "util/MemPool.h"
#include "util/Vector.h"

UTIL_BEGIN;


#define SAFE_STRLEN(str) (int32_t)(((str)==NULL)?0:strlen(str))
#define SAFE_MEMCPY(dest,src,len) (src==NULL)?(*dest=0,(void*)dest):memcpy(dest,src,len);

/* 
 * 将str按分割串delim进行切分，存入list
 */
void split(char* str, char* delim, Vector<char*> &list);
/* 
 * 在str字符串中查找字串substr，返回匹配字串的位置指针
 */
char* _strstr(char* str, char* substr, size_t len);
/* 
 * 整齐字符串,去掉字符串前后的空格,调用者要自己释放返回字符串delete[] result;
 */
char* trim(const char* szStr);
/*
 * strtok, 切分后的数组res, 调用者要自己释放返回的数组中的每一个字符串
 */
void strtok(Vector<char *>& res, char *szInput, int len, char *szChars);
/*
 */
char * strtok_r(char *s1, const char *s2, char **lasts);
/*
 *	复制字符串,函数内部分配内存空间,调用者必须负责释放返回的字符串
 */
char* replicate(const char* szStr);
/*
 *	复制字符串,传入内存分配管理器, 调用者不需负责返回字符串的内存释放
 */
char* replicate(const char* szStr, MemPool *pHeap, int32_t nLength=0);
/*
 *	将一个long型的值转换为字符串，传入内存分配管理器，调用者不需负责返回字符串的内存释放
 */
char* longtostr(long n, MemPool *pHeap);
char* doubletostr(double d, MemPool *pHeap);
char* inttostr(int32_t n, MemPool *pHeap);

/*
 * 封装字符串转长整型函数
 */
int64_t strtolong(const char* szText);
/*
 *	判断两个字符串是否内容相同,区分大小写
 */
bool equal(const char* szStr1, const char* szStr2);
/* 和上面的方法不同，这里要比较str1和str2是否为null */
bool safe_equal(const char* szStr1, const char* szStr2);
/*
 *	判断两个字符串是否内容相同,不区分大小写
 */
bool equalNoCase(const char* szStr1, const char* szStr2);
/*
 *	把两个字符串的内容相加,复制到新的字符串,调用者必须负责释放合并后的字符串
 */
char* mergeString(const char* szStr1, const char* szStr2);
char* mergeString(const char* szStr1, const char* szStr2, MemPool *);
/*
 *	判断字符串是否全为ASCII字符
 */
bool isAllASCII(const char* szStr);
/*
 *	判断字符串是否全为GBK中文字符
 */
bool isAllGBK(const char* szStr);
/*
 *	把字符串中的每个字符都转为大写字符,通过参数指针改变原值
 */
void toUpper(char* szStr);
/*
 *	把字符串中的每个字符都转为小写字符
 */
void toLower(char* szStr);
/*
 *	将字符串中某个字符(cRes)替换为指定的字符(cDest), 并返回替换后的字符串, 调用者负责释放替换后的字符串
 */
char* replace(const char* szStr, char cRes, char cDest);
/*
 *	过滤一个html文档中的html标签，并返回过滤后的字符串，调用者负责释放替换后的字符串
 *  add by chucai 2007.10.15
 */
int32_t filterHtml(char * szHtml,int32_t nLen, bool bTrans, bool bTaobao = false);
/*
 *	过滤一个Query 中的 stop words ，并返回过滤后的字符串，调用者负责释放替换后的字符串
 *  add by chucai 2007.10.15
 */
int32_t filterQuery(char * szHtml,int32_t nLen,bool bTrans, bool bTaobao = false); 
/*
 *	判断整个字符串是否都为数字
 */
bool isInteger(const char* szStr);
/*
 *	url encode, 请使用urlEncode2, urlEncode有bug
 */
void urlEncode(char* buf, const char* str);
void urlEncode2(char *buf, int buf_len, const char *url);

/*
 *  url decode
 */
void urlDecode(char * szQuery);

//以下为字符判断函数
/*
 *	判断是否为空格,包括单字节的'\t','\r','\n'等字符
 */
inline bool isSpace(char ch) {
	return (ch) == ' ' || (ch) == '\t' || (ch) == '\r' || (ch) == '\n' ;
}
/*
 *	判断是否为单字节的字母
 */
inline bool isAlpha(char ch) {
	return (((ch) >= 'a' && (ch) <= 'z') || ((ch) >= 'A' && (ch) <= 'Z'));
}
/*
 *	判断是否为双字节的字母
 */
inline bool isChineseAlpha(unsigned char c1, unsigned char c2) {
	return (c1==0xA3) && (((c2>=0xC1) && (c2<=0xDA)) || ((c2>=0xE1)&&(c2<=0xFA))) ;
}
/*
 *	判断是否为单字节的数字
 */
inline bool isNum(char ch) {
	return ( (ch) >= '0' && (ch) <= '9' );
}
/*
*	判断字符串是否为数字表达
*/
inline bool isAllNumber(const char *szStr) {
	if (!szStr){
		return false;
	}
	int iLen = strlen(szStr);
	for (int i=0; i<iLen; i++){
		if ((!isNum(szStr[i])) && (szStr[i]!='.')){
			if ( (i==0) && ((szStr[i] == '+')||(szStr[i] == '-')) ){
				continue;
			}
			return false;
		}
	}
        return  true;
}

/*
 *	判断是否为字母(包括单字节和双字节)
 */
inline bool isWholeAlpha(const char *pStr) {
	return (isAlpha(*pStr) || isChineseAlpha(pStr[0], pStr[1]));
}

/*
 *	判断是否为单字节的字母和数字
 */
inline bool isAlphaNum(const char ch) {
	return (isAlpha(ch) || isNum(ch));
}


UTIL_END;

#endif

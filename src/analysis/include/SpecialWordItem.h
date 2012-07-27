/******************************************************************************
 *
 * File     :SpecialWordItem.h
 * Version  :$Id: SpecialWordItem.h,
 * Desc     :nCategory：对于专业词典，该字段表示专业词所在的类目，对于通用词典该字段表示词性
 * （名词，动词），即一些词属于名词这个类目，两者都用一个ID来表示，对于专业词典有一个category类
 * 来解释每一个ID的意思和类目的层次关系，对于通用词典，用一个wordKind类来解释每一个id的意思。
 * 可用于扩展的接口 注释：对于专业词典，该字段表示专业词所在的类目，对于通用词典该字段表示
 * 词性（名词，动词）即一些词属于名词这个类目，两者都用一个int来表示。
 * Log		:Created by victor, 2004-12-29  modified by xionglin 
 *
 ******************************************************************************/

#ifndef SPECIAL_WORD_ITEM_H
#define SPECIAL_WORD_ITEM_H

#include <stdio.h>
#include <vector>
#include <string>
#include "BasisDef.h"
#include "StringUtility.h"
#include "InnerWordItem.h"

using namespace utility;

namespace analysis {

/* gbk编码的0xA3A1-0xA3FD对应ASCII的0x21-0x7D */
#define CC_ASCII(c1,c2) ((unsigned char)(c2)-0x80)
/* 将中文字转化为从0开始的索引号 */
#define CC_ID(c1,c2) ((unsigned char)(c1)-0xB0)*94+((unsigned char)(c2)-0xA1)

/* 关键字类型表 */
enum EWordType {
	WT_DELIMITER = 0,		//中英文标点符号(,/，等)
	WT_CHINESE = 1,			//中文汉字(汉)
	WT_ALPHA = 2,			//单字节符号(a-z,A-Z)
	WT_NUM = 3,				//单字节数字(0-9)
	WT_INDEX = 4,			//索引号	(I-IV)
	WT_LETTER = 5,			//双字节字母
	WT_SPACE = 6,			//空格（包括中英文的空格）
	WT_OTHER = 7			//其他
};

/* 产品关键字的类别 */
enum EWordKind {
	WK_Independence = 'i',   		//独立词(从词语组成上来区分)
	WK_Complex = 'c',				//复合词
	WK_Category = 'k',				//独立类目(该类目是一个独立的产品名)
	WK_Concept = 'p',				//复合类目（概括某系列产品的类目）
	WK_Auto = 'a',
	WK_Other = 'o'					//品牌，材料，品种，产地等其他类型的词
};

/* 属性类型 */
enum EFeatureType {
	FT_Text = 't',
	FT_String = 's',
	FT_Enum = 'e',
	FT_Integer = 'i',
	FT_Float = 'f',
	FT_Range = 'r'
};

/* 属性级别 */
enum EFeatureLevel {
	FL_Essential = 'e',				//必填项
	FL_Optional = 'o',				//可选项
	FL_Triggered = 't',				//该属性是由其他属性触发得到了，即有A才有B
	FL_Reserved = 'r'				//保留项,即该属性可能还不应用
};

/* 产品关键字属性结构 */
struct SWordFeature {
	basis::n32_t nId;						//属性ID
	char *pszFeature;				//属性
	EFeatureType type;				//字段类型
	EFeatureLevel level;			//属性级别,必填项,可选项,触发项,保留项
	basis::n32_t nParentId;				//前置属性ID
	char* pszForwardValue;			//前置属性的某个值,有些属性的存在是通过有A值才有B属性的关系
	std::vector<char*> values;			//属性值, 每个属性可以有很多特征值
	/* 默认构造 */
	SWordFeature()
	{
		pszFeature = NULL;
		pszForwardValue = NULL;
	}
	/* 拷贝构造函数 */
	SWordFeature( const SWordFeature& wFeature )
	{
		nId = wFeature.nId;
		pszFeature = string_utility::replicate( wFeature.pszFeature );
		type = wFeature.type;
		level = wFeature.level;
		nParentId = wFeature.nParentId;
		pszForwardValue = string_utility::replicate( wFeature.pszForwardValue );
		for( size_t i = 0; i < wFeature.values.size(); i++ )
		{
			values.push_back( string_utility::replicate(wFeature.values[i]) );
		}
	}
	/* 重载赋值操作符 */
	SWordFeature& operator=( const SWordFeature& wFeature )
	{
		if(this!=&wFeature)
		{
			nId = wFeature.nId;
			if(pszFeature) delete[] pszFeature;
			pszFeature = string_utility::replicate( wFeature.pszFeature );
			type = wFeature.type;
			level = wFeature.level;
			nParentId = wFeature.nParentId;
			if(pszForwardValue) delete[] pszForwardValue;
			pszForwardValue = string_utility::replicate( wFeature.pszForwardValue );
			for( size_t i = 0; i < values.size(); i++ )
			{
				if(values[i]) {
					delete[] values[i];
					values[i] = NULL;
				}
			}
			for( size_t i = 0; i < wFeature.values.size(); i++ )
			{
				values.push_back( string_utility::replicate(wFeature.values[i]) );
			}
		}

		return *this;
	}
	/* 析构函数 */
	~SWordFeature()
	{
		if(pszFeature) delete[] pszFeature;
		if(pszForwardValue) delete[] pszForwardValue;
		for( size_t i = 0; i < values.size(); i++ )
		{
			if(values[i]) delete[] values[i];
		}
	}
};

class CSpecialWordItem {
public:
	char* pszWord;		 	   //词(关键字)
	basis::n32_t nCategory;            //类目
	basis::n32_t nFrequency;	   //词的频率

public:
	std::vector<char*> synonyms;		//同义词
 	EWordKind kind;				//词的类型,独立词,复合词
	std::vector<SRelationWord> rWords;	//相关词数组
	std::vector<SWordFeature> wFeatures;	//产品关键字属性
	std::vector<char*> alias;               //别名

public:
	/* 空构造函数 */	
	CSpecialWordItem();	
	/* 构造函数 */	
	CSpecialWordItem( const char* pszWord, basis::n32_t nCategory = 0, EWordKind kind = WK_Other);	
	/* 拷贝构造函数 */	
	CSpecialWordItem( const CSpecialWordItem& wordItem );
	/* 析构函数 */	
	virtual ~CSpecialWordItem();
	/*获取关键字类型*/
	EWordType getWordType();
	/*获取关键字的哈希值*/
	basis::n32_t hashCode();
	/* 取得数据成员变量的总字节数 */
	virtual size_t getSize();
	/*将数据成员拷贝到从指定指针开始的内存中*/
	void outputData(char*& pPtr);
	/*将数据成员从指针指针处考入内存*/
	void inputData( char*& pPtr );
	/*打印出SpecialWordItem的数据内容*/
	void print();
};

}

#endif

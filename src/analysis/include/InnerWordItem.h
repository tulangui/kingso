/*********************************************************************
 * 
 * File:	InnerWordItem.h 
 * Desc:	这个类用来表示写mmap文件的wordItem类型
 * Log :
 * 	Create by regerett, 2007/5/21
 *
 *********************************************************************/

#ifndef INNER_WORD_ITEM_H
#define INNER_WORD_ITEM_H

#include <string>
#include "BasisDef.h"
#include "StringUtility.h"

namespace analysis {
	/* 相关词的级别 */
	enum ERelationLevel {
		RL_Upper = 'u',					//上位词
		RL_Lower = 'l',					//下位词
		RL_Contain = 'c',				//蕴涵关系词
		RL_Relation = 'r',				//同级别相关词
		RL_Accessory = 'a',				//附件或亲密关系的相关词
		RL_Other = 'o'					//其他相关词
	};

	struct SRelationWord {
		char* m_pKey;					//相关词关键字
		basis::n32_t m_nCategory;		//相关词所在的类目
		basis::n32_t m_nRank;			//相关度
		ERelationLevel m_level;			//相关类型
		/* 默认构造 */
		SRelationWord()
		{
			m_pKey = NULL;
		}
		SRelationWord(const SRelationWord &rRelWord) {
			m_pKey = utility::string_utility::replicate(rRelWord.m_pKey);
			m_nCategory = rRelWord.m_nCategory;
			m_nRank = rRelWord.m_nRank;
			m_level = rRelWord.m_level;
		}
		
		~SRelationWord()
		{
			if(m_pKey) delete []m_pKey;
		}
		
		static size_t getSize() 
		{
			//实际存放空间为：相关度＋相关类型 ＋ 相关词的索引偏移位置
			size_t size = sizeof(basis::n32_t)  + sizeof(ERelationLevel) + sizeof(basis::n32_t);
			return size;
		}
	};
	
	namespace word_kind
	{
		typedef basis::u_n16_t TYPE;             //定义wordkind的类型
		const TYPE NLP_MARKER	= 0x1; //支持分词
		const TYPE SPE_MARKER	= 0x2; //专业词与通用词
		const TYPE CAT_MARKER	= 0x4; //类目词与非类目词
		
		void clean(TYPE &marker);
		/* 设置为可支持分词 */
		void enableNLP(TYPE &marker);
		/* 设置为不可支持分词 */
		void unenableNLP(TYPE &marker);
		/* 判断是否支持分词 */
		bool beNLP(TYPE marker);
		/* 设为专业词 */
		void setSpeWord(TYPE &marker);
		/* 设为通用词 */
		void setGenWord(TYPE &marker);
		/* 是否是通用词 */
		bool beGenWord(TYPE marker);
		/* 是否是专业词 */
		bool beSpeWord(TYPE marker);
		/* 设置为类目词 */
		void setCategoryWord(TYPE &marker);
		/* 设置为非类目词 */
		void unsetCategoryWord(TYPE &marker);
		/* 判断是否是类目词 */
		bool beCategoryWord(TYPE marker);
	}

	class CInnerWordItem 
	{
	public:
		static const basis::n32_t MAX_WORD_LENGTH = 32;	//最大关键字长度
	public:
		char *m_pKey; //关键字
		basis::n32_t m_nCategory; //类目id
		
		CInnerWordItem() {
			m_pKey = NULL;
		}
		
		CInnerWordItem(const char *pKey, basis::n32_t nCategory) {
			m_pKey = new char[strlen(pKey)+1];
			strcpy(m_pKey, pKey);
			m_nCategory = nCategory;
		}
		
		virtual ~CInnerWordItem() {
			delete []m_pKey;
		}
		
		virtual size_t getSize() 
		{
			return 0;
		}
		
	};


}


#endif

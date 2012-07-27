#include "InnerWordItem.h"

namespace analysis {
	namespace word_kind {
		void clean(TYPE &marker)
		{
			marker = 0x0;
		}
		
		/* 设置为可支持分词 */
		void enableNLP(TYPE &marker) 
		{
			marker |= NLP_MARKER;
		}
		
		/* 设置为不可支持分词 */
		void unenableNLP(TYPE &marker) 
		{
			marker &= ~NLP_MARKER;
		}
		
		/* 判断是否支持分词 */
		bool beNLP(TYPE marker)
		{
			return ((marker & NLP_MARKER) != 0);
		}
		
		/* 设为专业词 */
		void setSpeWord(TYPE &marker) 
		{
			marker |= SPE_MARKER;
		}
		
		/* 设为通用词 */
		void setGenWord(TYPE &marker) 
		{
			marker &= ~SPE_MARKER;
		}
		
		/* 是否是通用词 */
		bool beGenWord(TYPE marker)
		{
			return ((marker & SPE_MARKER) == 0);
		}
		
		/* 是否是专业词 */
		bool beSpeWord(TYPE marker)
		{
			return ((marker & SPE_MARKER) != 0);
		}
		
		/* 设置为类目词 */
		void setCategoryWord(TYPE &marker)
		{
			marker |= CAT_MARKER;
		}
		
		/* 设置为非类目词 */
		void unsetCategoryWord(TYPE &marker)
		{
			marker &= ~CAT_MARKER;
		}
		
		/* 判断是否是类目词 */
		bool beCategoryWord(TYPE marker)
		{
			return ((marker & CAT_MARKER) != 0);
		}
	}
}

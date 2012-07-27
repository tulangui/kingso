#include <iostream>
#include "StringUtility.h"
#include "SpecialWordItem.h"

using namespace std;
using namespace basis;
using namespace utility;

namespace analysis {

/*
 * Desc:	默认构造函数
 * Params:
 *			None
 * Return:
 *			None
 */
CSpecialWordItem::CSpecialWordItem()
{
	pszWord = NULL;
	nCategory = 0;
	nFrequency = 0;
	kind = WK_Other;
}
/*
 * Desc:		构造函数,传入关键字和所在的类目
 * Params:
 *			const char* p_szWord:	关键字
 *			int nCategory:			所在类目（默认值是0）
 * Return:
 *			None
 */
CSpecialWordItem::CSpecialWordItem( const char* p_szWord, n32_t nCategory, EWordKind kind)
{
	this->pszWord = string_utility::replicate( p_szWord );
	this->nCategory = nCategory;
	this->kind = kind;
	this->nFrequency = 1;
}
/*
 * Desc:	拷贝构造函数
 * Params:
 *			const CSpecialWordItem& wordItem:	另外一个词条对象
 * Return:
 *			None
 */
CSpecialWordItem::CSpecialWordItem( const CSpecialWordItem& wordItem )
{
	pszWord = string_utility::replicate(wordItem.pszWord);
	nCategory = wordItem.nCategory;
	nFrequency = wordItem.nFrequency;
	kind = wordItem.kind;
	for( size_t i = 0; i < wordItem.synonyms.size(); i++ )
	{
		synonyms.push_back( string_utility::replicate(wordItem.synonyms[i]) );
	}
	for( size_t i = 0; i < wordItem.rWords.size(); i++ )
	{
		rWords.push_back( wordItem.rWords[i] );
	}
	for( size_t i = 0; i < wordItem.wFeatures.size(); i++ )
	{
		wFeatures.push_back( wordItem.wFeatures[i] );
	}
	for( size_t i = 0; i < wordItem.alias.size(); i++ )
	{
		alias.push_back( string_utility::replicate(wordItem.alias[i]) );
	}
}
/*
 * Desc:	析构函数
 * Params:
 *			None
 * Return:
 *			None
 */
CSpecialWordItem::~CSpecialWordItem()
{
	if(pszWord) {
		delete[] pszWord;
		pszWord = NULL;
	}
	for( size_t i = 0; i < synonyms.size(); i++ )
	{
		if(synonyms[i]) {
			delete[] synonyms[i];
			synonyms[i] = NULL;
		}
	}
	for( size_t i = 0; i < alias.size(); i++ )
	{
		if(alias[i]) {
			delete[] alias[i];
			alias[i] = NULL;
		}
	}
}
/*
 * Desc:	取得关键字类型
 * Params:
 *			None
 * Return:
 *			返回定义好的关键字类型
 */
EWordType CSpecialWordItem::getWordType()
{
	unsigned char c1 = pszWord[0];
	unsigned char c2 = pszWord[1];

	if( c1 > 0x20 && c1 < 0x7F )
	{
		if( strchr( "\"!:;,.?()[]{}", c1 ) )
			return WT_DELIMITER;
		else if( c1 >= 0x30 && c1 <= 0x39 )
			return WT_NUM;
		else
			return WT_ALPHA;
	}
	else if( c1==0xA2 || (c1==0xA3&&c2>=0xB0&&c2<=0xB9) )//中文索引数字
	{
		return WT_INDEX;
	}
	else if( c1==0xA3 && (c2>=0xC1&&c2<=0xDA||c2>=0xE1&&c2<=0xFA) )//双字节的字母（a-z,A-Z）
	{
		return WT_LETTER;
	}
	else if( c1==0xA1 )
	{
		return WT_DELIMITER;
	}
	else if( c1>=0xB0&&c1<=0xF7&&c2>=0xA1 )
	{
		return WT_CHINESE;
	}
	else if( c1 == ' ' || (c1 == 0xA1&&c2== 0xA1) )
	{
		return WT_SPACE;
	}
	return WT_OTHER;
}
/*
 * Desc:	根据字符的类型,计算词典中的hash值
 * Params:
 *			None
 * Return:
 *			返回对应的hash值
 */
n32_t CSpecialWordItem::hashCode()
{
	n32_t nHash;
	EWordType type = getWordType();

	if( type == WT_CHINESE )		//中文字
	{
		nHash = CC_ID( pszWord[0],pszWord[1] );
	}
	else if( type == WT_NUM )		//单字节数字
	{
		nHash = 3755;
	}
	else if( type == WT_LETTER )	//双字节字符
	{
		nHash = 3756;
	}
	else if( type == WT_ALPHA )		//单字节字符
	{
		nHash = 3757;
	}
	else if( type == WT_INDEX || type == WT_LETTER ) //中文数字和字符
	{
		nHash = 3758;
	}
	else
	{
		nHash = 3759;
	}

	return nHash;
}
/*
 * Desc:	取得数据成员变量的总字节数
 * Params:
 *			None
 * Return:
 *			总字节大小
 */
size_t CSpecialWordItem::getSize()
{
	size_t nSize = 0;
	//关键字大小
	nSize += ( strlen(pszWord)+1 )*sizeof(char);
	//类目 + 频率
	nSize += sizeof(n32_t) + sizeof(n32_t);
	//类型
	nSize += sizeof(char);
	//同义词
	nSize += sizeof(n32_t);
	for( size_t i = 0; i < synonyms.size(); i++ )
	{
		nSize += (strlen( synonyms[i] ) + 1);
	}
	//相关词
	nSize += sizeof(n32_t);//大小
	for( size_t i = 0; i < rWords.size(); i++ )
	{
		nSize += (strlen( rWords[i].m_pKey) + 1);
		nSize += sizeof(n32_t) + sizeof(n32_t);	//类目 + 相关度
		nSize += sizeof(char);				//级别
	}
	//属性
	nSize += sizeof(n32_t);	//大小
	for( size_t i = 0; i < wFeatures.size(); i++ )
	{
		nSize += sizeof(n32_t);								//属性id
		nSize += (strlen( wFeatures[i].pszFeature ) + 1);	//属性名称
		nSize += (sizeof(char) + sizeof(char));				//字段类型　+ 属性级别
		nSize += sizeof(n32_t);								//前置属性id
		nSize += (strlen( wFeatures[i].pszForwardValue ) + 1);
		nSize += sizeof(n32_t);								//属性值个数
		for( size_t j = 0; j < wFeatures[i].values.size(); j ++ )
		{
			nSize += (strlen( wFeatures[i].values[i] ) + 1);
		}
	}
    //别名
	nSize += sizeof(n32_t);
	for( size_t i = 0; i < alias.size(); i++ )
	{
		nSize += (strlen( alias[i] ) + 1);
	}
	return nSize;
}
/*
 * Desc:	将数据成员拷贝到从指定指针开始的内存中
 * Params:
 *			char*& pPtr:	内存中存放数据的起始地址,该地址的变化返回给用户
 * Return:
 *			None
 */
void CSpecialWordItem::outputData( char*& pPtr )
{
	//关键字
	strcpy( pPtr, pszWord );
	pPtr += ( strlen(pszWord) + 1 );
	//类目
	*(n32_t*)pPtr = nCategory;
	pPtr += sizeof(n32_t);
	//词频
	*(n32_t*)pPtr = nFrequency;
	pPtr += sizeof(n32_t);
	//类型
	*(char*)pPtr = kind;
	pPtr += sizeof(char);
	//同义词
	*(n32_t*)pPtr = static_cast<n32_t>( synonyms.size() );
	pPtr += sizeof(n32_t);
	for( size_t i = 0; i < synonyms.size(); i++ )
	{
		strcpy( pPtr, synonyms[i] );
		pPtr += (strlen( synonyms[i] ) + 1);
	}
	//相关词
	*(n32_t*)pPtr = static_cast<n32_t>( rWords.size() );
	pPtr += sizeof(n32_t);	//大小
	for( size_t i = 0; i < rWords.size(); i++ )
	{
		strcpy( pPtr, rWords[i].m_pKey);
		pPtr += (strlen( rWords[i].m_pKey) + 1);
		*(n32_t*)pPtr = rWords[i].m_nCategory;	//类目
		pPtr += sizeof(n32_t);
		*(n32_t*)pPtr = rWords[i].m_nRank;
		pPtr += sizeof(n32_t);				//相关度
		*(char*)pPtr = rWords[i].m_level;
		pPtr += sizeof(char);				//级别
	}
	//属性
	*(n32_t*)pPtr = static_cast<n32_t>( wFeatures.size() );
	pPtr += sizeof(n32_t);	//大小
	for( size_t i = 0; i < wFeatures.size(); i++ )
	{
		*(n32_t*)pPtr = wFeatures[i].nId;
		pPtr += sizeof(n32_t);
		strcpy( pPtr, wFeatures[i].pszFeature );
		pPtr += (strlen( wFeatures[i].pszFeature ) + 1);	//属性名称
		*(char*)pPtr = wFeatures[i].type;					//字段类型
		pPtr += sizeof(char);
		*(char*)pPtr = wFeatures[i].level;					//属性级别
		pPtr += sizeof(char);
		*(n32_t*)pPtr = wFeatures[i].nParentId;				//前置属性id
		pPtr += sizeof(n32_t);
		strcpy( pPtr, wFeatures[i].pszForwardValue );		//前置属性的某个值
		pPtr += (strlen( wFeatures[i].pszForwardValue ) + 1);
		*(n32_t*)pPtr = static_cast<n32_t>( wFeatures[i].values.size() ); //属性值个数
		pPtr += sizeof(n32_t);
		for( size_t j = 0; j < wFeatures[i].values.size(); j ++ )
		{
			strcpy( pPtr, wFeatures[i].values[i] );
			pPtr += (strlen( wFeatures[i].values[i] ) + 1);
		}
	}
	//别名
	*(n32_t*)pPtr = static_cast<n32_t>( alias.size() );
	pPtr += sizeof(n32_t);
	for( size_t i = 0; i < alias.size(); i++ )
	{
		strcpy( pPtr, alias[i] );
		pPtr += (strlen( alias[i] ) + 1);
	}
}
/*
 * Desc:	从某个地址开始给所有数据成员赋值,该对象使用一定是New出来的空对象。如果pszWord不空就有内存泄漏  
 * Params:
 *			char*& pPtr:	内存中存放数据的起始地址,该地址的变化返回给用户
 * Return:
 *			None
 */
void CSpecialWordItem::inputData( char*& pPtr )
{
	//关键字
	size_t nLen = strlen(pPtr);
	if(pszWord != NULL)
	{
		delete[] pszWord;
		pszWord = NULL;
	}
	pszWord = new char[nLen+1];
	strcpy(pszWord, pPtr);
	pPtr += (nLen+1);
	//类目
	nCategory = *(n32_t*)pPtr;
	pPtr += sizeof(n32_t);
	//词频
	nFrequency = *(n32_t*)pPtr;
	pPtr += sizeof(n32_t);
	//产品类型
	char ch = *(char*)pPtr;
	pPtr += sizeof(char);
	switch( ch )
	{
		case 'i':
			kind = WK_Independence;
			break;
		case 'c':
			kind = WK_Complex;
			break;
		case 'k':
			kind = WK_Category;
			break;
		case 'p':
			kind = WK_Concept;
			break;
		case 'a':
			kind = WK_Auto;
			break;
		default:
			kind = WK_Other;
			break;
	};
	//同义词
	n32_t nSize = *(n32_t*)pPtr;
	pPtr += sizeof(n32_t);
	for( n32_t i = 0; i < nSize; i++ )
	{
		synonyms.push_back( string_utility::replicate( pPtr ) );
		pPtr += (strlen( pPtr ) + 1);
	}
	//相关词
	nSize = *(n32_t*)pPtr;
	pPtr += sizeof(n32_t);	//大小
	for( n32_t i = 0; i < nSize; i++ )
	{
		SRelationWord rWord;
		rWord.m_pKey= string_utility::replicate( pPtr );
		pPtr += (strlen( pPtr ) + 1);
		rWord.m_nCategory = *(n32_t*)pPtr;	//类目
		pPtr += sizeof(n32_t);
		rWord.m_nRank = *(n32_t*)pPtr;
		pPtr += sizeof(n32_t);				//相关度
		ch = *(char*)pPtr;
		pPtr += sizeof(char);				//级别
		switch( ch )
		{
			case 'u':
				rWord.m_level = RL_Upper;
				break;
			case 'l':
				rWord.m_level = RL_Lower;
				break;
			case 'c':
				rWord.m_level = RL_Contain;
				break;
			case 'r':
				rWord.m_level = RL_Relation;
				break;
			case 'a':
				rWord.m_level = RL_Accessory;
				break;
		};
		rWords.push_back( rWord );
	}
	//属性
	nSize = *(n32_t*)pPtr;
	pPtr += sizeof(n32_t);	//大小
	for( n32_t i = 0; i < nSize; i++ )
	{
		SWordFeature wFeature;
		wFeature.nId = *(n32_t*)pPtr;
		pPtr += sizeof(n32_t);
		wFeature.pszFeature = string_utility::replicate( pPtr );
		pPtr += (strlen( pPtr ) + 1);	//属性名称
		ch = *(char*)pPtr;					//字段类型
		pPtr += sizeof(char);
		switch( ch )
		{
			case 's':
				wFeature.type = FT_String;
				break;
			case 'e':
				wFeature.type = FT_Enum;
				break;
			case 'i':
				wFeature.type = FT_Integer;
				break;
			case 'f':
				wFeature.type = FT_Float;
				break;
			case 'r':
				wFeature.type = FT_Range;
				break;
		};
		ch = *(char*)pPtr;					//属性级别
		pPtr += sizeof(char);
		switch( ch )
		{
			case 'e':
				wFeature.level = FL_Essential;
				break;
			case 'o':
				wFeature.level = FL_Optional;
				break;
			case 't':
				wFeature.level = FL_Triggered;
				break;
			case 'r':
				wFeature.level = FL_Reserved;
				break;
		};
		wFeature.nParentId = *(n32_t*)pPtr;				//前置属性id
		pPtr += sizeof(n32_t);
		wFeature.pszForwardValue = string_utility::replicate(pPtr);		//前置属性的某个值
		pPtr += (strlen( pPtr ) + 1);
		n32_t nValueSize = *(n32_t*)pPtr;			//属性值个数
		pPtr += sizeof(n32_t);
		for( n32_t j = 0; j < nValueSize; j ++ )
		{
			wFeature.values.push_back( string_utility::replicate( pPtr ) );
			pPtr += (strlen( pPtr ) + 1);
		}
		wFeatures.push_back( wFeature );
	}
	//别名
	nSize = *(n32_t*)pPtr;
	pPtr += sizeof(n32_t);
	for( n32_t i = 0; i < nSize; i++ )
	{
		alias.push_back( string_utility::replicate( pPtr ) );
		pPtr += (strlen( pPtr ) + 1);
	}
}

/*打印出SpecialWordItem的数据内容*/
void CSpecialWordItem::print()
{
	cout << "keyword = " << pszWord << endl;
	cout << "Category= " << nCategory << endl;
	cout << "Frequence=" << nFrequency << endl;
	cout << "产品类型 =" << kind << endl;
	cout << "同义词 =";
	for(n32_t i = 0; i < synonyms.size(); i++)
	{
		cout << " " << synonyms[i];
	}
	cout << endl; 
	cout << "相关词 = ";
	for(n32_t i = 0; i < rWords.size(); i++)
	{
		cout << "keyword=" << rWords[i].m_pKey
		     << "Category=" << rWords[i].m_nCategory
		     << "Rank=" << rWords[i].m_nRank
		     << "Level=" << rWords[i].m_level;
	}
	cout << endl;
	cout << "属性 = ";
	for(n32_t i = 0; i < wFeatures.size(); i++)
	{
		cout << "Feature ID = " << wFeatures[i].nId 
		     << " Name= " << wFeatures[i].pszFeature 
		     << " Type= " << wFeatures[i].type
		     << " Level= " << wFeatures[i].level
		     << " 前置属性=" << wFeatures[i].nParentId
	 	     << "前置属性值=" << wFeatures[i].pszForwardValue
		     << "属性值=";
		for(n32_t j = 0; j < (wFeatures[i].values).size(); j++)
		{
			cout << (wFeatures[i].values)[j];
		}
	}
	cout << endl;
	cout << "别名 =";
	for(n32_t i = 0; i < alias.size(); i++)
	{
		cout << alias[i];
	}
	cout << endl;
}

}

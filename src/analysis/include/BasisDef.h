#ifndef GLOBAL_DEF_H
#define GLOBAL_DEF_H
#include <assert.h>
#include <cstring>

namespace basis {

/* 系列数据常量定义 */
#define MAX_TOKEN_LENGTH 128		//定义最大token长度
#define MAX_FIELD_NAME_LENGTH 128	//定义最大字段名的长度
#define MAX_PARSE_QUERY_FIELD_LENGTH 1023 // define max field length while parse query field
#define MAX_FILE_PATH 512			//定义最大文件路径长度
#define MAX_FILE_NAME_LEN 256		//最大文件名长度
#define DEFAULT_BUFFER_LENGTH 1024	//默认buffer大小,该默认值应用在各种模块中
#define STOP_WORD_SPLIT_CHAR ','	//定义stop word之间的分隔字符 ','

#define TEXT_WORD_POS_BIT_COUNT 23
#define TEXT_FIELDBIT_COUNT 27

#define MIN_TIME_LENGTH 10			//最小的时间格式
#define MAX_TIME_LENGTH 22			//最长的时间格式

#define RESERVER_DOC_SIZE 128		//即时更新模式下,预留文档(doc)空间大小
#define RESERVER_OCC_SIZE 128		//即时更新模式下,预留位置(occ)空间大小
#define OCC_CHUNK_SIZE 260   //RESERVER_OCC_SIZE*sizeof( SOccInfo )+sizeof( SDocInfo )
#define EXTOCC_CHUNK_SIZE 388 //RESERVER_OCC_SIZE*sizeof( SExtOccInfo )+sizeof( SDocInfo )
#define OCC_READ_BUFFER_SIZE 16384	//位置信息读取时缓存空间大小,该大小必须大于IO实际的缓存值(一般为4096byte),并且为该缓存值的整数倍
#define MAX_OCCCOUNT_INONEDOC 4096  //一篇doc里Occ个数的最大值

#define DO_ERASE_DOC_COUNT    100   //当被删除的文档数超过100才开始执行清除
#define DO_ERASE_DOC_PERCENT  5     //当被删除的文档数百分比超过5%，也执行清除

#define SEGMENT_BUILD_MODE 'S'		//segment索引模式
#define INSTANT_BUILD_MODE 'I'		//即时更新(instant)索引模式
#define PROGRESSIVE_INSTANT 'P'		//进行中的即时更新模式

#define SEGMENT_ISUPDATE_MODE 'S'              //isupdate segment模式
#define INSTANT_ISUPDATE_MODE 'I'              //isupdate instant模式

#define DEFAULT_TEXT_KEY_TIMES   131072  //对于text类型的索引,索引关键字的个数默认为131072个
#define DEFAULT_STRING_KEY_TIMES 3		//对于string类型的索引,索引关键字的个数默认3倍于总文档数

#define SEGMENT_COUNT_LIMIT 128		//定义一个segmentInfos文件中最大索引数据段(segment)数量

#define INDEX_INVALIDINDEX -1 	//无效的索引值
#define INDEX_INVALIDSERIAL -2	//为了区分相关性排序和无排序而设的一个常量值,间或作为其他用途
#define INDEX_INVALID_XADRANK (INDEX_INVALIDINDEX-4)	//为第一排序指定为 XADRANK 所用
#define INDEX_INVALID_P4PPRICE (INDEX_INVALIDINDEX-5)	//为第一排序指定为 P4PPrice 所用

#define TIME_OF_DAY  86400   //一天的秒数 24*60*60 
#define FLOAT_TO_NUMBER 100 //2位小数精度,即符点数只精确2位

#define MAX_QUERY_KEYWORD_LENGTH 1024
#define MAX_QUERY_LENGTH 2048		//查询语句被允许的最大长度
#define MAX_AREA_NAME 1280            //area_name的最大长度


#define MIN_RESULT_NUM_TO_CHECK 10		//进行关键字检查的最小结果数
#define MIN_TOKENIZER_WORD_LENGTH 6		//进行分词的最小长度
#define MAX_SPELL_CHECK_WORD_LENGTH 64	//进行拼写检查的最大关键字长度
#define DEFAULT_MAX_MEMPOOL_SIZE 2048 //默认查询使用的mempool的最大size, 单位MB

#define RANK_COUNT 3	////用于排序的数组个数
#define FIRST_RANK  0      //第一排序,对应配置文件中第一排序字段
#define SECOND_RANK  1 //第二排序,对应配置文件中第二排序字段
#define THIRD_RANK  2      //临时的排序字段,用于distinct排序

  //  #define ACC_VALUE_COUNT(a) (a&0x7fffffff)     //attribute 值计数器实际值(number 类型attribute使用最高位用于标识值是否有效,'1'
  //#define IS_ACCVALUE_INVALID(a) (a&0x80000000) //检查attribute 值计数器值是否无效
  //#define ACC_VALUE_INVALID(a) (a|0x80000000)   //将attribute值计数器值设置无效

#define MAJOR_VERSION 4  //当前版本的主版本号
#define MINOR_VERSION 12  //当前版本的次版本号

#define DELETE_DELAY_TIME 200

//#define MAX_UNIQINFO_NUM 8   //用于Uniq，Uniq Info最大个数（建议小于5）
//#define MAX_UNIQINFO_CONTENT_NUM 4   //用于uniq, 指定content类型返回时，最大返回的个数

#define MAX_CUSVALUE_LENTH 512  //定义Custom方式统计的最大指定值字符串长度

#define UNION_SEARCH_DELIM "\01"
#define UNION_SEARCH_MAX_RESULT 2500000
#define UNION_SEARCH_MAX_QUERY 5

#define MAX_MULVALUE_LENTH 512  //定义多维方式统计的最大指定值字符串长度
#define MAX_MULCAT_DIM 3  //定义多维方式统计的最大维度

#define APP_KIND_P4P 1  // application for p4p 
#define APP_KIND_OTHER 0  // application for others

#define DICT_BLOCK_SIZE 64 // idx字典skiplist平均长度

#define ABNORMAL_MAX_QUERY_NUM 1000 // 高耗词表最大query数
#define ABNORMAL_MAXN 1000 // 在系统不稳定的情况小，最大允许的MAXN值
#define ABNORMAL_MAX_FILTER_NUM 3 // 在系统不稳定的情况下，最多允许的FILTER个数
#define ABNORMAL_MAX_POLAND_NUM 5 // 在系统不稳定的情况下，最多允许的query里的查询条件个数
#define ABNORMAL_MAX_KEYWORD_NUM 4 // 在系统不稳定的情况下，各查询条件最大的关键词个数（不包含TextQuery和UniqueQuery）
#define ABNORMAL_MAX_Q_LENGTH 30

#define INVALIDSLICEID 0
#define OPAND 1
#define OPOR 0
typedef char n8_t;
typedef short n16_t;
typedef int n32_t;
typedef long long n64_t;

typedef unsigned char u_n8_t;
typedef unsigned short u_n16_t;
typedef unsigned int u_n32_t;
typedef unsigned long long u_n64_t;

/* 
 * 排序分值用到的统一类型
 * 它可以代表整数(32bit或64bit),也可以代表字符串
 * 整数或字符串的区分通过最高2位来判断
 * 最高两位: 10为字符串,11为负数,其它为正数
 */
typedef n64_t ScoreType;

#define CHAR_FLAG 0x8000000000000000LL
#define HIGH_FLAG 0xC000000000000000LL
#define STR_TO_NUM(p) (((n64_t)(p)) | CHAR_FLAG)
#define NUM_TO_STR(n) (reinterpret_cast<char*>((n) ^ CHAR_FLAG))
#define IS_STRING(n) (((n) & HIGH_FLAG) != 0 && (((n) & HIGH_FLAG)^CHAR_FLAG) == 0)

/* 基本字段类型定义,类型定义是对字段而言的,不关注字段值的具体内容 */
enum EFieldType
{
	ft_text,			//文本类型,一般为可变长度需要分词的字符串
	ft_string,			//字符串类型,一般为文本整体出现既有意义
	ft_enum,			//枚举类型
	ft_integer,			//整数类型,此类型的字段可做NUMBER类型的索引
	ft_float,			//浮点数类型
	ft_long,			//长整数类型,64bit
	ft_time,			//时间类型,格式允许自定义
	ft_location,		//二维坐标类型
	ft_char,			//单字节类型(char)
    ft_online,			//online 类型
	ft_property,		//property类型
	ft_unknown,			//未知类型
	ft_2tuple,			//二元组
	ft_triple,			//三元组
	ft_join,					//辅表关联类型
    ft_short,            //16bit元素,add by wanmc
    ft_float4,            //4字节单精度浮点
	   ft_custom     //用户自定义类型，需要通过知道的loader访问 
};
//add by jinghua for Cache Type
enum ECacheType
{
	ct_default,
	ct_staticslice,
	ct_dynamicslice
};

// Uniq II --> unqi Info属性
enum EFieldInfoType
{
	fi_unknown,
	fi_sum,				//对Uniq Info进行sum
	fi_average,			//对Uniq Info进行average
	fi_min,				//对Uniq Info取min
	fi_max,				//对Uniq Info取max
	fi_content			//罗列Uniq Info
};

/* 索引类型定义 */
enum EIndexType
{
	it_text,			//文本索引，输入源为需要分词的可变长度字符串，用做模糊查找
	it_string,			//字符索引，输入源为不需分词的可变长度字符串，用做精确查找
	it_enum,			//枚举索引，输入源为字符类型的枚举，如"HangZhou"，用做快速精确查找
	it_property,		//属性索引，输入源为XML格式的字符串，用做专业化的属性搜索
	it_number,			//整数索引，输入源为各种数值，与string索引方式相同，但可做自动划分的范围查找
	it_range,			//范围索引，输入源为各种数值，用做范围查找，而且用户可以自定义范围段
	it_scope,			//区域索引，输入源为二维坐标,用做区域查找
	it_unique,          //主键索引,为了快速delete
	it_unknown			//未知索引，除上述索引之外的索引类型，不处理
};

/* 索引信息扩充类型定义 */
enum EIndexExtend
{
	ie_occ_none = 1,			//索引文件中不需要保存位置关系
	ie_occ_array = 2,			//索引文件中要保存所有的位置关系
	ie_occ_first = 4,			//索引文件中只要保存第一个位置
	ie_int = 8,					//对于String类型的索引,关键字实际上是一个整数
	ie_pk = 16,					//对于String类型的索引,关键字是唯一的,即一个primary key
	ie_md5 = 32,				//对于String类型的索引,关键字实际上是一个md5(128位)的数字
	ie_double = 64,				//对于String类型的索引,关键字实际上是一个64位的整数
	ie_range = 128,				//对于Integer类型的索引,需要对索引变形后进行范围查找功能
	ie_occ_preload = 512,			//occlist索引文件是否预加载
	ie_wildcard = 1024,			//索引支持wildcard查询功能
	ie_cache = 2048,			//对查询结果进行cache, 默认TEXT类型的PACKAGE进行cache
	ie_stopword_ignore = 4096,	//对stopword的处理，如果为true表示忽略，否则返回null结果
	ie_bitmap_preload = 8192,      //bitmap文件是否预加载
	ie_bits = 16384,			// 对Integer类型的索引,取Integer的bit位进行索引
	ie_read_position = 32768,             // 扩展读位置功能
        ie_unload = 65536,                      //不加载到内存中
	ie_index_freq = 131072,             //是否索引高频词
    ie_unknown = 262143
};

enum ELoadType
{
        lt_preload,
        lt_load,
        lt_unload,   //读取后清空page cache
        lt_directio, //实际上是读取后
        lt_unknown
};


//支持语言的类型定义
enum ESupportLanguage
{
	sl_English,
	sl_Chinese,
	sl_Traditional,
	sl_Japanese,
	sl_Korean,
	sl_unknown
};

//字段值被允许的编码类型定义
enum ESupportCoding
{
	sc_UTF8,
	sc_Unicode,
	sc_Big5,
	sc_GBK,
	sc_Ansi
};

//对字段内容处理的类型定义
enum EFieldFixed
{
	ff_Fixed,			//固定长度
	ff_Dynamic,			//动态加亮的内容摘要
	ff_Highlight,		//加亮内容摘要
	ff_Text				//全部文本
};

//定义索引过程中四种变化的状态
enum EBuildState
{
	bs_new = 'N',
	bs_merge = 'M',
	bs_dispose = 'D',
	bs_erase = 'E',
	bs_incre = 'A',
	bs_change = 'C',
	bs_unknown = 'U'
};

// 查询子句之间的连接符号
enum EQuerySign
{
	qs_and,			//逻辑与运算
	qs_or,			//逻辑或运算
	qs_not,			//逻辑非运算
	qs_left,		//左括号
	qs_right,		//右括号
	qs_prohibite,	//阻止运算，在最后的返回集合中把这个集合的所有东西都过滤掉
	qs_require,		//在返回的结果中必需有这个关键字
	qs_unknown,		//未知类型(Query)
	qs_otherwise,   // Otherwise 操作符
	qs_bothand		// bothand 操作符 (A BOTHAND B) <=> (A AND B) OR A OR B
};

//查询子句的类型

//返回结果的字段类型,这些类型指示出数据的来源或限制
enum EResultType
{
	rt_notNull,		//返回结果不能为空
	rt_attribute,	//返回结果的内容来自于attribute索引数据
	rt_union,		//返回来自union table的数据,比如在线信息等
	rt_primary,		//返回结果为第一排序分值
	rt_secondary,	//返回结果为第二排序分值
	rt_unknown,		//未指定类型, 默认结果来自detail的配置
	rt_joinarea,	//返回结果为其它Area的数据
	rt_joinarea_attribute, // 返回结果为其它Area的属性索引数据
	rt_custom  //用户自定义类型，需要通过loadr访问
};
/* 对查询输出关键字的解释说明 */
enum EKeyExplain {
	ke_unknown = 0,					//无解释说明
	ke_synonmys = 1,				//同义词搜索
	ke_category = 2,				//类目搜索
	ke_spellcheck_less = 3,			//有少量结果的拼写检查
	ke_wordtokenizer_less = 4,		//有少量结果的分词处理
	ke_spellCheck = 5,				//零结果后的拼写检查
	ke_wordtokenizer = 6,			//零结果后的分词处理
	ke_dodelete	 = 7				//删除记录
};
/* 对分布式时查询步骤的定义 */
enum ESearchStep {
	ss_ignore,		//普通的搜索模式,非分布式
	ss_first,		//分布式的第一次搜索
	ss_merge,		//分布式的merge状态
	ss_second		//分布式的第二次搜索
};

#if 0
enum EnumWorkMode {
    enum_SearchAndDocServer = 0,	//search 和DocData服务	
    enum_SearchServer,		//只做Search服务
    enum_DocDataServer		//只做DocData服务
};
#endif

/**The role in Document Separation mode. jinhui.li added in 2009.1.19*/
const int ROLE_SEARCH 	= 1 << 0; 
const int ROLE_DOC 		= 1 << 1;
const int ROLE_MIX 		= ROLE_SEARCH | ROLE_DOC; //the role of searcher and doc

//整数范围结构
struct SRange
{
	n32_t nMax;		//最大值
	n32_t nMin;		//最小值
	bool operator<(const SRange& rhs) const
	{
		return nMin < rhs.nMin;
	}
	bool operator!=(const SRange& rhs) const
	{
		return !(operator==(rhs));
	}
	bool operator==(const SRange& rhs) const
	{
		return nMin==rhs.nMin && nMax==rhs.nMax;
	}
	static const n32_t MAXVALUE = 0x7FFFFFFF;
	static const n32_t MINVALUE = 0x0;
};
//查询配置信息
struct QueryConfInfo {
	QueryConfInfo()
	{
        mPoolExtendSize = 1;
		loadAreaName[0] = '\0';
        iMaxDocCacheNum = 1000000;
		iAverageDocSize = 1024;
		role = ROLE_MIX;
		m_nMaxMemPoolSize = DEFAULT_MAX_MEMPOOL_SIZE;
		m_nMaxQueryLength = MAX_QUERY_LENGTH;

		m_nDocsFoundThreshold = 100000;
		m_nLatencyThreshold = 20;
		m_szAbnormalQueryFile[0] = '\0';
		m_nMaxSetOpComplexity = 10000000;
	}
	QueryConfInfo(const QueryConfInfo &other)
	{
		strncpy(loadAreaName, other.loadAreaName, 256);
		mPoolExtendSize = other.mPoolExtendSize;
		iMaxDocCacheNum = other.iMaxDocCacheNum;
		iAverageDocSize = other.iAverageDocSize;
		role = other.role;
		m_nMaxMemPoolSize = other.m_nMaxMemPoolSize;
		m_nMaxQueryLength = other.m_nMaxQueryLength;
		m_nDocsFoundThreshold = other.m_nDocsFoundThreshold;
		m_nLatencyThreshold = other.m_nLatencyThreshold;
		strncpy(m_szAbnormalQueryFile, other.m_szAbnormalQueryFile, 1024);
		m_nMaxSetOpComplexity = other.m_nMaxSetOpComplexity;
	}
	~QueryConfInfo() {}
	char loadAreaName[256];			//预先加载的area
	basis::n32_t mPoolExtendSize;	//mempool的size设置
	basis::n32_t iMaxDocCacheNum;   //最大cache的doc数
	basis::n32_t iAverageDocSize;   //平均文档大小
	int role; //机器的角色
	int    m_nMaxMemPoolSize;  //单个线程最大的mempool可分配的size
	int    m_nMaxQueryLength; //最大的query长度

    // added by zhanghp for stablizing the system when it is attacked by abnormal queries
    u_n32_t m_nDocsFoundThreshold;	
    u_n32_t m_nLatencyThreshold;	
	char m_szAbnormalQueryFile[1024];
	u_n32_t m_nMaxSetOpComplexity;
	// ended by zhanghp
};
union RangeType
{
  n8_t cvalue;
  n16_t svalue;
  float fvalue4;
    n32_t nvalue;
    n64_t lvalue;
    double fvalue;
    u_n64_t keyHash;
    char *szValue;
};

typedef RangeType ValueType;

struct NumRange
{
    RangeType min;
    RangeType max;
};

typedef NumRange SNumRange;

enum ENextPeriod {
	np_1st_less_now = 1,
	np_2nd_less_now
};

}

#endif

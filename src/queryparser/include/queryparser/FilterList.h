/***********************************************************************************
 * Describe : queryparser过滤信息链表包装的对外接口类
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-13
 **********************************************************************************/


#ifndef _QP_CLASS_FILTERLIST_H_
#define _QP_CLASS_FILTERLIST_H_

#include "commdef/commdef.h"
#include "util/Vector.h"

#define ITEMS_LEN_MAX 128

namespace queryparser {
/* 过滤条件类型 */
typedef enum
{
	EStringType,    // 字符串型过滤条件
	ESingleType,    // 单值型过滤条件
	ERangeType,     // 区间型过滤条件
} FilterItemType;

/* 过滤项对应的数据结构，一个域可能包含多个过滤项 */
typedef struct
{
	char *min;        // 最小值
	bool minEqual;                        // 最小值是否为闭区间
	char *max;        // 最大值
	bool maxEqual;                        // 最大值是否为闭区间
}FilterItem;


typedef struct  
{
	char           field_name[MAX_FIELD_NAME_LEN];          // 域名
	uint32_t       field_name_len;                          // 域名长度
	
	char           append_field_name[MAX_FIELD_NAME_LEN];   // 关联域名
	uint32_t       append_field_name_len;                   // 关联域名长度
	char           append_field_type[MAX_FIELD_NAME_LEN];   // 关联类型
	uint32_t       append_field_type_len;                   // 关联类型长度
	
    FilterItem     items[ITEMS_LEN_MAX];                    // 保存取值范围的数组，每种取值对应一个元素
	uint32_t       item_size;                                  // items中已经使用了多少个元素
	
	FilterItemType type;                                    // 该过滤项取值的类型
	bool           relation;                                // 该取值范围的逻辑关系（是或非关系)
	
}FilterNode;


class FilterList 
{
	public:
		
		FilterList();
		
		~FilterList();
		
		/**
		 * 获取节点个数
		 *
		 * @return < 0 执行错误
		 *         >=0 节点个数
		 */
		const int getNum() const;
		
		/*
		 * 获取旺旺在线状态
		 *
		 * @return 旺旺在线状态
		 */
		const OluStatType getOluStatType() const;
		
		/*
		 * 使游标指向第一个数据
		 */
		void first();
		
		/*
		 * 获取下一组数据
		 *
		 * @param fieldName 过滤项对应的字段名
		 * @param fieldItems 过滤项对应的取值范围
		 * @param number 过滤项对应的取值范围数组长度
		 * @param type 过滤项对应的取值类型
		 * @param relation 过滤项对应的关系（是/非）
		 *
		 * @return < 0 遍历完成
		 *         ==0 正确返回结果
		 */
		int next(char          *&fieldName,  char    *&appendFieldName, 
                 FilterItem    *&fieldItems, uint32_t &number,
			     FilterItemType &type,       bool     &relation);
		
		/**
		 * 打印过滤信息链表，调试时使用
		 */
		void print();
    
        void addFiltNode(FilterNode node);
	private:
        UTIL::Vector<FilterNode> _filter_nodes; 
        int      _node_size;
        int      _cur_index;
		
};


}
#endif

/***********************************************************************************
 * Describe : Query Parser 对外数据结构定义
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-03-08
 **********************************************************************************/


#ifndef _QP_QPTYPES_H_
#define _QP_QPTYPES_H_


#include <stdint.h>
#include "commdef/commdef.h"


namespace queryparser{


/* 过滤条件类型 */
typedef enum
{
	EStringType,    // 字符串型过滤条件
	ESingleType,    // 单值型过滤条件
	ERangeType,     // 区间型过滤条件
}
FilterItemType;

/* 过滤项对应的数据结构，一个域可能包含多个过滤项 */
typedef struct
{
	char* min;        // 最小值
	bool minEqual;    // 最小值是否为闭区间
	char* max;        // 最大值
	bool maxEqual;    // 最大值是否为闭区间
}
FilterItem;


}


#endif

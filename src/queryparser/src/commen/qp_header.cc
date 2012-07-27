#include "qp_header.h"


/* 与参数作用目标枚举对应的文字描述 */
extern char* qp_arg_dest_name[QP_DEST_MAX] = 
{
	"other",        // 其它目标
	"search",       // 该参数用于检索
	"filter",       // 该参数用于过滤
	"statistic",    // 该参数用于统计
	"sort",         // 该参数用于排序
};

/* 与参数作用目标枚举对应的文字描述长度 */
extern int qp_arg_dest_nlen[QP_DEST_MAX] = 
{
	5,    // 其它目标
	6,    // 该参数用于检索
	6,    // 该参数用于过滤
	9,    // 该参数用于统计
	4,    // 该参数用于排序
};

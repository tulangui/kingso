/***********************************************************************************
 * Describe : queryparser头文件
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-05-22
 **********************************************************************************/


#ifndef _QP_HEADER_H_
#define _QP_HEADER_H_


/* 参数作用目标枚举 */
typedef enum
{
	QP_OTHER_DEST,        // 其它目标
	QP_SYNTAX_DEST,       // 该参数用于检索
	QP_FILTER_DEST,       // 该参数用于过滤
	QP_STATISTIC_DEST,    // 该参数用于统计
	QP_SORT_DEST,         // 该参数用于排序
	QP_DEST_MAX,          // 参数枚举最大值
}
qp_arg_dest_t;


/* 与参数作用目标枚举对应的文字描述及长度 */
extern char* qp_arg_dest_name[QP_DEST_MAX];
extern int qp_arg_dest_nlen[QP_DEST_MAX];


#endif

/***********************************************************************************
 * Describe : detail核心代码
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-05-05
 **********************************************************************************/


#ifndef _DI_DETAIL_H_
#define _DI_DETAIL_H_


#include <stdint.h>
#include "mxml.h"


#define SID      "_sid_"         // nid字段标识
#define LIGHTKEY "_lightkey_"    // 飘红字段标识
#define USERLOC "userloc"
#define ET "!et"
#define LATLNG "latlng"
#define DL "dl"

#define MAX_MODEL_LEN 4096                       // detail样式信息最大长度                    
#define MAX_DETAIL_LEN 16384                     // detail输出展示信息最大长度(16K,by pujian)


#ifdef __cplusplus
extern "C"{
#endif

/*
 * 初始化detail
 *
 * @param config 配置
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
extern int di_detail_init(mxml_node_t* config);

/*
 * 销毁detail
 */
extern void di_detail_dest();

/*
 * @brief 从detail中获取v3格式的展示信息  
 *
 * @param id_list nid列表
 * @param args 各个字段的特殊处理参数
 * @param argcnt 参数个数
 * @param res 返回字符串数组
 * @param res_size 返回字符串数组长度
 *
 * @return < 0 获取出错
 *         ==0 获取成功
 */
extern int di_detail_get_v3(const char* id_list, const char* dl, char* args[], const int argcnt, char** res, int* res_size);

/*
 * 从detail中获取展示信息
 *
 * @param id_list nid列表
 * @param args 各个字段的特殊处理参数
 * @param argcnt 参数个数
 * @param res 返回字符串数组
 * @param res_size 返回字符串数组长度
 *
 * @return < 0 获取出错
 *         ==0 获取成功
 */
extern int di_detail_get(const char* id_list, const char* dl, char* args[], const int argcnt, char** res, int* res_size);

/*
 * 向detail中添加展示信息
 *
 * @param nid 文档nid
 * @param content 详细内容
 * @param len 详细内容长度
 *
 * @return < 0 添加出错
 *         ==0 添加成功
 */
extern int di_detail_add(int64_t nid, const char* content, int32_t len);

/*
 * 获取detail中存储的字段数量
 *
 * @return detail中存储的字段数量
 */
extern const int get_di_field_count();

/*
 * 获取detail中存储的字段名字数组
 *
 * @return detail中存储的字段名字数组
 */
extern const char** get_di_field_name();

/*
 * 获取detail中存储的字段名字长度数组
 *
 * @return detail中存储的字段名字长度数组
 */
extern const int* get_di_field_nlen();

/*
 * 申请字段参数数组
 *
 * @param args_p 存放申请到的字段参数数组
 *
 * @return < 0 申请出错
 *         >=0 数组长度
 */
extern int di_detail_arg_alloc(char*** args_p);

/*
 * 释放字段参数数组
 *
 * @param args_p 释放的字段参数数组
 */
extern void di_detail_arg_free(char** args);

/*
 * 向字段参数数组中添加参数
 *
 * @param args_p 字段参数数组
 * @param argcnt 字段参数数组长度
 * @param field 待添加字段
 * @param flen 待添加字段名字长度
 * @param arg 参数
 */
extern void di_detail_arg_set(char** args, const int argcnt, char* field, int flen, char* arg, const char* dl);

/*
 * 返回持久化数量文件路径
 *
 * @return 路径
 */
extern const char* get_di_msgcnt_file();

#ifdef __cplusplus
}
#endif


#endif

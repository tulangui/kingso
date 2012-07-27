/***********************************************************************************
 * Describe : detail缺省后处理器
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-07-17
 **********************************************************************************/

#ifndef _DI_DEFAULT_POSTPROCESSOR_H_
#define _DI_DEFAULT_POSTPROCESSOR_H_


#include "di_postprocessor_header.h"


#ifdef __cplusplus
extern "C"{
#endif


/*
 * 初始化detail后处理器
 *
 * @param field_count 字段总数
 *        field_array 字段名字数组
 *        flen_array 字段名字长度数组
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
extern int di_postprocessor_init(const int field_count, const char** field_array, const int* flen_array);

/*
 * 清理detail后处理器
 */
extern void di_postprocessor_dest();

/*
 * 执行detail后处理器
 *
 * @param data 一条detail数据
 *        size 数据长度
 *        args 处理每个字段需要的参数数组
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
extern char* di_postprocessor_work(char* input, int inlen, char** args);

/*
 * 向字段参数数组中添加参数
 *
 * @param args_p 字段参数数组
 * @param argcnt 字段参数数组长度
 * @param field 待添加字段
 * @param flen 待添加字段名字长度
 * @param arg 参数
 */
extern void di_argument_set(char** args, const int argcnt, const char* field, const int flen, char* arg);

/*
 * 获取detail后处理器名字
 *
 * @return detail后处理器名字
 */
extern const char* di_get_name_string();

/*
 * 获取detail后处理器名字长度
 *
 * @return detail后处理器名字长度
 */
extern const int di_get_name_length();

/*
 * 获取处理后展示的字段数量
 *
 * @return 字段数量
 */
extern const int di_get_field_count(const char* dl);

/*
 * 获取处理后展示的字段样式
 *
 * @return 字段样式
 */
extern const char* di_get_model_string(const char* dl);

/*
 * 获取处理后展示的字段样式长度
 *
 * @return 字段样式长度
 */
extern const int di_get_model_length(const char* dl);


#ifdef __cplusplus
}
#endif


#endif

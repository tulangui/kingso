/***********************************************************************************
 * Describe : detail后处理器管理器
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-07-17
 **********************************************************************************/

#ifndef _DI_POSTPROCESSOR_MANAGER_H_
#define _DI_POSTPROCESSOR_MANAGER_H_


#include "di_postprocessor_header.h"


#define POSTPROCESSER_MAX 1024


/* detail后处理器 */
typedef struct
{
	void* handle;
	di_get_name_string_t* name_string_func;      // 获取detail后处理器名字
	di_get_name_length_t* name_length_func;      // 获取detail后处理器名字长度
	di_get_field_count_t* field_count_func;      // 获取处理后展示的字段数量
	di_get_model_string_t* model_string_func;    // 获取处理后展示的字段样式
	di_get_model_length_t* model_length_func;    // 获取处理后展示的字段样式长度
	di_postprocessor_init_t* init_func;          // 初始化detail后处理器方法
	di_postprocessor_dest_t* dest_func;          // 清理detail后处理器方法
	di_postprocessor_work_t* work_func;          // 执行detail后处理器方法
	di_argument_set_t* arg_set_func;             // 字段参数设置方法
}
di_postprocessor_t;

/* detail后处理器管理器 */
typedef struct
{
	di_postprocessor_t postprocessors[POSTPROCESSER_MAX];    // detail后处理器
	int postprocessor_cnt;                                   // detail后处理器个数
}
di_postprocessor_manager_t;


#ifdef __cplusplus
extern "C"{
#endif


/*
 * 初始化detail后处理器管理
 *
 * @param manager detail后处理器管理
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
extern int di_postprocessor_manager_init(di_postprocessor_manager_t* manager, const char* plugin_path);

/*
 * 清理detail后处理器管理
 *
 * @param manager detail后处理器管理
 */
extern void di_postprocessor_manager_dest(di_postprocessor_manager_t* manager);

/*
 * 向detail后处理器管理器中加入后处理器
 *
 * @param manager detail后处理器管理
 * @param get_name_string_func 获取detail后处理器名字方法
 * @param get_name_length_func 获取detail后处理器名字长度方法
 * @param get_field_count_func 获取处理后展示的字段数量方法
 * @param get_model_string_func 获取处理后展示的字段样式方法
 * @param get_model_length_func 获取处理后展示的字段样式长度方法
 * @param init_func 初始化detail后处理器方法
 * @param dest_func 清理detail后处理器方法
 * @param work_func 执行detail后处理器方法
 *
 * @return < 0 加入出错
 *         ==0 加入成功
 */
extern int di_postprocessor_manager_put(di_postprocessor_manager_t* manager, void* handle,
	di_get_name_string_t* get_name_string_func,
	di_get_name_length_t* get_name_length_func,
	di_get_field_count_t* get_field_count_func,
	di_get_model_string_t* get_model_string_func,
	di_get_model_length_t* get_model_length_func,
	di_postprocessor_init_t* init_func,
	di_postprocessor_dest_t* dest_func,
	di_postprocessor_work_t* work_func,
	di_argument_set_t* arg_set_func);

/*
 * 从detail后处理器管理器中获取后处理器
 *
 * @param manager detail后处理器管理
 * @param name detail后处理器名字
 * @param name_len detail后处理器名字长度
 *
 * @return detail后处理器
 */
extern di_postprocessor_t* di_postprocessor_manager_get(di_postprocessor_manager_t* manager, const char* name, const int name_len);


#ifdef __cplusplus
}
#endif


#endif

/***********************************************************************************
 * Describe : detail后处理器代码
 * 
 * Author   : pianqian, pianqian.gc@taobao.com
 * 
 * Create   : 2011-07-15
 **********************************************************************************/

#ifndef _DI_POSTPROCESSOR_HEADER_H_
#define _DI_POSTPROCESSOR_HEADER_H_


#define GET_NAME_STRING_FUNC_NAME "di_get_name_string"
#define GET_NAME_LENGTH_FUNC_NAME "di_get_name_length"
#define GET_FIELD_COUNT_FUNC_NAME "di_get_field_count"
#define GET_MODEL_STRING_FUNC_NAME "di_get_model_string"
#define GET_MODEL_LENGTH_FUNC_NAME "di_get_model_length"
#define INIT_FUNC_NAME "di_postprocessor_init"
#define DEST_FUNC_NAME "di_postprocessor_dest"
#define WORK_FUNC_NAME "di_postprocessor_work"
#define ARG_SET_FUNC_NAME "di_argument_set"


/*
 * 初始化detail后处理器
 *
 * @param $0 字段总数
 *        $1 字段名字数组
 *        $2 字段名字长度数组
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
typedef int (di_postprocessor_init_t)(const int, const char**, const int*);

/*
 * 清理detail后处理器
 */
typedef void (di_postprocessor_dest_t)();

/*
 * 执行detail后处理器
 *
 * @param $0 一条detail数据
 *        $1 数据长度
 *        $2 处理每个字段需要的参数数组
 *
 * @return < 0 初始化出错
 *         ==0 初始化成功
 */
typedef char* (di_postprocessor_work_t)(char*, int, char**);

/*
 * 向字段参数数组中添加参数
 *
 * @param $0 字段参数数组
 * @param $1 字段参数数组长度
 * @param $2 待添加字段
 * @param $3 待添加字段名字长度
 * @param $4 参数
 */
typedef void (di_argument_set_t)(char**, const int, const char*, const int, char*);

/*
 * 获取detail后处理器名字
 *
 * @return detail后处理器名字
 */
typedef const char* (di_get_name_string_t)();

/*
 * 获取detail后处理器名字长度
 *
 * @return detail后处理器名字长度
 */
typedef const int (di_get_name_length_t)();

/*
 * 获取处理后展示的字段数量
 *
 * @return 字段数量
 */
typedef const int (di_get_field_count_t)(const char*);

/*
 * 获取处理后展示的字段样式
 *
 * @return 字段样式
 */
typedef const char* (di_get_model_string_t)(const char*);

/*
 * 获取处理后展示的字段样式长度
 *
 * @return 字段样式长度
 */
typedef const int (di_get_model_length_t)(const char*);

#endif

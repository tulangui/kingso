
/** \file
 *******************************************************************
 * $Author: gongyi.cl $
 *
 * $LastChangedBy: gongyi.cl $
 *
 * $Revision: 135 $
 *
 * $LastChangedDate: 2011-03-23 15:47:00 +0800 (Wed, 23 Mar 2011) $
 *
 * $Id: charsetfunc.h 135 2011-03-23 07:47:00Z gongyi.cl $
 *
 *******************************************************************
 */

#ifndef _CODE_H
#define _CODE_H

#include <stdint.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

    enum {
        CC_GBK,
        CC_UTF8,
        CC_COUNT
    };

#define CC_FLAG_IGNORE      1
#define CC_FLAG_TRANSLIT    2

    typedef void *code_conv_t;

/**
 * @brief 打开一个转换对象
 *
 * @param to 目标编码
 * @param from 源编码
 * @param flag CC_FLAG_IGNORE 忽略错误,安静地执行
 * @param flag CC_FLAG_TRANSLIT 将不能转换的字符转换为"近似的".
 * @return NULL 失败
 * @return NOT_NULL 转换句柄
 */
    code_conv_t code_conv_open(uint32_t to, uint32_t from, uint32_t flag);

/**
 * @brief 执行cc制定类型的转换
 * 
 * @param cc code_conv_open()打开的转换句柄
 * @param out 输出缓冲
 * @param size 输出缓冲的长度
 * @param in 输入字符串(必须0结束)
 * @return >=0 输出缓冲的长度(不包含末尾的0)
 * @return <0 错误,但out可能已经被修改，可能包含部分结果（如size不足的情况)
 */
    ssize_t code_conv(code_conv_t cc, char *out, ssize_t size,
                      const char *in);

/** @brief 关闭cc句柄
 *
 * @param cc code_conv_open()打开的转换句柄
 */
    void code_conv_close(code_conv_t cc);

/** 
 * @breif 将utf8编码转换为gbk. 每次执行重新打开句柄(code_conv_t),频繁使用效率较低.
 *
 * @param out 输出缓冲
 * @param size 输出缓冲的长度
 * @param in 输入字符串(必须0结束)
 * @return >=0 输出缓冲的长度(不包含末尾的0)
 * @return <0 错误
 */
    ssize_t code_gbk_to_utf8(char *out, ssize_t size,
                             const char *in, uint32_t flag);

/** 
 * @breif 将gbk编码转换为utf-8. 每次执行重新打开句柄(code_conv_t),频繁使用效率较低.
 *
 * @param out 输出缓冲
 * @param size 输出缓冲的长度
 * @param in 输入字符串(必须0结束)
 * @return >=0 输出缓冲的长度(不包含末尾的0)
 * @return <0 错误
 */
    ssize_t code_utf8_to_gbk(char *out, ssize_t size,
                             const char *in, uint32_t flag);

#ifdef __cplusplus
}
#endif
#endif

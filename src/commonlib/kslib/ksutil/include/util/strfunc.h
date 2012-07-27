
/** \file
 *******************************************************************
 * $Author: zhuangyuan $
 *
 * $LastChangedBy: zhuangyuan $
 *
 * $Revision: 1796 $
 *
 * $LastChangedDate: 2011-05-13 16:10:54 +0800 (Fri, 13 May 2011) $
 *
 * $Id: strfunc.h 1796 2011-05-13 08:10:54Z zhuangyuan $
 *
 * $Brief: string functions $
 *******************************************************************
 */

#ifndef STR_FUNC_H
#define STR_FUNC_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

    /**
     * @brief lowercase the string
     * @param src input string, will be changed by this function
     * @return lowercased string
     */
    char *str_lowercase(char *src);

    /**
     * @brief uppercase the string
     * @param src input string, will be changed by this function
     * @return uppercased string
     */
    char *str_uppercase(char *src);

    /**
     * @brief convert buffer content to hex string such as 0B82A1
     * @param s the buffer
     * @param len the buffer length
     * @param hex_buff store the hex string (must have enough space)
     * @return hex string (hex_buff)
     */
    char *str_bin2hex(const char *s, const int len, char *hex_buff);

    /**
     * @brief parse hex string to binary content
     * @param s the hex string such as 8B04CD
     * @param bin_buff store the converted binary content(must have enough space)
     * @param dest_len store the converted content length
     * @return converted binary content (bin_buff)
     */
    char *str_hex2bin(const char *s, char *bin_buff, int *dest_len);

    /**
     * @brief print binary buffer as hex string
     * @param s the buffer to print
     * @param len the buffer length
     */
    void str_print_buffer(const char *s, const int len);

    /**
     * @brief trim leading spaces ( \\t\\r\\n)
     * @param str the string to trim
     * @return: the left trimed string porinter as str
     */
    char *str_ltrim(char *str);

    /**
     * @brief trim tail spaces ( \\t\\r\\n)
     * @param str the string to trim
     * @return the right trimed string porinter as str
     */
    char *str_rtrim(char *str);

    /**
     * @brief trim leading and tail spaces ( \\t\\r\\n)
     * @param str the string to trim
     * @return the trimed string porinter as str
     */
    char *str_trim(char *str);

    /**
     * @brief split string, NOT recommand
     * @param src the source string, will be modified by this function
     * @param seperator seperator char
     * @param max_cols max columns (max split count)
     * @param col_count store the columns (array elements) count
     * @return string array, should call str_free_split to free, 
     * @return return NULL when fail
     */
    char **str_split(char *src, const char seperator,
                     const int max_cols, int *col_count);

    /**
     * @brief free split results
     * @param p return by function str_split
     */
    void str_free_split(char **p);

    /**
     * @brief split string, recommand to use this function
     * @param src the source string, will be modified by this function
     * @param seperator seperator char
     * @param cols store split strings
     * @param max_cols max columns (max split count)
     * @return string array / column count
     */
    int str_split_ex(char *src, const char seperator,
                     char **cols, const int max_cols);

    /**
     * @brief 分割字符串，试用delims中的所有字符作为分隔符，输入字符串将被修改！！！
     * @param str待切分的字符串
     * @param items 存储切分结果的char*[] 数组
     * @param item_size char*[]数组的最大长度
     * @param delims 分割符字符串，0结束
     * @return -1 失败
     * @return >=0 切分结果数量
     */
    int split_multi_delims(char *str, char **items, int item_size,
                         const char *delims);

#ifdef __cplusplus
}
#endif
#endif

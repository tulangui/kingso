/** \file
 *******************************************************************
 * $Author: zhuangyuan $
 *
 * $LastChangedBy: zhuangyuan $
 *
 * $Revision: 1847 $
 *
 * $LastChangedDate: 2011-05-16 11:16:11 +0800 (Mon, 16 May 2011) $
 *
 * $Id: common.h 1847 2011-05-16 03:16:11Z zhuangyuan $
 *
 * $Brief: common defines  $
 *******************************************************************
 */

#ifndef _COMMON_H_
#define _COMMON_H_

#include <stdint.h>
#include <errno.h>

#ifndef __cplusplus
    #ifndef true
        typedef char  bool;
        #define true  1
        #define false 0
     #endif
#endif

/* define likely/unlikely/expected. */
#if __GNUC__ > 2 || __GNUC_MINOR__ >= 96
#  define likely(x)       __builtin_expect(!!(x),1)
#  define unlikely(x)     __builtin_expect(!!(x),0)
#  define expected(x,y)   __builtin_expect((x),(y))
#else //__GNUC__ > 2 || __GNUC_MINOR__ >= 96
#  define likely(x)       (x)
#  define unlikely(x)     (x)
#  define expected(x,y)   (x)
#endif //__GNUC__ > 2 || __GNUC_MINOR__ >= 96

#define SUCCESS 0
#define EFAILED 201 /*程序处理失败*/
#define EUNKNOW 202 /*未知错误*/

/* define for close assign operator and copy constructor. */
#define COPY_CONSTRUCTOR(T) \
        T(const T &); \
T & operator=(const T &);

/* define STR. */
#define _STR(x) # x
#define STR(x) _STR(x)

/* define SAFE_STRING */
#define SAFE_STRING(x) (x ? x : "(null)")
#define SAFE_STRING_LENGTH(x) (x ? strlen(x) : 0)
#define SAFE_STRING_EQUAL(x,y) (x && y ? (strcmp(x, y) == 0) : (x == y))

#endif

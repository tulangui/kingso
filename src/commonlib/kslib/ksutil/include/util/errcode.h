/** \file
 *******************************************************************
 * $Author: zhuangyuan $
 *
 * $LastChangedBy: zhuangyuan $
 *
 * $Revision: 293 $
 *
 * $LastChangedDate: 2011-04-01 17:32:50 +0800 (Fri, 01 Apr 2011) $
 *
 * $Id: errcode.h 293 2011-04-01 09:32:50Z zhuangyuan $
 *
 * $Brief: kingso error code defines $
 *******************************************************************
 */

#ifndef _ERR_CODE_H_
#define _ERR_CODE_H_

/* error code define */
#define KS_SUCCESS    0 //success
#define KS_EFAILED   -1 //程序处理失败
#define KS_ENOENT    -2 //实体不存在
#define KS_EINTR     -3 //调用被中断
#define KS_EIO       -4 //IO错误
#define KS_EAGAIN    -5 //需要重试
#define KS_ENOMEM    -6 //内存不足，比如分配内存失败时，会返回这个错误
#define KS_EEXIST    -7 //实体已存在
#define KS_EINVAL    -8 //参数无效
#define KS_ETIMEDOUT -9 //超时错误

#endif

/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 498 $
 *
 * $LastChangedDate: 2011-04-08 16:37:41 +0800 (五, 08  4月 2011) $
 *
 * $Id: SearchDef.h 498 2011-04-08 08:37:41Z boluo.ybb $
 *
 * $Brief: search模块全局定义 $
 ********************************************************************
 */

#ifndef _SEARCH_DEF_H_
#define _SEARCH_DEF_H_

namespace search{

#define MAX_INT64 0x7FFFFFFFFFFFFFFF
#define MIN_INT64 0x8000000000000000
#define MAX_UINT64 0xFFFFFFFFFFFFFFFF
#define MIN_UINT64 0x0000000000000000

#define MAX_INT32 0x7fffffff
#define MIN_INT32 0x80000000

#define INDEX_CUT_LEN 100000

#define DISCOUNT_BASE 10000

#define INVAILD_ZKGROUP -1000
#define TERR2MSG(context, msg) context->setErrorMessage(msg); TERR(msg)

}


#endif

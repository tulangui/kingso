/** \file
 ********************************************************************
 * $Author: boluo.ybb $
 *
 * $LastChangedBy: boluo.ybb $
 *
 * $Revision: 497 $
 *
 * $LastChangedDate: 2011-04-08 16:34:45 +0800 (五, 08  4月 2011) $
 *
 * $Id: SearchResult.h 497 2011-04-08 08:34:45Z boluo.ybb $
 *
 * $Brief: search module return type $
 ********************************************************************
 */

#ifndef _SEARCH_RESULT_H_
#define _SEARCH_RESULT_H_

#include <stdint.h>

typedef struct SearchResult
{
    uint32_t *nDocIds;                           /* docid结果数组 */
    uint32_t *nRank;                             /* rank分数数组 */
    uint32_t nDocSize;                          /* search到的doc数组大小 */
    uint32_t nDocFound;                          /* search到的有效doc数 */
    uint32_t nEstimateDocFound;                  /* 索引截断预估有效doc数 */
    uint32_t nDocSearch;                         /* search的doc总量 */
} SearchResult;

#endif

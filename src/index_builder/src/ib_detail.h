/*********************************************************************
 * $Author: cuilun $
 *
 * $LastChangedBy: cuilun $
 *
 * $Revision: 2577 $
 *
 * $LastChangedDate: 2011-03-09 09:50:55 +0800 (Wed, 09 Mar 2011) $
 *
 * $Id: ib_detail.h 2577 2011-03-09 01:50:55Z cuilun $
 *
 * $Brief: ini file parser $
 ********************************************************************/

#ifndef SRC_IB_DETAIL_H_
#define SRC_IB_DETAIL_H_

#include <stdint.h>
#include <zlib.h>

#include "ib_ctx.h"
#include "ks_build/ks_build.h"

typedef struct ib_detail_idx_s {
    ib_detail_idx_s(int64_t l1, uint64_t l2, uint64_t l3) : nid(l1), off(l2), len(l3) {}
    int64_t nid;
    uint64_t off;
    uint64_t len;
}ib_detail_idx_t;

int ib_write_detail(gzFile fp, gzFile fp_idx, ksb_conf_t *conf, ksb_result_t *presult);

#endif //SRC_IB_DETAIL_H_
